/* Feeds made-up results for a few known PCs into the scoring code and
   writes each report, to check the scores and game lists by eye.
   Build: see tests/run_tests.sh */
#include "../src/wb.h"

static char buf[60000];

static void one(const char *file, const char *os, int nt, int maj, int min, int sp, double dx,
                double dmips, double mwips, double mr, double mw, double mc,
                double dr, double dw, double da, const char *adapter, unsigned ram)
{
    Results r; HANDLE h; DWORD n;
    memset(&r, 0, sizeof r);
    strcpy(r.si.os_name, os); r.si.os_is_nt = nt; r.si.os_major = maj; r.si.os_minor = min; r.si.os_sp = sp;
    r.si.dx_ver = dx; strcpy(r.si.dx_text, "test");
    strcpy(r.si.cpu_name, "test CPU"); r.si.ram_mb = ram; strcpy(r.si.adapter, adapter);
    strcpy(r.si.temp_dir, "C:\\TEMP\\"); r.si.disk_free_mb = 5000;
    r.cpu.dmips = dmips; r.cpu.mwips = mwips;
    r.mem.read_mbs = mr; r.mem.write_mbs = mw; r.mem.copy_mbs = mc;
    r.disk.ok = 1; r.disk.unbuffered = 1; r.disk.read_mbs = dr; r.disk.write_mbs = dw; r.disk.access_ms = da;
    strcpy(r.gpu.err, "test"); /* no OpenGL run: card comes from the adapter name */
    score_compute(&r);
    report_build(&r, buf, sizeof buf);
    h = CreateFileA(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    WriteFile(h, buf, strlen(buf), &n, 0);
    CloseHandle(h);
}

void __stdcall test_start(void)
{
    /* the reference PC itself: should score close to 100 */
    one("t_ref.txt", "Windows 98 SE", 0, 4, 10, 0, 7, 1595, 816, 580, 280, 220, 30, 28, 13, "NVIDIA GeForce2 GTS", 256);
    /* 1997: Pentium MMX 200, EDO, Voodoo on top of a 2D card */
    one("t_1997.txt", "Windows 98", 0, 4, 10, 0, 6, 276, 134, 110, 70, 45, 7, 6, 17, "3dfx Voodoo Graphics", 32);
    /* 2004: Athlon 64 3200+, DDR400, Radeon 9800 Pro, XP SP2 */
    one("t_2004.txt", "Windows XP SP2", 1, 5, 1, 2, 9.3, 4000, 1600, 3000, 1500, 1800, 50, 48, 13, "RADEON 9800 PRO", 1024);
    ExitProcess(0);
}
