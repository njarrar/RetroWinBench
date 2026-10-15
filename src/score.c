/* Turn raw results into scores, compare them with period hardware and
   build the text report, including the list of games. */
#include "wb.h"

static double clampd(double v, double lo) { return v < lo ? lo : v; }
static double ratio(double v, double ref) { return clampd(v, 0.0001) / ref; }

static double os_level(const SysInfo *si)
{
    if (!si->os_is_nt) return si->os_minor == 0 ? 0 : 1;
    if (si->os_major < 5) return 1.5;               /* NT 4 */
    if (si->os_major == 5 && si->os_minor == 0) return 2;
    if (si->os_major == 5) return si->os_sp >= 2 || si->os_minor > 1 ? 4 : 3;
    return 5;
}

static int find_gpu(const char *name)
{
    int i;
    if (!name || !name[0]) return -1;
    for (i = 0; i < g_ngpus; i++)
        if (rt_stristr(name, g_gpus[i].match)) return i;
    return -1;
}

void score_compute(Results *r)
{
    double c, m, d, g;
    r->s_cpu  = 100.0 * rt_sqrt(ratio(r->cpu.dmips, REF_DMIPS) * ratio(r->cpu.mwips, REF_MWIPS));
    r->s_mem  = 100.0 * rt_pow(ratio(r->mem.read_mbs, REF_MEM_READ) * ratio(r->mem.write_mbs, REF_MEM_WRITE)
                               * ratio(r->mem.copy_mbs, REF_MEM_COPY), 1.0 / 3.0);
    r->s_disk = r->disk.ok ? 100.0 * rt_pow(ratio(r->disk.read_mbs, REF_DISK_READ) * ratio(r->disk.write_mbs, REF_DISK_WRITE)
                                            * ratio(REF_DISK_ACCESS, clampd(r->disk.access_ms, 0.01)), 1.0 / 3.0) : 0;
    r->s_gpu  = r->gpu.ok ? 100.0 * rt_pow(ratio(r->gpu.fill_mpix, REF_FILL), 0.6) * rt_pow(ratio(r->gpu.mtris, REF_TRIS), 0.4) : 0;

    /* which card is it? OpenGL name first, then the Windows adapter name */
    r->gpu_row = -1;
    if (r->gpu.ok && !r->gpu.software) r->gpu_row = find_gpu(r->gpu.renderer);
    if (r->gpu_row < 0) r->gpu_row = find_gpu(r->si.adapter);
    if (r->gpu_row < 0 && r->si.glide) r->gpu_row = find_gpu("voodoo2");   /* Glide add-on card, guess Voodoo2 */

    if (r->gpu_row >= 0) {
        r->gpu_game_index = g_gpus[r->gpu_row].index;
        r->gpu_dx = g_gpus[r->gpu_row].dx10 / 10.0;
    } else if (r->gpu.ok && !r->gpu.software) {
        r->gpu_game_index = r->s_gpu;
        r->gpu_dx = r->gpu.dx_guess;
    } else {
        r->gpu_game_index = 0;
        r->gpu_dx = 0;
    }
    /* a card the driver runs only in software still counts as no 3D */
    if (r->s_gpu <= 0 && r->gpu_row >= 0) r->s_gpu = g_gpus[r->gpu_row].index;

    c = clampd(r->s_cpu, 1); m = clampd(r->s_mem, 1); d = clampd(r->s_disk, 1); g = clampd(r->s_gpu, 1);
    r->s_total = rt_exp(0.35 * rt_log(c) + 0.35 * rt_log(g) + 0.15 * rt_log(m) + 0.15 * rt_log(d));
}

/* ---------------- report ---------------- */
typedef struct { char *p; int left; } Buf;

static void out(Buf *b, const char *f, ...)
{
    va_list ap; int n;
    if (b->left <= 1) return;
    va_start(ap, f);
    n = rt_vfmt(b->p, b->left, f, ap);
    va_end(ap);
    if (n >= b->left) n = b->left - 1;
    if (n > 0) { b->p += n; b->left -= n; }
}

static double year_score(const YearRow *y)
{
    return rt_exp(0.35 * rt_log(y->cpu) + 0.35 * rt_log(y->gpu) + 0.15 * rt_log(y->ram) + 0.15 * rt_log(y->disk));
}

static int nearest_cpu(double idx)
{
    int i, best = 0; double bd = 1e9;
    idx = clampd(idx, 1);
    for (i = 0; i < g_ncpus; i++) {
        double v = g_cpus[i].index;
        double dd = v > idx ? v / idx : idx / v;
        if (dd < bd) { bd = dd; best = i; }
    }
    return best;
}

static int nearest_gpu(double idx)
{
    int i, best = 0; double bd = 1e9;
    idx = clampd(idx, 1);
    for (i = 0; i < g_ngpus; i++) {
        double v = g_gpus[i].index;
        double dd = v > idx ? v / idx : idx / v;
        if (dd < bd) { bd = dd; best = i; }
    }
    return best;
}

static int pct_cpu(double idx) { int i, n = 0; for (i = 0; i < g_ncpus; i++) if (g_cpus[i].index < idx) n++; return n * 100 / g_ncpus; }
static int pct_gpu(double idx) { int i, n = 0; for (i = 0; i < g_ngpus; i++) if (g_gpus[i].index < idx) n++; return n * 100 / g_ngpus; }

static const char *os_need[] = { "Windows 95", "Windows 98", "Windows 2000/XP", "Windows XP", "Windows XP SP2" };

/* returns 2 = meets recommended, 1 = meets minimum, 0 = not; fills why */
static int check_game(const Results *r, const GameRow *gm, char *why, int cap)
{
    double osl = os_level(&r->si);
    double cpu = r->s_cpu, gi = r->gpu_game_index, gdx = r->gpu_dx;
    unsigned ram = r->si.ram_mb;
    Buf b; int ok = 1;
    b.p = why; b.left = cap; if (why) why[0] = 0;

    if (osl < gm->os || (gm->os == 2 && osl < 2)) { out(&b, "needs %s; ", os_need[(int)gm->os]); ok = 0; }
    if (gm->min_dx10 >= 100 && osl < 5) { out(&b, "needs DirectX 10 (Vista); "); ok = 0; }
    if (cpu < g_cpus[gm->min_cpu].index * 0.95) { out(&b, "CPU below %s; ", g_cpus[gm->min_cpu].name); ok = 0; }
    if (ram + 4 < (unsigned)gm->min_ram) { out(&b, "needs %d MB RAM; ", gm->min_ram); ok = 0; }
    if (gm->min_gpu >= 0) {
        const GpuRow *need = &g_gpus[gm->min_gpu];
        if (gi <= 0) { out(&b, "needs a 3D card (%s or better); ", need->name); ok = 0; }
        else if (gi < need->index * 0.9) { out(&b, "3D card below %s; ", need->name); ok = 0; }
        /* the card must have the features of the card the box names; a box that
           asks for Shader Model 3 (9.0c level) or DirectX 10 raises the bar */
        int need_dx = need->dx10;
        if (gm->min_dx10 >= 93 && gm->min_dx10 > need_dx) need_dx = gm->min_dx10;
        if (gi > 0 && gi >= need->index * 0.9 && need_dx >= 80 && gdx * 10 + 0.5 < need_dx) {
            out(&b, "card lacks DirectX %d.%d features; ", need_dx / 10, need_dx % 10); ok = 0;
        }
    }
    if (!ok) { if (why && b.p > why + 1) b.p[-2] = 0; return 0; }
    if (gm->min_dx10 > 0 && r->si.dx_ver * 10 + 0.5 < gm->min_dx10 && gm->min_dx10 < 100)
        out(&b, "install DirectX %d.%d; ", gm->min_dx10 / 10, gm->min_dx10 % 10);
    if (r->si.disk_free_mb > 0 && r->si.disk_free_mb < gm->disk_mb) out(&b, "free %d MB disk; ", gm->disk_mb);

    {
        int slow_cpu = cpu < g_cpus[gm->rec_cpu].index * 0.95;
        int low_ram = ram + 4 < (unsigned)gm->rec_ram;
        int slow_gpu = gm->rec_gpu >= 0 && gi < g_gpus[gm->rec_gpu].index * 0.9;
        if (!slow_cpu && !low_ram && !slow_gpu) { if (why && b.p > why + 1) b.p[-2] = 0; return 2; }
        out(&b, "below recommended:%s%s%s; ", slow_cpu ? " CPU" : "", slow_gpu ? " 3D" : "", low_ram ? " RAM" : "");
        if (why && b.p > why + 1) b.p[-2] = 0;
        return 1;
    }
}

static void games_section(Buf *b, const Results *r, const unsigned char *tiers, int count, int want, const char *title)
{
    int i; char why[160];
    out(b, "\r\n%s (%d)\r\n", title, count);
    for (i = 0; i < g_ngames; i++) {
        const GameRow *gm = &g_games[i];
        if (tiers[i] != want) continue;
        check_game(r, gm, why, sizeof why);
        out(b, "  %-38.38s %d%s", gm->name, gm->year, gm->est ? "*" : " ");
        if (why[0]) out(b, "  %s", why);
        out(b, "\r\n");
    }
}

int report_build(Results *r, char *outbuf, int cap)
{
    Buf b; int i, best_year = -1, counts[3] = { 0, 0, 0 };
    unsigned char tiers[256];
    double ys[16];
    const SysInfo *si = &r->si;
    b.p = outbuf; b.left = cap;

    out(&b, "RetroWinBench %s\r\n", WB_VERSION);
    out(&b, "=====================================================================\r\n");
    out(&b, "Windows   %s, DirectX %s\r\n", si->os_name, si->dx_text);
    out(&b, "CPU       %s", si->cpu_name);
    if (si->cpu_mhz > 0) out(&b, ", %d MHz", (int)(si->cpu_mhz + 0.5));
    if (si->cores > 1) out(&b, ", %d threads", si->cores);
    out(&b, "%s%s\r\n", si->has_sse2 ? ", SSE2" : si->has_sse ? ", SSE" : "", si->has_mmx && !si->has_sse ? ", MMX" : "");
    out(&b, "RAM       %u MB\r\n", si->ram_mb);
    out(&b, "Video     %s\r\n", si->adapter[0] ? si->adapter : "(unknown)");
    if (r->gpu.ok) out(&b, "OpenGL    %s / %s / %s%s\r\n", r->gpu.vendor, r->gpu.renderer, r->gpu.version,
                       r->gpu.software ? "  (software, no 3D chip)" : "");
    else out(&b, "OpenGL    not available: %s\r\n", r->gpu.err);
    if (r->gpu_row >= 0) out(&b, "3D card   %s (%d MB class, DirectX %d.%d level)%s\r\n", g_gpus[r->gpu_row].name,
                             g_gpus[r->gpu_row].vram, g_gpus[r->gpu_row].dx10 / 10, g_gpus[r->gpu_row].dx10 % 10,
                             si->glide && !rt_stristr(si->adapter, "voodoo") ? " (3dfx Glide driver found)" : "");
    out(&b, "Disk      %s, %d MB free\r\n", si->temp_dir, (int)si->disk_free_mb);

    out(&b, "\r\nRESULTS                        Yours        Reference PC\r\n");
    out(&b, "CPU integer (Dhrystone 2.1)  %8.0f DMIPS  %6.0f\r\n", r->cpu.dmips, REF_DMIPS);
    out(&b, "CPU floating (Whetstone)     %8.0f MWIPS  %6.0f\r\n", r->cpu.mwips, REF_MWIPS);
    out(&b, "RAM read                     %8.0f MB/s   %6.0f\r\n", r->mem.read_mbs, REF_MEM_READ);
    out(&b, "RAM write                    %8.0f MB/s   %6.0f\r\n", r->mem.write_mbs, REF_MEM_WRITE);
    out(&b, "RAM copy                     %8.0f MB/s   %6.0f\r\n", r->mem.copy_mbs, REF_MEM_COPY);
    if (r->disk.ok) {
        out(&b, "Disk sequential read         %8.1f MB/s   %6.1f\r\n", r->disk.read_mbs, REF_DISK_READ);
        out(&b, "Disk sequential write        %8.1f MB/s   %6.1f\r\n", r->disk.write_mbs, REF_DISK_WRITE);
        out(&b, "Disk random 4 KB read        %8.1f ms     %6.1f\r\n", r->disk.access_ms, REF_DISK_ACCESS);
        if (!r->disk.unbuffered) out(&b, "  (Windows would not bypass its file cache; used a %u MB file)\r\n", r->disk.file_mb);
        if (r->disk.access_ms < 0.05) out(&b, "  (reads came from the Windows cache, so the disk numbers are too high)\r\n");
    } else out(&b, "Disk                         skipped: %s\r\n", r->disk.err);
    if (r->gpu.ok) {
        out(&b, "3D fill rate (textured)      %8.1f Mpix/s %6.0f\r\n", r->gpu.fill_mpix, REF_FILL);
        out(&b, "3D triangles                 %8.2f M/s    %6.1f\r\n", r->gpu.mtris, REF_TRIS);
    }

    out(&b, "\r\nSCORES (reference PC = 100; %s)\r\n", REF_DESC);
    out(&b, "  CPU   %6.0f   close to a %s\r\n", r->s_cpu, g_cpus[nearest_cpu(r->s_cpu)].name);
    out(&b, "  3D    %6.0f", r->s_gpu);
    if (r->gpu_row >= 0) out(&b, "   table rating for your card: %d", g_gpus[r->gpu_row].index);
    else if (r->gpu_game_index > 0) out(&b, "   close to a %s", g_gpus[nearest_gpu(r->gpu_game_index)].name);
    else out(&b, "   no 3D acceleration found");
    out(&b, "\r\n  RAM   %6.0f\r\n  Disk  %6.0f\r\n", r->s_mem, r->s_disk);
    out(&b, "  -------------\r\n  TOTAL %6.0f\r\n", r->s_total);

    out(&b, "\r\nHow this PC compares\r\n");
    out(&b, "  CPU is faster than %d%% of the %d CPUs in the built-in table.\r\n", pct_cpu(r->s_cpu), g_ncpus);
    if (r->gpu_game_index > 0)
        out(&b, "  3D card is faster than %d%% of the %d cards in the table.\r\n", pct_gpu(r->gpu_game_index), g_ngpus);

    for (i = 0; i < g_nyears; i++) {
        ys[i] = year_score(&g_years[i]);
        if (ys[i] <= r->s_total) best_year = i;
    }
    out(&b, "\r\nTypical new mid-range PC by year\r\n");
    for (i = 0; i < g_nyears; i++) {
        out(&b, "  %d  %6.0f  %s%s\r\n", g_years[i].year, ys[i], g_years[i].desc,
            i == best_year ? "   <== this PC" : "");
    }
    if (best_year < 0) out(&b, "  This PC scores below a typical %d PC.\r\n", g_years[0].year);
    else out(&b, "  This PC matches or beats a typical new PC from %d.\r\n", g_years[best_year].year);

    for (i = 0; i < g_ngames; i++) {
        int t = check_game(r, &g_games[i], 0, 0);
        tiers[i] = (unsigned char)t;
        counts[t]++;
    }
    out(&b, "\r\nGAMES  (* = some specs in the table are estimates)\r\n");
    games_section(&b, r, tiers, counts[2], 2, "Runs well: meets the recommended specs");
    games_section(&b, r, tiers, counts[1], 1, "Runs: meets the minimum specs, use low settings");
    games_section(&b, r, tiers, counts[0], 0, "Will not run well: below the minimum");

    out(&b, "\r\nReference figures and game specs come from published benchmarks and\r\n"
            "box requirements; many are estimates. See README.txt for sources.\r\n");
    return cap - b.left;
}
