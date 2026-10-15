/* What is this PC? Windows version, DirectX, CPU, RAM, display adapter,
   temp folder and free space. Newer API calls are looked up at run time
   so the program still loads on Windows 98. */
#include "wb.h"

static int cpuid_supported(void)
{
    unsigned a, b;
    __asm__ volatile(
        "pushfl\n\tpopl %0\n\tmovl %0, %1\n\t"
        "xorl $0x200000, %0\n\tpushl %0\n\tpopfl\n\t"
        "pushfl\n\tpopl %0\n\tpushl %1\n\tpopfl"
        : "=&r"(a), "=&r"(b));
    return ((a ^ b) & 0x200000) != 0;
}

static void cpuid(unsigned leaf, unsigned r[4])
{
    __asm__ volatile("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3]) : "a"(leaf), "c"(0));
}

static unsigned long long rdtsc(void)
{
    unsigned lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
}

static void trim(char *s)
{
    char *p = s, *q;
    while (*p == ' ') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    q = s + strlen(s);
    while (q > s && q[-1] == ' ') *--q = 0;
    /* fold runs of spaces */
    for (p = q = s; *p; p++) if (!(*p == ' ' && q > s && q[-1] == ' ')) *q++ = *p;
    *q = 0;
}

static void cpu_info(SysInfo *si)
{
    unsigned r[4], maxext, fam, model;
    if (!cpuid_supported()) { strcpy(si->cpu_name, "486-class CPU (no CPUID)"); return; }
    si->has_cpuid = 1;
    cpuid(0, r);
    memcpy(si->cpu_vendor, &r[1], 4); memcpy(si->cpu_vendor + 4, &r[3], 4); memcpy(si->cpu_vendor + 8, &r[2], 4);
    si->cpu_vendor[12] = 0;
    cpuid(1, r);
    fam = (r[0] >> 8) & 15; model = (r[0] >> 4) & 15;
    si->has_tsc = (r[3] >> 4) & 1; si->has_mmx = (r[3] >> 23) & 1;
    si->has_sse = (r[3] >> 25) & 1; si->has_sse2 = (r[3] >> 26) & 1;

    cpuid(0x80000000, r); maxext = r[0];
    if (maxext >= 0x80000004 && maxext < 0x8000ffff) {
        unsigned *b = (unsigned *)si->cpu_name;
        cpuid(0x80000002, b); cpuid(0x80000003, b + 4); cpuid(0x80000004, b + 8);
        si->cpu_name[48] = 0;
        trim(si->cpu_name);
        if (si->cpu_name[0]) return;
    }
    if (!strcmp(si->cpu_vendor, "GenuineIntel")) {
        const char *n = "Intel CPU";
        if (fam == 4) n = "Intel 486";
        else if (fam == 5) n = (model == 4 || model == 8) ? "Intel Pentium MMX" : "Intel Pentium";
        else if (fam == 6) {
            if (model == 1) n = "Intel Pentium Pro";
            else if (model == 3 || model == 5) n = "Intel Pentium II";
            else if (model == 6) n = "Intel Celeron (Mendocino)";
            else n = "Intel Pentium III / Celeron";
        }
        strcpy(si->cpu_name, n);
    } else if (!strcmp(si->cpu_vendor, "AuthenticAMD")) {
        const char *n = "AMD CPU";
        if (fam == 4) n = "AMD 486/5x86";
        else if (fam == 5) n = model < 4 ? "AMD K5" : model < 8 ? "AMD K6" : model == 8 ? "AMD K6-2"
                             : model == 9 ? "AMD K6-III" : "AMD K6-2+/III+";
        else if (fam == 6) n = "AMD Athlon/Duron";
        strcpy(si->cpu_name, n);
    } else if (!strcmp(si->cpu_vendor, "CyrixInstead")) strcpy(si->cpu_name, "Cyrix 6x86/MII");
    else if (!strcmp(si->cpu_vendor, "CentaurHauls")) strcpy(si->cpu_name, "IDT WinChip / VIA C3");
    else rt_fmt(si->cpu_name, sizeof si->cpu_name, "%s family %u model %u", si->cpu_vendor, fam, model);
}

static void cpu_mhz(SysInfo *si)
{
    unsigned long long c0, c1;
    double t0, t1;
    HANDLE th = GetCurrentThread();
    int old = GetThreadPriority(th);
    if (!si->has_tsc) return;
    SetThreadPriority(th, THREAD_PRIORITY_TIME_CRITICAL);
    t0 = now_sec(); c0 = rdtsc();
    do t1 = now_sec(); while (t1 - t0 < 0.25);
    c1 = rdtsc();
    SetThreadPriority(th, old);
    si->cpu_mhz = (double)(c1 - c0) / (t1 - t0) / 1e6;
}

static int reg_str(HKEY root, const char *key, const char *val, char *out, DWORD cap)
{
    HKEY h; DWORD type, n = cap; LONG e;
    out[0] = 0;
    if (RegOpenKeyExA(root, key, 0, KEY_READ, &h) != ERROR_SUCCESS) return 0;
    e = RegQueryValueExA(h, val, 0, &type, (BYTE *)out, &n);
    RegCloseKey(h);
    if (e != ERROR_SUCCESS || type != REG_SZ) { out[0] = 0; return 0; }
    out[cap - 1] = 0;
    return 1;
}

static void dx_info(SysInfo *si)
{
    char v[32];
    int a = 0, b = 0, c = 0, d = 0, *f[4], i = 0;
    const char *p;
    f[0] = &a; f[1] = &b; f[2] = &c; f[3] = &d;
    if (!reg_str(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\DirectX", "Version", v, sizeof v)) {
        strcpy(si->dx_text, "not found (DirectX 3 or older)");
        si->dx_ver = 3; return;
    }
    for (p = v; *p && i < 4; p++) {
        if (*p >= '0' && *p <= '9') *f[i] = *f[i] * 10 + (*p - '0');
        else if (*p == '.') i++;
    }
    /* version strings look like 4.09.00.0904 */
    if (b >= 9) {
        si->dx_ver = d >= 904 ? 9.3 : 9.0;
        strcpy(si->dx_text, d >= 904 ? "9.0c" : d >= 902 ? "9.0b" : d >= 901 ? "9.0a" : "9.0");
    } else if (b == 8) { si->dx_ver = c >= 1 ? 8.1 : 8.0; strcpy(si->dx_text, c >= 1 ? "8.1" : "8.0"); }
    else if (b == 7) { si->dx_ver = 7; strcpy(si->dx_text, d >= 716 ? "7.0a" : "7.0"); }
    else if (b == 6) { si->dx_ver = 6; strcpy(si->dx_text, c >= 1 ? "6.1" : "6.0"); }
    else if (b == 5) { si->dx_ver = 5; strcpy(si->dx_text, "5"); }
    else { si->dx_ver = 3; rt_fmt(si->dx_text, sizeof si->dx_text, "%s", v); }
    rt_fmt(si->dx_text + strlen(si->dx_text), 32 - (int)strlen(si->dx_text), " (%s)", v);
}

typedef BOOL (WINAPI *EnumDD)(LPCSTR, DWORD, PDISPLAY_DEVICEA, DWORD);
typedef BOOL (WINAPI *FreeEx)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

static void adapter_info(SysInfo *si)
{
    EnumDD edd = (EnumDD)GetProcAddress(GetModuleHandleA("user32.dll"), "EnumDisplayDevicesA");
    char sysdir[MAX_PATH], path[MAX_PATH];
    if (edd) {
        DISPLAY_DEVICEA dd; DWORD i;
        for (i = 0; i < 8; i++) {
            memset(&dd, 0, sizeof dd); dd.cb = sizeof dd;
            if (!edd(0, i, &dd, 0)) break;
            if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) { rt_fmt(si->adapter, sizeof si->adapter, "%s", dd.DeviceString); break; }
        }
    }
    if (!si->adapter[0])  /* Windows 95/98 keep it here */
        reg_str(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class\\Display\\0000",
                "DriverDesc", si->adapter, sizeof si->adapter);
    trim(si->adapter);

    GetSystemDirectoryA(sysdir, sizeof sysdir);
    rt_fmt(path, sizeof path, "%s\\glide2x.dll", sysdir);
    if (GetFileAttributesA(path) != 0xFFFFFFFF) si->glide = 1;
    rt_fmt(path, sizeof path, "%s\\glide3x.dll", sysdir);
    if (GetFileAttributesA(path) != 0xFFFFFFFF) si->glide = 1;
}

static void os_info(SysInfo *si)
{
    OSVERSIONINFOA v;
    const char *n = "Windows";
    memset(&v, 0, sizeof v); v.dwOSVersionInfoSize = sizeof v;
    GetVersionExA(&v);
    si->os_major = v.dwMajorVersion; si->os_minor = v.dwMinorVersion;
    si->os_is_nt = v.dwPlatformId == VER_PLATFORM_WIN32_NT;
    si->os_build = si->os_is_nt ? (int)v.dwBuildNumber : (int)(v.dwBuildNumber & 0xFFFF);
    if (!si->os_is_nt) {
        if (v.dwMinorVersion == 0) n = "Windows 95";
        else if (v.dwMinorVersion == 10) n = si->os_build >= 2222 ? "Windows 98 SE" : "Windows 98";
        else n = "Windows ME";
    } else if (v.dwMajorVersion == 4) n = "Windows NT 4.0";
    else if (v.dwMajorVersion == 5 && v.dwMinorVersion == 0) n = "Windows 2000";
    else if (v.dwMajorVersion == 5 && v.dwMinorVersion == 1) n = "Windows XP";
    else if (v.dwMajorVersion == 5) n = "Windows XP x64 / Server 2003";
    else if (v.dwMajorVersion == 6 && v.dwMinorVersion == 0) n = "Windows Vista";
    else if (v.dwMajorVersion == 6 && v.dwMinorVersion == 1) n = "Windows 7";
    else n = "Windows 8 or later";
    {
        const char *sp = rt_stristr(v.szCSDVersion, "Service Pack ");
        if (sp) si->os_sp = sp[13] - '0';
    }
    rt_fmt(si->os_name, sizeof si->os_name, "%s %s (%d.%d.%d)", n, v.szCSDVersion,
           si->os_major, si->os_minor, si->os_build);
    trim(si->os_name);
}

void sysinfo_get(SysInfo *si)
{
    MEMORYSTATUS ms;
    SYSTEM_INFO sys;
    FreeEx fx;
    char root[4];

    memset(si, 0, sizeof *si);
    os_info(si);
    dx_info(si);
    cpu_info(si);
    cpu_mhz(si);
    GetSystemInfo(&sys);
    si->cores = sys.dwNumberOfProcessors ? (int)sys.dwNumberOfProcessors : 1;

    ms.dwLength = sizeof ms;
    GlobalMemoryStatus(&ms);
    si->ram_mb = (unsigned)((ms.dwTotalPhys + 512 * 1024) / (1024 * 1024));

    adapter_info(si);

    if (!GetTempPathA(sizeof si->temp_dir, si->temp_dir) || GetFileAttributesA(si->temp_dir) == 0xFFFFFFFF)
        strcpy(si->temp_dir, "C:\\");
    root[0] = si->temp_dir[0]; root[1] = ':'; root[2] = '\\'; root[3] = 0;
    fx = (FreeEx)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA");
    if (fx) {
        ULARGE_INTEGER a, t, f;
        if (fx(root, &a, &t, &f)) si->disk_free_mb = (double)a.QuadPart / (1024.0 * 1024.0);
    } else {
        DWORD spc, bps, fc, tc;
        if (GetDiskFreeSpaceA(root, &spc, &bps, &fc, &tc)) si->disk_free_mb = (double)spc * bps * fc / (1024.0 * 1024.0);
    }
}
