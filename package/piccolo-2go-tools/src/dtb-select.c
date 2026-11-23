// dtb-select.c — Runtime Device Tree selector for BATLEXP G350, R36H, R46H & R36S variants
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct DtbAlias {
    const char* alias;
    const char* filename;
};

static const struct DtbAlias kAliases[] = {
    {"g350",             "rk3326-g350-linux.dtb"},
    {"r36h",             "rk3326-r36h-linux.dtb"},
    {"r46h",             "rk3326-r46h-linux.dtb"},
    {"r36s-plus",        "rk3326-r36splus-linux.dtb"},
    {"r36s-panel0",      "rk3326-r36s-panel0-linux.dtb"},
    {"r36s-panel1",      "rk3326-r36s-panel1-linux.dtb"},
    {"r36s-panel2",      "rk3326-r36s-panel2-linux.dtb"},
    {"r36s-panel3",      "rk3326-r36s-panel3-linux.dtb"},
    {"r36s-panel4",      "rk3326-r36s-panel4-type1-linux.dtb"},
    {"r36s-panel4-t1",   "rk3326-r36s-panel4-type1-linux.dtb"},
    {"r36s-panel4-t2",   "rk3326-r36s-panel4-type2-linux.dtb"},
    {"r36s-v21",         "rk3326-r36s-v21-linux.dtb"},
    {"r36s-pro",         "rk3326-r36pro-linux.dtb"},
    {"r36s-xx",          "rk3326-r36xx-linux.dtb"},
};

static int is_safe_filename(const char* name) {
    if (!name || name[0] == '\0') return 0;
    if (strchr(name, '/') != NULL || strstr(name, "..") != NULL) return 0;
    return 1;
}

static int copy_file_sync(const char* src, const char* dst) {
    int in_fd = open(src, O_RDONLY | O_CLOEXEC);
    if (in_fd < 0) return -1;

    int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (out_fd < 0) {
        close(in_fd);
        return -1;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(in_fd, buf, sizeof(buf))) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out_fd, buf + off, (size_t)(n - off));
            if (w <= 0) {
                close(in_fd);
                close(out_fd);
                return -1;
            }
            off += w;
        }
    }

    fsync(out_fd);
    close(in_fd);
    close(out_fd);
    return (n < 0) ? -1 : 0;
}

int main(int argc, char** argv) {
    const char* dtb_dir = "/boot/dtbs";
    const char* active_dtb = "/boot/rk3326-g350-linux.dtb";

    if (argc < 2) {
        fprintf(stderr, "Usage: dtb-select <list | g350 | r36h | r46h | r36s-plus | r36s-panel0..4 | r36s-v21 | filename.dtb>\n");
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        printf("Built-in aliases:\n");
        for (size_t i = 0; i < sizeof(kAliases) / sizeof(kAliases[0]); ++i) {
            printf("  %-18s -> %s\n", kAliases[i].alias, kAliases[i].filename);
        }
        DIR* d = opendir(dtb_dir);
        if (d) {
            printf("\nInstalled /boot/dtbs/*.dtb files:\n");
            struct dirent* ent;
            while ((ent = readdir(d)) != NULL) {
                const char* ext = strrchr(ent->d_name, '.');
                if (ext && strcmp(ext, ".dtb") == 0) {
                    printf("  %s\n", ent->d_name);
                }
            }
            closedir(d);
        }
        return 0;
    }

    const char* target_name = argv[1];
    for (size_t i = 0; i < sizeof(kAliases) / sizeof(kAliases[0]); ++i) {
        if (strcmp(argv[1], kAliases[i].alias) == 0) {
            target_name = kAliases[i].filename;
            break;
        }
    }

    if (!is_safe_filename(target_name)) {
        fprintf(stderr, "[dtb-select] Invalid DTB name '%s'\n", target_name);
        return 1;
    }

    char src_path[512];
    int written = snprintf(src_path, sizeof(src_path), "%s/%s", dtb_dir, target_name);
    if (written < 0 || (size_t)written >= sizeof(src_path)) {
        fprintf(stderr, "[dtb-select] Path too long for '%s'\n", target_name);
        return 1;
    }

    if (access(src_path, R_OK) != 0) {
        fprintf(stderr, "[dtb-select] Source DTB not readable '%s': %s\n", src_path, strerror(errno));
        return 1;
    }

    if (copy_file_sync(src_path, active_dtb) != 0) {
        fprintf(stderr, "[dtb-select] Failed to activate '%s': %s\n", src_path, strerror(errno));
        return 1;
    }

    sync();
    printf("[dtb-select] Activated '%s' -> '%s'. Reboot to apply.\n", src_path, active_dtb);
    return 0;
}
