/* Tiny runtime so the program needs no C runtime DLL.
   Math uses x87 instructions, which every 32-bit x86 CPU with an FPU has. */
#include "wb.h"

void *memset(void *d, int c, unsigned n)
{
    unsigned char *p = d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}

void *memcpy(void *d, const void *s, unsigned n)
{
    unsigned char *p = d; const unsigned char *q = s;
    while (n--) *p++ = *q++;
    return d;
}

void *memmove(void *d, const void *s, unsigned n)
{
    unsigned char *p = d; const unsigned char *q = s;
    if (p < q) while (n--) *p++ = *q++;
    else { p += n; q += n; while (n--) *--p = *--q; }
    return d;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *d, const char *s)
{
    char *r = d;
    while ((*d++ = *s++)) ;
    return r;
}

unsigned strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (unsigned)(p - s);
}

static int lc(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

int rt_strncasecmp(const char *a, const char *b, unsigned n)
{
    while (n--) {
        int x = lc((unsigned char)*a++), y = lc((unsigned char)*b++);
        if (x != y) return x - y;
        if (!x) return 0;
    }
    return 0;
}

const char *rt_stristr(const char *hay, const char *needle)
{
    unsigned n = strlen(needle);
    if (!n) return hay;
    for (; *hay; hay++)
        if (rt_strncasecmp(hay, needle, n) == 0) return hay;
    return 0;
}

void rt_lower(char *s) { for (; *s; s++) *s = (char)lc((unsigned char)*s); }

/* ---- formatting: %d %u %x %s %c %f with width, precision and '-' ---- */
typedef struct { char *p; int left; int n; } Out;

static void put(Out *o, char c)
{
    if (o->left > 1) { *o->p++ = c; o->left--; }
    o->n++;
}

static void put_field(Out *o, const char *s, int len, int width, int left)
{
    int pad = width > len ? width - len : 0;
    if (!left) while (pad--) put(o, ' ');
    while (len--) put(o, *s++);
    if (left) while (pad-- > 0) put(o, ' ');
}

static int utoa(char *buf, unsigned long long v, unsigned base)
{
    char t[24]; int n = 0, i = 0;
    do { unsigned d = (unsigned)(v % base); t[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v /= base; } while (v);
    while (n) buf[i++] = t[--n];
    return i;
}

static int dtoa(char *buf, double v, int prec)
{
    int i = 0, k;
    unsigned long long ip, fp, scale = 1;
    if (v != v) { buf[0] = 'n'; buf[1] = 'a'; buf[2] = 'n'; return 3; }
    if (v < 0) { buf[i++] = '-'; v = -v; }
    if (v > 1e18) { buf[i++] = '>'; buf[i++] = '1'; buf[i++] = 'e'; buf[i++] = '1'; buf[i++] = '8'; return i; }
    if (prec > 9) prec = 9;
    for (k = 0; k < prec; k++) scale *= 10;
    v = v * (double)scale + 0.5;
    ip = (unsigned long long)v;
    fp = ip % scale; ip /= scale;
    i += utoa(buf + i, ip, 10);
    if (prec) {
        char t[12]; int n = utoa(t, fp, 10);
        buf[i++] = '.';
        for (k = n; k < prec; k++) buf[i++] = '0';
        for (k = 0; k < n; k++) buf[i++] = t[k];
    }
    return i;
}

int rt_vfmt(char *out, int cap, const char *f, va_list ap)
{
    Out o; o.p = out; o.left = cap; o.n = 0;
    while (*f) {
        int left = 0, width = 0, prec = -1;
        char buf[48]; int len;
        if (*f != '%') { put(&o, *f++); continue; }
        f++;
        if (*f == '-') { left = 1; f++; }
        while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        if (*f == '.') { f++; prec = 0; while (*f >= '0' && *f <= '9') prec = prec * 10 + (*f++ - '0'); }
        switch (*f) {
        case 'd': { int v = va_arg(ap, int); len = 0;
                    if (v < 0) { buf[len++] = '-'; len += utoa(buf + len, (unsigned)(-v), 10); }
                    else len = utoa(buf, (unsigned)v, 10);
                    put_field(&o, buf, len, width, left); break; }
        case 'u': len = utoa(buf, va_arg(ap, unsigned), 10); put_field(&o, buf, len, width, left); break;
        case 'x': len = utoa(buf, va_arg(ap, unsigned), 16); put_field(&o, buf, len, width, left); break;
        case 'c': buf[0] = (char)va_arg(ap, int); put_field(&o, buf, 1, width, left); break;
        case 's': { const char *s = va_arg(ap, const char *); if (!s) s = "";
                    len = (int)strlen(s); if (prec >= 0 && len > prec) len = prec;
                    put_field(&o, s, len, width, left); break; }
        case 'f': len = dtoa(buf, va_arg(ap, double), prec < 0 ? 2 : prec); put_field(&o, buf, len, width, left); break;
        case '%': put(&o, '%'); break;
        default: put(&o, '%'); if (*f) put(&o, *f); break;
        }
        if (*f) f++;
    }
    if (cap > 0) *o.p = 0;
    return o.n;
}

int rt_fmt(char *out, int cap, const char *f, ...)
{
    va_list ap; int n;
    va_start(ap, f); n = rt_vfmt(out, cap, f, ap); va_end(ap);
    return n;
}

/* ---- x87 math ---- */
double rt_sqrt(double x) { double r; __asm__("fsqrt" : "=t"(r) : "0"(x)); return r; }
double rt_sin(double x)  { double r; __asm__("fsin"  : "=t"(r) : "0"(x)); return r; }
double rt_cos(double x)  { double r; __asm__("fcos"  : "=t"(r) : "0"(x)); return r; }
double rt_atan(double x) { double r; __asm__("fld1\n\tfpatan" : "=t"(r) : "0"(x)); return r; }
double rt_log(double x)
{
    double r;
    __asm__("fldln2\n\tfxch\n\tfyl2x" : "=t"(r) : "0"(x));
    return r;
}
double rt_exp(double x)
{
    double r;
    __asm__("fldl2e\n\t"
            "fmulp\n\t"            /* st0 = x*log2(e) */
            "fld %%st(0)\n\t"
            "frndint\n\t"          /* st0 = n, st1 = y */
            "fxch\n\t"             /* st0 = y, st1 = n */
            "fsub %%st(1), %%st\n\t" /* st0 = y - n */
            "f2xm1\n\t"
            "fld1\n\t"
            "faddp\n\t"            /* st0 = 2^(y-n), st1 = n */
            "fscale\n\t"
            "fstp %%st(1)"
            : "=t"(r) : "0"(x));
    return r;
}
double rt_pow(double x, double y) { return x <= 0 ? 0 : rt_exp(y * rt_log(x)); }

void *rt_alloc(unsigned n) { return VirtualAlloc(0, n, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); }
void rt_free(void *p) { if (p) VirtualFree(p, 0, MEM_RELEASE); }

static unsigned rng = 2463534242u;
unsigned rt_rand(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

static LARGE_INTEGER qpf;
double now_sec(void)
{
    LARGE_INTEGER c;
    if (!qpf.QuadPart) {
        if (!QueryPerformanceFrequency(&qpf) || !qpf.QuadPart) qpf.QuadPart = 1;
    }
    if (qpf.QuadPart == 1) return GetTickCount() / 1000.0;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)qpf.QuadPart;
}
