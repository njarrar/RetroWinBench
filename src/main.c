/* RetroWinBench: window, buttons and the order the tests run in.
   /auto tests, saves the report next to the program and exits;
   /run starts the test at once and leaves the window open. */
#include "wb.h"

#define ID_RUN  101
#define ID_SAVE 102
#define ID_TEXT 103

static HINSTANCE g_inst;
static HWND g_wnd, g_run, g_save, g_status, g_text;
static HFONT g_font;
static Results g_res;
static int g_running, g_done, g_auto, g_run_now;
static char g_report[60000];   /* Windows 9x edit boxes hold at most 64 KB */

static void pump(void)
{
    MSG m;
    while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); return; }
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
}

static void progress(const char *msg)
{
    if (msg && g_status) SetWindowTextA(g_status, msg);
    pump();
}

static void exe_dir(char *out, int cap)
{
    char *s;
    GetModuleFileNameA(0, out, cap);
    s = out + strlen(out);
    while (s > out && s[-1] != '\\') s--;
    *s = 0;
}

static int save_report(char *path_out, int cap)
{
    char path[MAX_PATH];
    HANDLE h; DWORD n;
    exe_dir(path, sizeof path);
    rt_fmt(path + strlen(path), (int)(sizeof path - strlen(path)), "winbench-report.txt");
    h = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) {   /* program on a CD or read-only folder */
        GetTempPathA(sizeof path, path);
        rt_fmt(path + strlen(path), (int)(sizeof path - strlen(path)), "winbench-report.txt");
        h = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (h == INVALID_HANDLE_VALUE) return 0;
    }
    WriteFile(h, g_report, strlen(g_report), &n, 0);
    CloseHandle(h);
    rt_fmt(path_out, cap, "%s", path);
    return 1;
}

static void show_intro(void)
{
    SysInfo *si = &g_res.si;
    rt_fmt(g_report, sizeof g_report,
        "RetroWinBench %s\r\n\r\n"
        "Windows   %s, DirectX %s\r\n"
        "CPU       %s, %d MHz\r\n"
        "RAM       %u MB\r\n"
        "Video     %s\r\n"
        "Disk      %s, %d MB free\r\n\r\n"
        "Press Run benchmark. The test takes one to three minutes:\r\n"
        "  CPU     Dhrystone 2.1 and Whetstone, about 2 seconds each\r\n"
        "  RAM     read, write and copy speed\r\n"
        "  Disk    writes and reads a temporary file in the folder above, then deletes it\r\n"
        "  3D      opens a 640x480 OpenGL window for about 4 seconds\r\n\r\n"
        "Close other programs first for steady numbers.\r\n"
        "Built-in table: %d CPUs, %d 3D cards, %d games.\r\n",
        WB_VERSION, si->os_name, si->dx_text, si->cpu_name, (int)(si->cpu_mhz + 0.5), si->ram_mb,
        si->adapter[0] ? si->adapter : "(unknown)", si->temp_dir, (int)si->disk_free_mb,
        g_ncpus, g_ngpus, g_ngames);
    SetWindowTextA(g_text, g_report);
}

static void run_all(void)
{
    char path[MAX_PATH];
    HANDLE th = GetCurrentThread();
    int pri = GetThreadPriority(th);
    g_running = 1;
    EnableWindow(g_run, FALSE); EnableWindow(g_save, FALSE);
    SetWindowTextA(g_text, "Testing... please do not use the PC until it finishes.\r\n");

    SetThreadPriority(th, THREAD_PRIORITY_ABOVE_NORMAL);
    bench_cpu(&g_res.cpu, progress);
    bench_mem(&g_res.mem, g_res.si.ram_mb, progress);
    bench_disk(&g_res.disk, g_res.si.temp_dir, g_res.si.disk_free_mb, g_res.si.ram_mb, progress);
    SetThreadPriority(th, pri);
    bench_gpu(&g_res.gpu, g_inst, progress);

    progress("Scoring...");
    score_compute(&g_res);
    report_build(&g_res, g_report, sizeof g_report);
    SetWindowTextA(g_text, g_report);

    if (save_report(path, sizeof path)) {
        char msg[MAX_PATH + 40];
        rt_fmt(msg, sizeof msg, "Done. Total score %d. Report saved to %s", (int)(g_res.s_total + 0.5), path);
        SetWindowTextA(g_status, msg);
    } else SetWindowTextA(g_status, "Done. Could not save the report file.");
    EnableWindow(g_run, TRUE); EnableWindow(g_save, TRUE);
    g_running = 0; g_done = 1;
}

static void layout(int w, int h)
{
    MoveWindow(g_run, 8, 8, 140, 26, TRUE);
    MoveWindow(g_save, 156, 8, 140, 26, TRUE);
    MoveWindow(g_status, 306, 12, w - 314, 20, TRUE);
    MoveWindow(g_text, 8, 42, w - 16, h - 50, TRUE);
}

static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        g_run = CreateWindowA("BUTTON", "Run benchmark", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 0, 0, h, (HMENU)ID_RUN, g_inst, 0);
        g_save = CreateWindowA("BUTTON", "Save report", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, h, (HMENU)ID_SAVE, g_inst, 0);
        g_status = CreateWindowA("STATIC", "Ready.", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, h, 0, g_inst, 0);
        g_text = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                                 ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, h, (HMENU)ID_TEXT, g_inst, 0);
        g_font = CreateFontA(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
        if (!g_font) g_font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
        SendMessageA(g_text, WM_SETFONT, (WPARAM)g_font, 0);
        SendMessageA(g_run, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        SendMessageA(g_save, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        SendMessageA(g_status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        EnableWindow(g_save, FALSE);
        return 0;
    case WM_SIZE:
        layout(LOWORD(l), HIWORD(l));
        return 0;
    case WM_COMMAND:
        if (LOWORD(w) == ID_RUN && !g_running) run_all();
        else if (LOWORD(w) == ID_SAVE && g_done) {
            char path[MAX_PATH], msg[MAX_PATH + 32];
            if (save_report(path, sizeof path)) rt_fmt(msg, sizeof msg, "Saved to %s", path);
            else strcpy(msg, "Could not save the report.");
            SetWindowTextA(g_status, msg);
        }
        return 0;
    case WM_CLOSE:
        if (g_running) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

static int win_main(HINSTANCE inst, const char *cmd)
{
    WNDCLASSA wc;
    MSG msg;

    g_inst = inst;
    g_auto = rt_stristr(cmd, "/auto") != 0;
    g_run_now = rt_stristr(cmd, "/run") != 0;
    sysinfo_get(&g_res.si);

    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = wndproc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconA(inst, (LPCSTR)1);
    if (!wc.hIcon) wc.hIcon = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "WBenchMain";
    if (!RegisterClassA(&wc)) return 1;
    g_wnd = CreateWindowA("WBenchMain", "RetroWinBench " WB_VERSION, WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, 760, 560, 0, 0, inst, 0);
    if (!g_wnd) return 1;
    ShowWindow(g_wnd, SW_SHOWNORMAL);
    UpdateWindow(g_wnd);
    show_intro();

    if (g_auto || g_run_now) run_all();
    if (g_auto) DestroyWindow(g_wnd);
    while (GetMessageA(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

/* entry point: no C runtime, so no WinMain wrapper */
void __stdcall wb_start(void)
{
    ExitProcess(win_main(GetModuleHandleA(0), GetCommandLineA()));
}
