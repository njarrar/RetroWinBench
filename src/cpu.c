/* CPU tests: Dhrystone 2.1 (integer) and Whetstone (floating point).
   Both are the classic programs, so results line up with published
   figures for old PCs. Each runs about two seconds. */
#include "wb.h"

#define NOINL __attribute__((noinline))

/* ================= Dhrystone 2.1 ================= */
typedef enum { Ident_1, Ident_2, Ident_3, Ident_4, Ident_5 } Enumeration;
typedef int One_Thirty, One_Fifty, Boolean;
typedef char Capital_Letter;
typedef char Str_30[31];
typedef int Arr_1_Dim[50];
typedef int Arr_2_Dim[50][50];

typedef struct record {
    struct record *Ptr_Comp;
    Enumeration Discr;
    union {
        struct { Enumeration Enum_Comp; int Int_Comp; char Str_Comp[31]; } var_1;
        struct { Enumeration E_Comp_2; char Str_2_Comp[31]; } var_2;
        struct { char Ch_1_Comp; char Ch_2_Comp; } var_3;
    } variant;
} Rec_Type, *Rec_Pointer;

static Rec_Pointer Ptr_Glob, Next_Ptr_Glob;
static int Int_Glob;
static Boolean Bool_Glob;
static char Ch_1_Glob, Ch_2_Glob;
static int Arr_1_Glob[50];
static int Arr_2_Glob[50][50];
static Rec_Type rec_a, rec_b;

NOINL static void Proc_1(Rec_Pointer);
NOINL static void Proc_2(One_Fifty *);
NOINL static void Proc_3(Rec_Pointer *);
NOINL static void Proc_4(void);
NOINL static void Proc_5(void);
NOINL static void Proc_6(Enumeration, Enumeration *);
NOINL static void Proc_7(One_Fifty, One_Fifty, One_Fifty *);
NOINL static void Proc_8(Arr_1_Dim, Arr_2_Dim, int, int);
NOINL static Enumeration Func_1(Capital_Letter, Capital_Letter);
NOINL static Boolean Func_2(Str_30, Str_30);
NOINL static Boolean Func_3(Enumeration);

NOINL static void dstrcpy(char *d, const char *s) { while ((*d++ = *s++)) ; }
NOINL static int dstrcmp(const char *a, const char *b) { return strcmp(a, b); }

static void Proc_1(Rec_Pointer Ptr_Val_Par)
{
    Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;
    *Ptr_Val_Par->Ptr_Comp = *Ptr_Glob;
    Ptr_Val_Par->variant.var_1.Int_Comp = 5;
    Next_Record->variant.var_1.Int_Comp = Ptr_Val_Par->variant.var_1.Int_Comp;
    Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
    Proc_3(&Next_Record->Ptr_Comp);
    if (Next_Record->Discr == Ident_1) {
        Next_Record->variant.var_1.Int_Comp = 6;
        Proc_6(Ptr_Val_Par->variant.var_1.Enum_Comp, &Next_Record->variant.var_1.Enum_Comp);
        Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
        Proc_7(Next_Record->variant.var_1.Int_Comp, 10, &Next_Record->variant.var_1.Int_Comp);
    } else
        *Ptr_Val_Par = *Ptr_Val_Par->Ptr_Comp;
}

static void Proc_2(One_Fifty *Int_Par_Ref)
{
    One_Fifty Int_Loc = *Int_Par_Ref + 10;
    Enumeration Enum_Loc = Ident_2;
    do
        if (Ch_1_Glob == 'A') { Int_Loc -= 1; *Int_Par_Ref = Int_Loc - Int_Glob; Enum_Loc = Ident_1; }
    while (Enum_Loc != Ident_1);
}

static void Proc_3(Rec_Pointer *Ptr_Ref_Par)
{
    if (Ptr_Glob) *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
    Proc_7(10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
}

static void Proc_4(void)
{
    Boolean Bool_Loc = Ch_1_Glob == 'A';
    Bool_Glob = Bool_Loc | Bool_Glob;
    Ch_2_Glob = 'B';
}

static void Proc_5(void) { Ch_1_Glob = 'A'; Bool_Glob = 0; }

static void Proc_6(Enumeration Enum_Val_Par, Enumeration *Enum_Ref_Par)
{
    *Enum_Ref_Par = Enum_Val_Par;
    if (!Func_3(Enum_Val_Par)) *Enum_Ref_Par = Ident_4;
    switch (Enum_Val_Par) {
    case Ident_1: *Enum_Ref_Par = Ident_1; break;
    case Ident_2: *Enum_Ref_Par = Int_Glob > 100 ? Ident_1 : Ident_4; break;
    case Ident_3: *Enum_Ref_Par = Ident_2; break;
    case Ident_4: break;
    case Ident_5: *Enum_Ref_Par = Ident_3; break;
    }
}

static void Proc_7(One_Fifty a, One_Fifty b, One_Fifty *r)
{
    One_Fifty Int_Loc = a + 2;
    *r = b + Int_Loc;
}

static void Proc_8(Arr_1_Dim A1, Arr_2_Dim A2, int v1, int v2)
{
    One_Fifty i, Int_Loc = v1 + 5;
    A1[Int_Loc] = v2;
    A1[Int_Loc + 1] = A1[Int_Loc];
    A1[Int_Loc + 30] = Int_Loc;
    for (i = Int_Loc; i <= Int_Loc + 1; ++i) A2[Int_Loc][i] = Int_Loc;
    A2[Int_Loc][Int_Loc - 1] += 1;
    A2[Int_Loc + 20][Int_Loc] = A1[Int_Loc];
    Int_Glob = 5;
}

static Enumeration Func_1(Capital_Letter c1, Capital_Letter c2)
{
    Capital_Letter Ch_1_Loc = c1, Ch_2_Loc = Ch_1_Loc;
    if (Ch_2_Loc != c2) return Ident_1;
    Ch_1_Glob = Ch_1_Loc;
    return Ident_2;
}

static Boolean Func_2(Str_30 s1, Str_30 s2)
{
    One_Thirty Int_Loc = 2;
    Capital_Letter Ch_Loc = 0;
    while (Int_Loc <= 2)
        if (Func_1(s1[Int_Loc], s2[Int_Loc + 1]) == Ident_1) { Ch_Loc = 'A'; Int_Loc += 1; }
    if (Ch_Loc >= 'W' && Ch_Loc < 'Z') Int_Loc = 7;
    if (Ch_Loc == 'R') return 1;
    if (dstrcmp(s1, s2) > 0) { Int_Loc += 7; Int_Glob = Int_Loc; return 1; }
    return 0;
}

static Boolean Func_3(Enumeration e) { return e == Ident_3; }

volatile int g_dhry_sink;

static void dhry_run(int runs)
{
    One_Fifty Int_1_Loc = 0, Int_2_Loc = 0, Int_3_Loc = 0;
    char Ch_Index;
    Enumeration Enum_Loc = Ident_1;
    Str_30 Str_1_Loc, Str_2_Loc;
    int Run_Index;

    Next_Ptr_Glob = &rec_b;
    Ptr_Glob = &rec_a;
    Ptr_Glob->Ptr_Comp = Next_Ptr_Glob;
    Ptr_Glob->Discr = Ident_1;
    Ptr_Glob->variant.var_1.Enum_Comp = Ident_3;
    Ptr_Glob->variant.var_1.Int_Comp = 40;
    dstrcpy(Ptr_Glob->variant.var_1.Str_Comp, "DHRYSTONE PROGRAM, SOME STRING");
    dstrcpy(Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");
    Arr_2_Glob[8][7] = 10;

    for (Run_Index = 1; Run_Index <= runs; ++Run_Index) {
        Proc_5();
        Proc_4();
        Int_1_Loc = 2;
        Int_2_Loc = 3;
        dstrcpy(Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
        Enum_Loc = Ident_2;
        Bool_Glob = !Func_2(Str_1_Loc, Str_2_Loc);
        while (Int_1_Loc < Int_2_Loc) {
            Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
            Proc_7(Int_1_Loc, Int_2_Loc, &Int_3_Loc);
            Int_1_Loc += 1;
        }
        Proc_8(Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
        Proc_1(Ptr_Glob);
        for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index) {
            if (Enum_Loc == Func_1(Ch_Index, 'C')) {
                Proc_6(Ident_1, &Enum_Loc);
                dstrcpy(Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
                Int_2_Loc = Run_Index;
                Int_Glob = Run_Index;
            }
        }
        Int_2_Loc = Int_2_Loc * Int_1_Loc;
        Int_1_Loc = Int_2_Loc / Int_3_Loc;
        Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
        Proc_2(&Int_1_Loc);
    }
    g_dhry_sink = Int_1_Loc + Int_2_Loc + Int_3_Loc + Enum_Loc;
}

/* ================= Whetstone (double precision) ================= */
static double T, T1, T2, E1[5];
static int J, K, L;
volatile double g_whet_sink;

NOINL static void PA(double *E)
{
    int j;
    for (j = 0; j < 6; j++) {
        E[1] = (E[1] + E[2] + E[3] - E[4]) * T;
        E[2] = (E[1] + E[2] - E[3] + E[4]) * T;
        E[3] = (E[1] - E[2] + E[3] + E[4]) * T;
        E[4] = (-E[1] + E[2] + E[3] + E[4]) / T2;
    }
}

NOINL static void P0(void) { E1[J] = E1[K]; E1[K] = E1[L]; E1[L] = E1[J]; }

NOINL static void P3(double X, double Y, double *Z)
{
    double X1 = X, Y1 = Y;
    X1 = T * (X1 + Y1);
    Y1 = T * (X1 + Y1);
    *Z = (X1 + Y1) / T2;
}

static void whet_run(int loop)
{
    int i, N2, N3, N4, N6, N7, N8, N9, N11;
    double X1, X2, X3, X4, X, Y, Z;

    T = .499975; T1 = 0.50025; T2 = 2.0;
    N2 = 12 * loop; N3 = 14 * loop; N4 = 345 * loop; N6 = 210 * loop;
    N7 = 32 * loop; N8 = 899 * loop; N9 = 616 * loop; N11 = 93 * loop;

    X1 = 1.0; X2 = -1.0; X3 = -1.0; X4 = -1.0;
    for (i = 1; i <= N2; i++) {
        X1 = (X1 + X2 + X3 - X4) * T;
        X2 = (X1 + X2 - X3 + X4) * T;
        X3 = (X1 - X2 + X3 + X4) * T;
        X4 = (-X1 + X2 + X3 + X4) * T;
    }
    E1[1] = 1.0; E1[2] = -1.0; E1[3] = -1.0; E1[4] = -1.0;
    for (i = 1; i <= N3; i++) PA(E1);

    J = 1;
    for (i = 1; i <= N4; i++) {
        J = (J == 1) ? 2 : 3;
        J = (J > 2) ? 0 : 1;
        J = (J < 1) ? 1 : 0;
        __asm__ volatile("" : "+r"(J));   /* keep the branch test from being folded away */
    }

    J = 1; K = 2; L = 3;
    for (i = 1; i <= N6; i++) {
        J = J * (K - J) * (L - K);
        K = L * K - (L - J) * K;
        L = (L - K) * (K + J);
        E1[L - 1] = J + K + L;
        E1[K - 1] = J * K * L;
    }

    X = 0.5; Y = 0.5;
    for (i = 1; i <= N7; i++) {
        X = T * rt_atan(T2 * rt_sin(X) * rt_cos(X) / (rt_cos(X + Y) + rt_cos(X - Y) - 1.0));
        Y = T * rt_atan(T2 * rt_sin(Y) * rt_cos(Y) / (rt_cos(X + Y) + rt_cos(X - Y) - 1.0));
    }

    X = 1.0; Y = 1.0; Z = 1.0;
    for (i = 1; i <= N8; i++) P3(X, Y, &Z);

    J = 1; K = 2; L = 3;
    E1[1] = 1.0; E1[2] = 2.0; E1[3] = 3.0;
    for (i = 1; i <= N9; i++) P0();

    X = 0.75;
    for (i = 1; i <= N11; i++) X = rt_sqrt(rt_exp(rt_log(X) / T1));

    g_whet_sink = X1 + X2 + X3 + X4 + X + Y + Z + E1[1] + J + K + L;
}

void bench_cpu(CpuResult *r, ProgressFn p)
{
    int runs = 1000, loop = 1;
    double t, t0;

    p("CPU: integer test (Dhrystone)...");
    for (;;) {
        t0 = now_sec(); dhry_run(runs); t = now_sec() - t0;
        if (t > 2.0 || runs > (1 << 29)) break;
        runs *= (t < 0.05) ? 10 : 2;
    }
    r->dmips = runs / t / 1757.0;

    p("CPU: floating point test (Whetstone)...");
    for (;;) {
        t0 = now_sec(); whet_run(loop); t = now_sec() - t0;
        if (t > 2.0 || loop > (1 << 20)) break;
        loop *= (t < 0.05) ? 10 : 2;
    }
    /* one loop = 100,000 Whetstone instructions */
    r->mwips = 100.0 * loop / t / 1000.0;
}
