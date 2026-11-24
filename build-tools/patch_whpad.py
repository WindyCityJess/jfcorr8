"""MiSTer: Witchaven's pad buttons.

The game maps four joystick buttons (joyb bits 0..3) to actions. On the
MiSTer joyb's bits 2..9 are the core's eight buttons in the order of its J1
list, R B A L Y X Select Start (bits 0 and 1 are the OSD's Menu OK/Back, and
the d-pad presses the arrow keys): the eight are mapped by default to
  R fire, B jump, A use, L run, Y cast spell, X use potion, Select map,
  Start nothing (Enter in the menus).
The joystick is always on. The game's own setup screen still shows the first
four of these as its joystick buttons 1..4.
usage: patch_whpad.py <jfwhaven>
"""
import os, sys

root = sys.argv[1]
CRLF = chr(13) + chr(10)

def edit(rel, pairs):
    p = os.path.join(root, rel)
    if not os.path.exists(p):
        return
    s = open(p, encoding='utf-8', errors='replace', newline='').read()
    crlf = CRLF in s
    for old, new in pairs:
        if crlf:
            old, new = old.replace(chr(10), CRLF), new.replace(chr(10), CRLF)
        if new in s:
            continue
        assert s.count(old) == 1, f'{rel}: {old[:50]!r} x{s.count(old)}'
        s = s.replace(old, new)
    open(p, 'w', encoding='utf-8', newline='').write(s)
    print('patched', rel)

edit('src/whinp.c', [
    ('''	 butbit[]={0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80};
''', '''#ifdef MISTER_HYBRID
	 // MiSTer: the core's eight buttons, joyb bits 2..9 (R B A L Y X Select Start)
	 butbit[]={0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200};
#else
	 butbit[]={0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80};
#endif
'''),
    ('''int joykeys[4]={KEYFIRE,KEYUSE,KEYUSEP,KEYSTRAFE};
''', '''#ifdef MISTER_HYBRID
#define NUMJOYKEYS 8
int joykeys[NUMJOYKEYS]={KEYFIRE,KEYJUMP,KEYUSE,KEYRUN,KEYCAST,KEYUSEP,KEYMAP,-1};
#else
#define NUMJOYKEYS 4
int joykeys[NUMJOYKEYS]={KEYFIRE,KEYUSE,KEYUSEP,KEYSTRAFE};
#endif
'''),
    ('''		for (i=0 ; i < 4 ; i++) {
			if (((joyb&butbit[i]) == butbit[i])          // button is down
''', '''		for (i=0 ; i < NUMJOYKEYS ; i++) {
			if (joykeys[i] < 0) continue;                // a button without an action
			if (((joyb&butbit[i]) == butbit[i])          // button is down
'''),
])

edit('src/icorp.h', [
    ('''extern int keys[NUMKEYS], mousekeys[2], joykeys[4];
''', '''extern int keys[NUMKEYS], mousekeys[2], joykeys[];
'''),
])

edit('src/whaven.c', [
    ('''    initinput();
''', '''    initinput();
#ifdef MISTER_HYBRID
    option[3] |= 2; // MiSTer: the pad is always on
#endif
'''),
])
