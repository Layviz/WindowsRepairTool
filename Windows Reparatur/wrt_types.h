#pragma once

typedef enum {
    NO_OP = 0,
    SIMPLE_REPAIR = 1,
    DEFAULT_REPAIR = 2,
    EXT_REPAIR = 3,
    HELP = 4,
    VERBOSE_ON = 5,
    VERBOSE_OFF = 6,
}OPTION;

typedef enum {
    DEFAULT,
    VERBOSE,
    SILENT
}VEROBOSITY;

typedef struct {
    OPTION mode;
    VEROBOSITY verbose;
}OPERATION;
