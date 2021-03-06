void *
Allocate(int Size)
{
    return VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

enum key {
    Key_Backspace = 0x08,
    Key_Tab,

    Key_Enter = 0x0d,

    Key_Shift = 0x10,
    Key_Ctrl,
    Key_Alt,
    Key_Pause,
    Key_CapsLock,

    Key_Esc = 0x1b,

    Key_Space = 0x20,
    Key_PgUp,
    Key_PgDown,
    Key_End,
    Key_Home,
    Key_Left,
    Key_Up,
    Key_Right,
    Key_Down,

    Key_Ins = 0x2d,
    Key_Del,
    
    Key_1 = 0x31,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    Key_0,

    Key_A = 0x41,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    Key_F1 = 0x70,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,

    Key_Semicolon = 0xBA,
    Key_Plus,
    Key_Comma,
    Key_Minus,
    Key_Dot,
    Key_ForwardSlash,
    Key_Tildae,

    Key_OpenBracket = 0xDB,
    Key_BackSlash,
    Key_CloseBracket,
    Key_Quote,

    Key_Count,
};

struct button {
    b32 Down;
    b32 WentDown;
    b32 DoubleClick;
    b32 WentUp;
    // b32 WentDownOrRepeated;
};

struct input {
    button Keys[Key_Count];
    button Mouse[2];
    int Wheel;

    v2 MouseP;

    utf8 Chars[10];
    u32 CharsSize;
    u32 CharCount;
};

static input Input = {};

struct state {
    u32 Width;
    u32 Height;
};

static state State = {};

