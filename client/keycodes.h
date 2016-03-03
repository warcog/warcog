#ifndef WIN32
#include <X11/Xutil.h>

enum {
    key_up = XK_Up,
    key_down = XK_Down,
    key_left = XK_Left,
    key_right = XK_Right,
    key_tab = XK_Tab,
    key_enter = XK_Return,
    key_back = XK_BackSpace,

    key_f1 = XK_F1,
    key_f2 = XK_F2,
    key_f3 = XK_F3,
    key_f4 = XK_F4,
    key_f5 = XK_F5,
    key_f6 = XK_F6,
    key_f7 = XK_F7,
    key_f8 = XK_F8,
    key_f9 = XK_F9,
    key_f10 = XK_F10,
    key_f11 = XK_F11,
    key_f12 = XK_F12,

    button_left = 0,
    button_middle = 1,
    button_right = 2,

    shift_mask = ShiftMask,
    lock_mask = LockMask,
    ctrl_mask = ControlMask,
    alt_mask = Mod1Mask,
};

#else

/* from winuser.h */
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_CANCEL 0x03
#define VK_MBUTTON 0x04
#define VK_XBUTTON1 0x05
#define VK_XBUTTON2 0x06
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_KANA 0x15
#define VK_HANGEUL 0x15
#define VK_HANGUL 0x15
#define VK_JUNJA 0x17
#define VK_FINAL 0x18
#define VK_HANJA 0x19
#define VK_KANJI 0x19
#define VK_ESCAPE 0x1B
#define VK_CONVERT 0x1C
#define VK_NONCONVERT 0x1D
#define VK_ACCEPT 0x1E
#define VK_MODECHANGE 0x1F
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SELECT 0x29
#define VK_PRINT 0x2A
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F

#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B

enum {
    key_up = VK_UP,
    key_down = VK_DOWN,
    key_left = VK_LEFT,
    key_right = VK_RIGHT,
    key_tab = VK_TAB,
    key_enter = VK_RETURN,
    key_back = VK_BACK,

    key_f1 = VK_F1,
    key_f2 = VK_F2,
    key_f3 = VK_F3,
    key_f4 = VK_F4,
    key_f5 = VK_F5,
    key_f6 = VK_F6,
    key_f7 = VK_F7,
    key_f8 = VK_F8,
    key_f9 = VK_F9,
    key_f10 = VK_F10,
    key_f11 = VK_F11,
    key_f12 = VK_F12,

    button_left = 0,
    button_middle = 6,
    button_right = 3,

    shift_mask = 0,
    lock_mask = 0,
    ctrl_mask = 0,
    alt_mask = 0,
};

#endif
