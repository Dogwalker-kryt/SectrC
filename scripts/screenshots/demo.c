/* Prints SectrC screens with made-up drives for the README pictures.
   It only calls the real UI functions: no disk is listed, opened or written. */
#include "../../src/defs.h"
#include "../../src/globals.h"
#include "../../src/ui/ui.h"
#include "../../src/list_drives/list_drives.h"
#include "../../src/clone/cl_ui.h"
#include "../../src/disk_health/health_ui.h"
#include "../../src/io_stats/io_ui.h"
#include <sys/statvfs.h>
#define GB (1024ULL*1024*1024)
int __wrap_statvfs(const char *p, struct statvfs *s) {
    memset(s, 0, sizeof *s);
    double t = strcmp(p, "/") == 0 ? 914 : strcmp(p, "/boot/efi") == 0 ? 0.5 : 3600;
    double f = strcmp(p, "/") == 0 ? 0.58 : strcmp(p, "/boot/efi") == 0 ? 0.02 : 0.83;
    s->f_frsize = 4096; s->f_blocks = (fsblkcnt_t)(t * GB / 4096);
    s->f_bfree = s->f_bavail = (fsblkcnt_t)((double)s->f_blocks * (1 - f));
    return 0;
}
static void add(Vec *v, const char *n, const char *par, const char *model, const char *bus, const char *ser, const char *fs, const char *lab, const char *mnt, uint64_t sz) {
    drive_mt d; memset(&d, 0, sizeof d);
#define C(f, x) snprintf(d.f, sizeof d.f, "%s", x)
    C(name, n); C(parent, par); C(model, model); C(bus, bus); C(serial, ser); C(fstype, fs); C(label, lab); C(mountpoint, mnt);
    C(type, par[0] ? "partition" : "disk"); d.size = sz; vec_add_element(v, &d);
}
static void mark(int ms) { fflush(stderr); printf("\x1e%d;", ms); fflush(stdout); }
static void type(const char *cmd) {
    printf("\033[1;32m$\033[0m "); mark(500);
    for (; *cmd; ++cmd) { putchar(*cmd); mark(45); }
    mark(350); putchar('\n');
}
int main(int c, char **a) {
    (void)c; globals.no_color = false; globals.no_log = true; ui_program_name = "sectrc";
    setvbuf(stdout, NULL, _IONBF, 0);
    Vec v; vec_init(&v, 16, sizeof(drive_mt));
    add(&v, "/dev/nvme0n1", "", "Example NVMe 1TB", "nvme", "DEMO0000001", "", "", "", 931*GB);
    add(&v, "/dev/nvme0n1p1", "/dev/nvme0n1", "", "nvme", "", "vfat", "EFI", "/boot/efi", GB/2);
    add(&v, "/dev/nvme0n1p2", "/dev/nvme0n1", "", "nvme", "", "ext4", "system", "/", 914*GB);
    add(&v, "/dev/nvme0n1p3", "/dev/nvme0n1", "", "nvme", "", "swap", "", "", 16*GB);
    add(&v, "/dev/sda", "", "Example HDD 4TB", "ata", "DEMO0000002", "", "", "", 3726*GB);
    add(&v, "/dev/sda1", "/dev/sda", "", "ata", "", "ext4", "backup", "/mnt/backup", 3726*GB);
    add(&v, "/dev/sdb", "", "Example Stick", "usb", "DEMO0000003", "", "", "", 32*GB);
    add(&v, "/dev/sdb1", "/dev/sdb", "", "usb", "", "vfat", "BOOT", "", GB/2);
    add(&v, "/dev/sdb2", "/dev/sdb", "", "usb", "", "ext4", "rescue", "", 31*GB);
    add(&v, "/dev/sdc", "", "Example Stick XL", "usb", "DEMO0000004", "", "", "", 64*GB);
    add(&v, "/dev/sdc1", "/dev/sdc", "", "usb", "", "exfat", "STICK", "", 64*GB);
    drive_mt *d = v.data;
    if (!strcmp(a[1], "list")) print_disk_infos(d, v.element_count, NULL);
    if (!strcmp(a[1], "nvme")) {
        nvme_health_t h = {.temperature=38,.available_spare=100,.spare_threshold=10,.percentage_used=7,.data_read=24ULL*1024*GB,.data_written=18ULL*1024*GB,.read_commands=412345678,.write_commands=298765432,.power_cycles=640,.power_on_hours=5210,.unsafe_shutdowns=9,.error_log_entries=0,.level=HEALTH_OK,.reason="no problems reported"};
        print_nvme_health(&d[0], &h);
    }
    if (!strcmp(a[1], "ata")) {
        ata_smart_t s = {.checksum_ok=true,.count=8,.level=HEALTH_FAILING,.reason="attribute 5 is at or below its threshold (9 <= 10)",
        .attrs={{1,0,200,200,6,0},{5,0,9,9,10,1500},{9,0,71,71,0,21480},{12,0,99,99,0,845},{194,0,112,95,0,41},{197,0,200,200,0,8},{198,0,100,100,0,0},{199,0,200,200,0,2}}};
        print_ata_smart(&d[4], &s);
    }
    if (!strcmp(a[1], "info")) {
        io_stats_fmt f = {.data={"183204","40117","12.84 GB (12840001536 bytes)","41.208 s (41208 ms)","96512","120844","9.31 GB (9310003200 bytes)","63.977 s (63977 ms)","0","88.140 s (88140 ms)","105.185 s (105185 ms)","310","0","2.10 GB (2100002816 bytes)","0.412 s (412 ms)"}};
        print_io_stats("nvme0n1", &f);
    }
    if (!strcmp(a[1], "clone")) {
        type("sudo sectrc --clone sdb:sdc");
        cl_print_plan(&v, &d[6], &d[9], 32*GB, 64*GB); mark(700);
        cl_confirm_clone("/dev/sdb", "/dev/sdc", 64*GB); mark(1600);
        printf("y"); mark(400); printf("\n\n");
        for (int i = 0; i <= 40; ++i) { cl_print_progress(32*GB*i/40, 32*GB, 178.0*i/40); mark(i ? 110 : 300); }
        putchar('\n');
        cl_print_done("/dev/sdb", "/dev/sdc", 32*GB, 64*GB, 178.0, false); mark(4500);
    }
    return 0;
}
