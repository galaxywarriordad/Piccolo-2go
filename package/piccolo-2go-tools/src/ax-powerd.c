// ax-powerd.c — Core 3 safe KEY_POWER, 75C thermal rollback & 3280mV battery guard for RK3326
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef BTN_TRIGGER_HAPPY5
#define BTN_TRIGGER_HAPPY5 0x2c4
#endif

static volatile sig_atomic_t g_running = 1;

static void on_sig(int sig) {
    (void)sig;
    g_running = 0;
}

static void pin_to_core3(void) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(3, &set);
    (void)sched_setaffinity(0, sizeof(set), &set);
}

static int read_sysfs_int(const char* path, int fallback) {
    FILE* f = fopen(path, "r");
    if (!f) return fallback;
    int val = fallback;
    if (fscanf(f, "%d", &val) != 1) val = fallback;
    fclose(f);
    return val;
}

static void enforce_safe_clock_1300(void) {
    for (int cpu = 0; cpu < 4; ++cpu) {
        char path[256];
        int n = snprintf(path, sizeof(path),
                         "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", cpu);
        if (n < 0 || (size_t)n >= sizeof(path)) continue;
        FILE* f = fopen(path, "w");
        if (f) {
            fprintf(f, "1296000\n");
            fclose(f);
        }
    }
}

static void run_cmd_best_effort(char* const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return;
    if (pid == 0) {
        int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (devnull >= 0) {
            (void)dup2(devnull, STDIN_FILENO);
            (void)dup2(devnull, STDOUT_FILENO);
            (void)dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) close(devnull);
        }
        execvp(argv[0], argv);
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) break;
    }
}

static void clean_poweroff(void) {
    char* const stop_flac[] = {"pkill", "-USR1", "-x", "flac_ax", NULL};
    char* const stop_ra[] = {"pkill", "-TERM", "-x", "retroarch", NULL};
    char* const stop_storage[] = {"/etc/init.d/S02piccolo-2go-storage", "stop", NULL};

    run_cmd_best_effort(stop_flac);
    run_cmd_best_effort(stop_ra);
    usleep(250000);
    sync();
    run_cmd_best_effort(stop_storage);
    sync();
    (void)reboot(RB_POWER_OFF);
    _exit(0);
}

int main(void) {
    if (geteuid() != 0) {
        fprintf(stderr, "[ax-powerd] Must run as root\n");
        return 1;
    }

    setenv("PATH", "/usr/sbin:/usr/bin:/sbin:/bin", 1);
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);
    pin_to_core3();

    struct pollfd pfds[16];
    int nfds = 0;

    DIR* d = opendir("/dev/input");
    if (d) {
        struct dirent* ent;
        while ((ent = readdir(d)) != NULL && nfds < 16) {
            if (strncmp(ent->d_name, "event", 5) != 0) continue;
            char path[512];
            int n = snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
            if (n < 0 || (size_t)n >= sizeof(path)) continue;
            int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd >= 0) {
                pfds[nfds].fd = fd;
                pfds[nfds].events = POLLIN;
                nfds++;
            }
        }
        closedir(d);
    }

    struct timespec press_ts = {0, 0};
    int power_held = 0;
    int fn_held = 0;
    int throttled_to_1300 = 0;
    int tick_counter = 0;

    while (g_running) {
        int rc = poll(pfds, (nfds_t)nfds, 500);
        if (rc > 0) {
            for (int i = 0; i < nfds; ++i) {
                if (!(pfds[i].revents & POLLIN)) continue;
                struct input_event ev;
                while (read(pfds[i].fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
                    if (ev.type == EV_KEY) {
                        if (ev.code == BTN_TRIGGER_HAPPY5 || ev.code == BTN_MODE) {
                            fn_held = (ev.value != 0);
                        } else if (ev.code == KEY_POWER) {
                            if (ev.value == 1) {
                                if (fn_held) {
                                    clean_poweroff();
                                }
                                clock_gettime(CLOCK_MONOTONIC, &press_ts);
                                power_held = 1;
                            } else if (ev.value == 0) {
                                power_held = 0;
                            }
                        }
                    }
                }
            }
        }

        if (power_held) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed_ms = (now.tv_sec - press_ts.tv_sec) * 1000L +
                              (now.tv_nsec - press_ts.tv_nsec) / 1000000L;
            if (elapsed_ms >= 1500L) {
                clean_poweroff();
                break;
            }
        }

        if (++tick_counter >= 10) {
            tick_counter = 0;
            int temp_mc = read_sysfs_int("/sys/class/thermal/thermal_zone0/temp", 0);
            if (!throttled_to_1300 && temp_mc >= 75000) {
                enforce_safe_clock_1300();
                throttled_to_1300 = 1;
            }

            int uvolt = read_sysfs_int("/sys/class/power_supply/battery/voltage_now", 3800000);
            int discharging = 1;
            FILE* st = fopen("/sys/class/power_supply/battery/status", "r");
            if (st) {
                char status_buf[32] = {0};
                if (fgets(status_buf, sizeof(status_buf), st) &&
                    strncmp(status_buf, "Charging", 8) == 0) {
                    discharging = 0;
                }
                fclose(st);
            }
            if (discharging && uvolt > 2500000 && uvolt <= 3280000) {
                clean_poweroff();
                break;
            }
        }
    }

    for (int i = 0; i < nfds; ++i) {
        close(pfds[i].fd);
    }
    return 0;
}
