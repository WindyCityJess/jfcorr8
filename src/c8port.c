// Corridor 8: Galactic Wars on the MiSTer (jfSW core): the program's entry
// and the few things of the DOS build environment the game modules used.
// The DOS game was hosted by Ken's BUILD editor: its ExtInit() runs the whole
// game when not in editor mode. Here app_main() sets the engine up the
// MiSTer way and calls that ExtInit().
#include "icorp.h"
#include "osd.h"
#include "version.h"
#include "sos.h"
#ifdef MISTER_HYBRID
#include "mister_port.h"
#endif

extern char tkeys[256];
int xdimgame = 320, ydimgame = 200, bppgame = 8, forcesetup = 1;
char tempbuf[8192];
// the BUILD editor's globals the game shares with it (Ken's build.c)
int posx, posy, posz, kensplayerheight;
short ang, cursectnum;
char buildkeys[20];
char names[9216][17];
// the DOS runtime's program arguments the game's checkParms() reads
int _argc;
char **_argv;

// referenced by code the optimizer drops in a release build (an -O0 build links them)
__attribute__((weak)) const unsigned char appicon_bmp[1];
__attribute__((weak)) const int appicon_bmp_size = 0;

void c8_misterinit(void) { }

// A crash reports itself to the log (the MiSTer writes no core file):
// the signal and the raw backtrace, to symbolize with addr2line against
// the unstripped build (non-PIE, so the addresses are the binary's).
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
static void c8_crashreport(int sig) {
    void *frames[40];
    char line[64];
    int n = backtrace(frames, 40), i, len;
    len = snprintf(line, sizeof line, "c8 crash: signal %d, backtrace:\n", sig);
    write(1, line, len);
    for (i = 0; i < n; i++) {
        len = snprintf(line, sizeof line, "  %p\n", frames[i]);
        write(1, line, len);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}
__attribute__((constructor)) static void c8_crashreport_init(void) {
    signal(SIGSEGV, c8_crashreport);
    signal(SIGBUS, c8_crashreport);
    signal(SIGILL, c8_crashreport);
    signal(SIGFPE, c8_crashreport);
    signal(SIGABRT, c8_crashreport);
}

void keytimerstuff(void) { }


// The pad as the game's action keys (the game reads keystatus[] through its
// keys[] table alone). joyb (the engine's MiSTer joystick update): bits 2..9
// are the core's eight buttons R B A L Y X Select Start; the d-pad, Menu OK
// and Start are already the arrow keys, Enter and Escape there. Buttons:
// R fire, B jump, A use, L run, Y crouch, X strafe, Select map. Sticks:
// left moves and strafes, right turns and looks, as keys past a quarter.
extern int c8_invertlook; // readcfg.c, [MISTER] INVERT LOOK
static void c8_padkeys(void) {
    static const struct { int bit, key; } buttons[] = {
        { 2, KEYFIRE }, { 3, KEYJUMP }, { 4, KEYUSE }, { 5, KEYRUN },
        { 6, KEYCROUCH }, { 7, KEYSTRAFE }, { 8, KEYMAP },
    };
    static const struct { int axis, sign, key; } sticks[] = {
        { 1, -1, KEYFWD }, { 1, 1, KEYBACK }, { 0, -1, KEYSTFL }, { 0, 1, KEYSTFR },
        { 2, -1, KEYLEFT }, { 2, 1, KEYRIGHT }, { 3, -1, KEYLKUP }, { 3, 1, KEYLKDN },
    };
    static int lastb, stickdown[8];
    int b = joyb, changed = b ^ lastb, k;
    for (k = 0; k < (int)(sizeof buttons / sizeof buttons[0]); k++) {
        if (changed & (1 << buttons[k].bit)) {
            keystatus[(unsigned char)keys[buttons[k].key]] = (b >> buttons[k].bit) & 1;
        }
    }
    lastb = b;
    {
        static int tracenext;
        if (getenv("C8DEBUG") && (int)(totalclock - tracenext) >= 0) {
            tracenext = totalclock + 60;
            { int ps = PLR_getPlayerSprite(myconnectindex); buildprintf("c8 pad: joyb %x axes [%d %d %d %d] fwd %02x=%d tk %d pos %d,%d ang %d\n", b, joyaxis[0], joyaxis[1], joyaxis[2], joyaxis[3], (unsigned char)keys[KEYFWD], keystatus[(unsigned char)keys[KEYFWD]], tkeys[(unsigned char)keys[KEYFWD]], ps >= 0 ? sprite[ps].x : 0, ps >= 0 ? sprite[ps].y : 0, ps >= 0 ? sprite[ps].ang : 0); }
        }
    }
    for (k = 0; k < (int)(sizeof sticks / sizeof sticks[0]); k++) {
        int sign = sticks[k].sign;
        if (sticks[k].axis == 3 && c8_invertlook) sign = -sign; // Joystick Options: INVERT LOOK
        int v = joynumaxes > sticks[k].axis ? joyaxis[sticks[k].axis] * sign : 0;
        int down = v > 8192;
        if (down != stickdown[k]) {
            keystatus[(unsigned char)keys[sticks[k].key]] = down;
            stickdown[k] = down;
        }
    }
}

void c8_nextpage(void) {
    handleevents();
    c8_padkeys();
    OSD_DispatchQueued();
    (nextpage)();
}

static void c8_tick(void) {
    sosemu_tick();
}

int c8_setgamemode(void) {
    if (setgamemode(0, xdimgame, ydimgame, bppgame) < 0) {
        crash("Could not set video mode %dx%d", xdimgame, ydimgame);
    }
    return 0;
}

// the DOS engine's loadboard took the angle as a long
int c8_loadboard(const char *name, int *x, int *y, int *z, short *ang, short *sect) {
    return loadboard((char *)name, 0, x, y, z, ang, sect);
}
int c8_getpacket(short *node, char *buf) {
    int other = 0, r = getpacket(&other, (unsigned char *)buf);
    *node = (short)other;
    return r;
}
void c8_getmousevalues(short *x, short *y, short *b) {
    int mx, my, mb;
    getmousevalues(&mx, &my, &mb);
    *x = (short)mx; *y = (short)my; *b = (short)mb;
}

// the editor's screens: nothing on the MiSTer (the game's editor mode is not built in)
short getnumber16(char *namestart, short num, int maxnumber) { (void)namestart; (void)maxnumber; return num; }
void printmessage16(char *name) { (void)name; }
void clearmidstatbar16(void) { }
int gettile(int tilenum) { return tilenum; }
void draw2dscreen(int posxe, int posye, short ange, int zoome, short gride) { (void)posxe; (void)posye; (void)ange; (void)zoome; (void)gride; }
void clear2dscreen(int col) { (void)col; }
void printext16(int x, int y, short col, short backcol, char *name, char fontsize) { (void)x; (void)y; (void)col; (void)backcol; (void)name; (void)fontsize; }
void editinput(void) { }
void drawline16(int x1, int y1, int x2, int y2, char col) { (void)x1; (void)y1; (void)x2; (void)y2; (void)col; }

void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext) {
    const char *slash = strrchr(path, '/'), *bs = strrchr(path, '\\'), *dot;
    const char *base = slash > bs ? slash + 1 : (bs ? bs + 1 : path);
    if (drive) drive[0] = 0;
    if (dir) { size_t n = base - path; memcpy(dir, path, n); dir[n] = 0; }
    dot = strrchr(base, '.');
    if (fname) { size_t n = dot ? (size_t)(dot - base) : strlen(base); memcpy(fname, base, n); fname[n] = 0; }
    if (ext) strcpy(ext, dot ? dot : "");
}

int app_main(int argc, char const * const argv[]) {
    _argc = argc;
    _argv = (char **)argv;
#ifdef MISTER_HYBRID
    {
        char cwd[BMAX_PATH + 1];
        if (getcwd(cwd, sizeof(cwd))) {
            addsearchpath(cwd);
        }
    }
    MiSTer_Init();
#else
    buildsetlogfile("corr8.log");
#endif
    wm_setapptitle("Corridor 8: Galactic Wars");
    buildprintf("\nCorridor 8: Galactic Wars (Capstone, 1996, unfinished) on JonoF's Build engine\n"
                "Version %s.\nBuilt %s %s.\n", game_version, game_date, game_time);
    if (preinitengine()) {
        wm_msgbox("Build Engine Initialisation Error", "There was a problem initialising the Build engine: %s", engineerrstr);
        exit(1);
    }
#ifdef MISTER_HYBRID
    xdimgame = MiSTer_ScreenWidth();
    ydimgame = MiSTer_ScreenHeight();
    bppgame = 8;
    MiSTer_Loading();
    MiSTer_PickGame();
#endif
    initinput();     // the engine's keyboard and pad (JonoF's ports do this in app_main)
    inittimer(120, c8_tick);
    // the game: it does not return (the game's crash("normal shutdown") exits)
    ExtInit();
    return 0;
}
