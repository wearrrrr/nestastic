#include <cstdint>

enum AddrMode {
    IMP, IMM, ZP, ZPX, ZPY, ABS, ABSX, ABSY,
    IND, XIND, INDY, REL
};

struct OpInfo {
    AddrMode mode;
    uint8_t bytes;
    uint8_t cycles;
};

static const OpInfo OPTABLE[256] = {
/* 00 */ {IMP,1,7}, {XIND,2,6}, {IMP,1,0}, {IMP,1,0},
/* 04 */ {ZP,2,3}, {ZP,2,5}, {ZP,2,0}, {IMP,1,0},
/* 08 */ {IMP,1,3}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* 0C */ {ABS,3,4}, {ABS,3,6}, {ABS,3,0}, {IMP,1,0},

/* 10 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* 14 */ {ZPX,2,4}, {ZPX,2,6}, {ZPX,2,0}, {IMP,1,0},
/* 18 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* 1C */ {ABSX,3,4}, {ABSX,3,6}, {ABSX,3,0}, {IMP,1,0},

/* 20 */ {ABS,3,6}, {XIND,2,6}, {IMP,1,0}, {IMP,1,0},
/* 24 */ {ZP,2,3}, {ZP,2,5}, {ZP,2,0}, {IMP,1,0},
/* 28 */ {IMP,1,4}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* 2C */ {ABS,3,4}, {ABS,3,6}, {ABS,3,0}, {IMP,1,0},

/* 30 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* 34 */ {ZPX,2,4}, {ZPX,2,6}, {ZPX,2,0}, {IMP,1,0},
/* 38 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* 3C */ {ABSX,3,4}, {ABSX,3,6}, {ABSX,3,0}, {IMP,1,0},

/* 40 */ {IMP,1,6}, {XIND,2,6}, {IMP,1,0}, {IMP,1,0},
/* 44 */ {ZP,2,3}, {ZP,2,5}, {ZP,2,0}, {IMP,1,0},
/* 48 */ {IMP,1,3}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* 4C */ {ABS,3,3}, {ABS,3,6}, {ABS,3,0}, {IMP,1,0},

/* 50 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* 54 */ {ZPX,2,4}, {ZPX,2,6}, {ZPX,2,0}, {IMP,1,0},
/* 58 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* 5C */ {ABSX,3,4}, {ABSX,3,6}, {ABSX,3,0}, {IMP,1,0},

/* 60 */ {IMP,1,6}, {XIND,2,6}, {IMP,1,0}, {IMP,1,0},
/* 64 */ {ZP,2,3}, {ZP,2,5}, {ZP,2,0}, {IMP,1,0},
/* 68 */ {IMP,1,4}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* 6C */ {IND,3,5}, {ABS,3,6}, {ABS,3,0}, {IMP,1,0},

/* 70 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* 74 */ {ZPX,2,4}, {ZPX,2,6}, {ZPX,2,0}, {IMP,1,0},
/* 78 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* 7C */ {ABSX,3,4}, {ABSX,3,6}, {ABSX,3,0}, {IMP,1,0},

/* 80 */ {IMP,1,0}, {XIND,2,6}, {IMP,1,0}, {IMP,1,0},
/* 84 */ {ZP,2,3}, {ZP,2,3}, {ZP,2,3}, {IMP,1,0},
/* 88 */ {IMP,1,2}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* 8C */ {ABS,3,4}, {ABS,3,4}, {ABS,3,4}, {IMP,1,0},

/* 90 */ {REL,2,2}, {INDY,2,6}, {IMP,1,0}, {IMP,1,0},
/* 94 */ {ZPX,2,4}, {ZPX,2,4}, {ZPX,2,4}, {IMP,1,0},
/* 98 */ {IMP,1,2}, {ABSY,3,5}, {IMP,1,2}, {IMP,1,0},
/* 9C */ {ABSX,3,0}, {ABSX,3,5}, {ABSY,3,5}, {IMP,1,0},

/* A0 */ {IMM,2,2}, {XIND,2,6}, {IMM,2,2}, {IMP,1,0},
/* A4 */ {ZP,2,3}, {ZP,2,3}, {ZP,2,3}, {IMP,1,0},
/* A8 */ {IMP,1,2}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* AC */ {ABS,3,4}, {ABS,3,4}, {ABS,3,4}, {IMP,1,0},

/* B0 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* B4 */ {ZPX,2,4}, {ZPX,2,4}, {ZPX,2,4}, {IMP,1,0},
/* B8 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* BC */ {ABSX,3,4}, {ABSX,3,4}, {ABSX,3,4}, {IMP,1,0},

/* C0 */ {IMM,2,2}, {XIND,2,6}, {IMM,2,2}, {IMP,1,0},
/* C4 */ {ZP,2,3}, {ZP,2,5}, {ZP,2,0}, {IMP,1,0},
/* C8 */ {IMP,1,2}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* CC */ {ABS,3,4}, {ABS,3,6}, {ABS,3,0}, {IMP,1,0},

/* D0 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* D4 */ {ZPX,2,4}, {ZPX,2,6}, {ZPX,2,0}, {IMP,1,0},
/* D8 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* DC */ {ABSX,3,4}, {ABSX,3,6}, {ABSX,3,0}, {IMP,1,0},

/* E0 */ {IMM,2,2}, {XIND,2,6}, {IMM,2,2}, {IMP,1,0},
/* E4 */ {ZP,2,3}, {ZP,2,5}, {ZP,2,0}, {IMP,1,0},
/* E8 */ {IMP,1,2}, {IMM,2,2}, {IMP,1,2}, {IMP,1,0},
/* EC */ {ABS,3,4}, {ABS,3,6}, {ABS,3,0}, {IMP,1,0},

/* F0 */ {REL,2,2}, {INDY,2,5}, {IMP,1,0}, {IMP,1,0},
/* F4 */ {ZPX,2,4}, {ZPX,2,6}, {ZPX,2,0}, {IMP,1,0},
/* F8 */ {IMP,1,2}, {ABSY,3,4}, {IMP,1,2}, {IMP,1,0},
/* FC */ {ABSX,3,4}, {ABSX,3,6}, {ABSX,3,0}, {IMP,1,0},
};
