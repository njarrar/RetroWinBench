/* 3D test through OpenGL 1.1, which Windows 98 and XP both ship.
   opengl32.dll is loaded at run time so the program still starts on a PC
   without it. Measures textured fill rate and triangle throughput. */
#include "wb.h"

#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_TRIANGLES        0x0004
#define GL_QUADS            0x0007
#define GL_TEXTURE_2D       0x0DE1
#define GL_DEPTH_TEST       0x0B71
#define GL_CULL_FACE        0x0B44
#define GL_DITHER           0x0BD0
#define GL_VENDOR           0x1F00
#define GL_RENDERER         0x1F01
#define GL_VERSION          0x1F02
#define GL_EXTENSIONS       0x1F03
#define GL_PROJECTION       0x1701
#define GL_MODELVIEW        0x1700
#define GL_RGBA             0x1908
#define GL_RGB              0x1907
#define GL_UNSIGNED_BYTE    0x1401
#define GL_FLOAT            0x1406
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_LINEAR           0x2601
#define GL_VERTEX_ARRAY     0x8074
#define GL_COLOR_ARRAY      0x8076
#define GL_SMOOTH           0x1D01

typedef void (__stdcall *PFV)(void);
static struct {
    HGLRC (__stdcall *CreateContext)(HDC);
    BOOL  (__stdcall *MakeCurrent)(HDC, HGLRC);
    BOOL  (__stdcall *DeleteContext)(HGLRC);
    const char *(__stdcall *GetString)(unsigned);
    void (__stdcall *Viewport)(int, int, int, int);
    void (__stdcall *ClearColor)(float, float, float, float);
    void (__stdcall *Clear)(unsigned);
    void (__stdcall *Enable)(unsigned);
    void (__stdcall *Disable)(unsigned);
    void (__stdcall *Finish)(void);
    void (__stdcall *MatrixMode)(unsigned);
    void (__stdcall *LoadIdentity)(void);
    void (__stdcall *Ortho)(double, double, double, double, double, double);
    void (__stdcall *Begin)(unsigned);
    void (__stdcall *End)(void);
    void (__stdcall *Vertex2f)(float, float);
    void (__stdcall *TexCoord2f)(float, float);
    void (__stdcall *Color3f)(float, float, float);
    void (__stdcall *TexImage2D)(unsigned, int, int, int, int, int, unsigned, unsigned, const void *);
    void (__stdcall *TexParameteri)(unsigned, unsigned, int);
    void (__stdcall *ShadeModel)(unsigned);
    void (__stdcall *EnableClientState)(unsigned);
    void (__stdcall *VertexPointer)(int, unsigned, int, const void *);
    void (__stdcall *ColorPointer)(int, unsigned, int, const void *);
    void (__stdcall *DrawArrays)(unsigned, int, int);
} gl;

static const char *gl_names[] = {
    "wglCreateContext", "wglMakeCurrent", "wglDeleteContext", "glGetString",
    "glViewport", "glClearColor", "glClear", "glEnable", "glDisable", "glFinish",
    "glMatrixMode", "glLoadIdentity", "glOrtho", "glBegin", "glEnd", "glVertex2f",
    "glTexCoord2f", "glColor3f", "glTexImage2D", "glTexParameteri", "glShadeModel",
    "glEnableClientState", "glVertexPointer", "glColorPointer", "glDrawArrays"
};

typedef int  (__stdcall *ChoosePF)(HDC, const PIXELFORMATDESCRIPTOR *);
typedef BOOL (__stdcall *SetPF)(HDC, int, const PIXELFORMATDESCRIPTOR *);
typedef int  (__stdcall *DescribePF)(HDC, int, UINT, PIXELFORMATDESCRIPTOR *);
typedef BOOL (__stdcall *SwapB)(HDC);

#define W 640
#define H 480

static LRESULT CALLBACK gl_wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    return DefWindowProcA(h, m, w, l);
}

static int has_ext(const char *ext, const char *name)
{
    unsigned n = strlen(name);
    const char *s = ext;
    while (s && (s = rt_stristr(s, name))) {
        if ((s == ext || s[-1] == ' ') && (s[n] == ' ' || s[n] == 0)) return 1;
        s += n;
    }
    return 0;
}

static double guess_dx(const char *ext)
{
    if (!ext) return 6;
    if (has_ext(ext, "GL_EXT_gpu_shader4")) return 10;
    if (has_ext(ext, "GL_NV_vertex_program3") || has_ext(ext, "GL_ATI_shader_texture_lod")) return 9.3;
    if (has_ext(ext, "GL_ARB_fragment_program") || has_ext(ext, "GL_ARB_fragment_shader")) return 9;
    if (has_ext(ext, "GL_NV_texture_shader") || has_ext(ext, "GL_ATI_fragment_shader") || has_ext(ext, "GL_ARB_vertex_program")) return 8;
    if (has_ext(ext, "GL_ARB_texture_cube_map") || has_ext(ext, "GL_EXT_texture_cube_map")) return 7;
    return 6;
}

void bench_gpu(GpuResult *r, HINSTANCE inst, ProgressFn p)
{
    HMODULE lib, gdi;
    HWND wnd = 0;
    HDC dc = 0;
    HGLRC rc = 0;
    PIXELFORMATDESCRIPTOR pfd;
    ChoosePF choose; SetPF setpf; SwapB swap;
    WNDCLASSA wc;
    unsigned i;
    int pf, k, batch;
    unsigned total_quads, total_tris;
    double t0, t;
    const char *s;
    float *va = 0; unsigned char *ca = 0;
    int ntri = 20000;

    r->ok = 0;
    p("3D: starting OpenGL...");
    lib = GetModuleHandleA("opengl32.dll");
    if (!lib) lib = LoadLibraryA("opengl32.dll");
    gdi = GetModuleHandleA("gdi32.dll");
    if (!lib) { strcpy(r->err, "opengl32.dll not found (install OpenGL / video driver)"); return; }
    for (i = 0; i < sizeof gl_names / sizeof gl_names[0]; i++) {
        PFV f = (PFV)GetProcAddress(lib, gl_names[i]);
        if (!f) { rt_fmt(r->err, sizeof r->err, "OpenGL is missing %s", gl_names[i]); FreeLibrary(lib); return; }
        ((PFV *)&gl)[i] = f;
    }
    choose = (ChoosePF)GetProcAddress(gdi, "ChoosePixelFormat");
    setpf  = (SetPF)GetProcAddress(gdi, "SetPixelFormat");
    swap   = (SwapB)GetProcAddress(gdi, "SwapBuffers");
    if (!choose || !setpf || !swap) { strcpy(r->err, "this Windows has no OpenGL pixel format support"); FreeLibrary(lib); return; }

    memset(&wc, 0, sizeof wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = gl_wndproc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    wc.lpszClassName = "WBenchGL";
    RegisterClassA(&wc);
    {
        RECT rr = { 0, 0, W, H };
        AdjustWindowRect(&rr, WS_POPUP | WS_CAPTION, FALSE);
        wnd = CreateWindowA("WBenchGL", "WinBench 3D test", WS_POPUP | WS_CAPTION | WS_VISIBLE,
                            40, 40, rr.right - rr.left, rr.bottom - rr.top, 0, 0, inst, 0);
    }
    if (!wnd) { strcpy(r->err, "cannot open test window"); goto done; }
    dc = GetDC(wnd);

    memset(&pfd, 0, sizeof pfd);
    pfd.nSize = sizeof pfd; pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32; pfd.cDepthBits = 16;
    pf = choose(dc, &pfd);
    if (!pf || !setpf(dc, pf, &pfd)) { strcpy(r->err, "no usable OpenGL pixel format"); goto done; }
    rc = gl.CreateContext(dc);
    if (!rc || !gl.MakeCurrent(dc, rc)) { strcpy(r->err, "cannot create OpenGL context"); goto done; }

    s = gl.GetString(GL_VENDOR);   if (s) rt_fmt(r->vendor, sizeof r->vendor, "%s", s);
    s = gl.GetString(GL_RENDERER); if (s) rt_fmt(r->renderer, sizeof r->renderer, "%s", s);
    s = gl.GetString(GL_VERSION);  if (s) rt_fmt(r->version, sizeof r->version, "%s", s);
    r->dx_guess = guess_dx(gl.GetString(GL_EXTENSIONS));
    r->software = rt_stristr(r->renderer, "GDI Generic") != 0 || rt_stristr(r->renderer, "llvmpipe") != 0
               || rt_stristr(r->renderer, "softpipe") != 0 || rt_stristr(r->renderer, "Software") != 0;
    if (r->software) r->dx_guess = 0;

    gl.Viewport(0, 0, W, H);
    gl.MatrixMode(GL_PROJECTION); gl.LoadIdentity(); gl.Ortho(0, W, H, 0, -1, 1);
    gl.MatrixMode(GL_MODELVIEW); gl.LoadIdentity();
    gl.Disable(GL_DEPTH_TEST); gl.Disable(GL_CULL_FACE); gl.Disable(GL_DITHER);
    gl.ClearColor(0, 0, 0, 0);

    /* ---- fill rate: full-window textured quads ---- */
    p("3D: fill rate...");
    va = rt_alloc(ntri * 3 * 2 * sizeof(float));
    ca = rt_alloc(ntri * 3 * 4);
    if (!va || !ca) { strcpy(r->err, "out of memory"); goto done; }
    {
        unsigned char *tex = (unsigned char *)va;
        for (i = 0; i < 128 * 128 * 4; i++) tex[i] = (unsigned char)((i * 7) ^ (i >> 9));
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    }
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.Enable(GL_TEXTURE_2D);
    gl.Color3f(1, 1, 1);
    k = 4; total_quads = 0;
    gl.Clear(GL_COLOR_BUFFER_BIT); gl.Finish();
    t0 = now_sec();
    do {
        int q;
        gl.Begin(GL_QUADS);
        for (q = 0; q < k; q++) {
            float o = (float)(q & 7) * 0.01f;
            gl.TexCoord2f(o, o);        gl.Vertex2f(0, 0);
            gl.TexCoord2f(4 + o, o);    gl.Vertex2f(W, 0);
            gl.TexCoord2f(4 + o, 3 + o); gl.Vertex2f(W, H);
            gl.TexCoord2f(o, 3 + o);    gl.Vertex2f(0, H);
        }
        gl.End();
        gl.Finish();
        total_quads += (unsigned)k;
        t = now_sec() - t0;
        if (k == 4 && t < 0.02) k = 32;   /* fast card: draw more per batch */
        p(0);
    } while (t < 2.0);
    r->fill_mpix = (double)total_quads * W * H / t / 1e6;
    swap(dc);
    gl.Disable(GL_TEXTURE_2D);

    /* ---- triangles: many small colored triangles from a vertex array ---- */
    p("3D: triangle rate...");
    for (i = 0; i < (unsigned)ntri; i++) {
        float x = (float)(rt_rand() % (W - 8)), y = (float)(rt_rand() % (H - 8));
        float *v = va + i * 6;
        v[0] = x; v[1] = y; v[2] = x + 6; v[3] = y; v[4] = x; v[5] = y + 6;
    }
    for (i = 0; i < (unsigned)ntri * 3; i++) ((unsigned *)ca)[i] = rt_rand();
    gl.ShadeModel(GL_SMOOTH);
    gl.EnableClientState(GL_VERTEX_ARRAY);
    gl.EnableClientState(GL_COLOR_ARRAY);
    gl.VertexPointer(2, GL_FLOAT, 0, va);
    gl.ColorPointer(4, GL_UNSIGNED_BYTE, 0, ca);
    batch = 2500; total_tris = 0; t0 = now_sec();
    do {
        gl.DrawArrays(GL_TRIANGLES, 0, batch * 3);
        gl.Finish();
        total_tris += (unsigned)batch;
        t = now_sec() - t0;
        if (batch < ntri && t < 0.05) batch = ntri;
        p(0);
    } while (t < 2.0);
    r->mtris = (double)total_tris / t / 1e6;
    swap(dc);
    r->ok = 1;

done:
    if (rc) { gl.MakeCurrent(0, 0); gl.DeleteContext(rc); }
    if (dc) ReleaseDC(wnd, dc);
    if (wnd) DestroyWindow(wnd);
    rt_free(va); rt_free(ca);
    /* opengl32 stays loaded: some old drivers crash when unloaded */
}
