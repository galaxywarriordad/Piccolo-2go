// ax-powerd.c — Core 3 safe KEY_POWER, 75C thermal rollback & 3280mV battery guard for RK3326
#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
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
        char path;
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", cpu);
        FILE* f = fopen(path, "w");
        if (f) {
            fprintf(f, "1296000\n");
            fclose(f);
        }
    }
}

static void run_cmd_best_effort(const char* cmd) {
    int rc = system(cmd);
    (void)rc;
}

static void clean_poweroff(void) {
    run_cmd_best_effort("pkill -USR1 -x flac_ax 2>/dev/null || true");
    run_cmd_best_effort("pkill -TERM -x retroarch 2>/dev/null || true");
    usleep(250000);
    sync();
    run_cmd_best_effort("/etc/init.d/S02piccolo-2go-storage stop 2>/dev/null || true");
    sync();
    reboot(RB_POWER_OFF);
}

int main(void) {
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);
    pin_to_core3();

    struct pollfd pfds;
    int nfds = 0;

    DIR* d = opendir("/dev/input");
    if (d) {
        struct dirent* ent;
        while ((ent = readdir(d)) != NULL && nfds < 16) {
            if (strncmp(ent->d_name, "event", 5) != 0) continue;
            char path;
            snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
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

        // Every ~5 seconds: check 75C thermal rollback & 3280 mV LiPo brownout floor
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
                char status_buf = {0};
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
