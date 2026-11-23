// ax-boot-recovery.c — Instant EVIOCGKEY hold-button safe-boot detector for BATLEXP G350 & R36S
#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef BTN_TRIGGER_HAPPY5
#define BTN_TRIGGER_HAPPY5 0x2c4
#endif

static int is_key_pressed(const unsigned long* key_bits, int code) {
    if (code < 0 || code >= KEY_MAX) return 0;
    const unsigned long bits_per_long = sizeof(unsigned long) * 8;
    return (key_bits[code / bits_per_long] & (1UL << (code % bits_per_long))) != 0;
}

int main(void) {
    DIR* d = opendir("/dev/input");
    if (!d) return 1;

    int triggered = 0;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "event", 5) != 0) continue;
        char path[512];
        int n = snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
        if (n < 0 || (size_t)n >= sizeof(path)) continue;

        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;

        unsigned long key_bits[(KEY_MAX / (sizeof(unsigned long) * 8)) + 1];
        memset(key_bits, 0, sizeof(key_bits));
        if (ioctl(fd, EVIOCGKEY(sizeof(key_bits)), key_bits) >= 0) {
            if (is_key_pressed(key_bits, BTN_SELECT) ||
                is_key_pressed(key_bits, BTN_TRIGGER_HAPPY5) ||
                is_key_pressed(key_bits, BTN_MODE) ||
                is_key_pressed(key_bits, BTN_EAST)) {
                triggered = 1;
            }
        }
        close(fd);
        if (triggered) break;
    }
    closedir(d);

    return triggered ? 0 : 1;
}
