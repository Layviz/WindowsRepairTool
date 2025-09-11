/*
MIT License

Copyright (c) 2025 Jörn Andreesen <joern.andreesen@fu-berlin.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef WINTERMBOX_H_INCL
#define WINTERMBOX_H_INCL


#if WINTERMBOX_EXPORTS
#define WINTERMBOX_API __declspec(dllexport)
#else
#define WINTERMBOX_API __declspec(dllimport)
#endif

#define WTB_CURSOR_VISIBILITY_OFF 0
#define WTB_CURSOR_VISIBILITY_ON 1
#define WTB_CURSOR_VISIBILITY_ENABLE_BLINK 2
#define WTB_CURSOR_VISIBILITY_DISABLE_BLINK 3

#define WTB_CURSOR_SHAPE_DEFAULT 0
#define WTB_CURSOR_SHAPE_BLINKING_BLOCK 1
#define WTB_CURSOR_SHAPE_STEADY_BLOCK 2
#define WTB_CURSOR_SHAPE_BLINKING_UNDERLINE 3
#define WTB_CURSOR_SHAPE_STEADY_UNDERLINE 4
#define WTB_CURSOR_SHAPE_BLINKING_BAR 5
#define WTB_CURSOR_SHAPE_STEADY_BAR 6

#define WTB_COLOR_DEFAULT 0
#define WTB_COLOR_BOLD 1
#define WTB_COLOR_NO_BOLD 22
#define WTB_COLOR_UNDERLINE 4
#define WTB_COLOR_NO_UNDERLINE 24
#define WTB_COLOR_NEGATIVE 7
#define WTB_COLOR_NO_NEGATIVE 27
#define WTB_COLOR_BLACK 30
#define WTB_COLOR_RED 31
#define WTB_COLOR_GREEN 32
#define WTB_COLOR_YELLOW 33
#define WTB_COLOR_BLUE 34
#define WTB_COLOR_MAGENTA 35
#define WTB_COLOR_CYAN 36
#define WTB_COLOR_WHITE 37
#define WTB_COLOR_BRIGHT_BLACK 90
#define WTB_COLOR_BRIGHT_RED 91
#define WTB_COLOR_BRIGHT_GREEN 92
#define WTB_COLOR_BRIGHT_YELLOW 93
#define WTB_COLOR_BRIGHT_BLUE 94
#define WTB_COLOR_BRIGHT_MAGENTA 95
#define WTB_COLOR_BRIGHT_CYAN 96
#define WTB_COLOR_BRIGHT_WHITE 97

#define WTB_SCREEN_BUFFER_MAIN 0
#define WTB_SCREEN_BUFFER_ALT 1

/**
 * @brief Enables virtual terminal processing for the console. Must be called before any other function.
 * @return 1 on success, 0 on failure.
 */
extern "C" WINTERMBOX_API int wtb_init(void);

/**
 * @brief Clears the console screen and moves the cursor to the top-left corner.
 * @return 1 on success, 0 when wtb is not initalized.
 */
extern "C" WINTERMBOX_API int wtb_clear(void);

/**
 * @brief Clears the given line (y coordinate) and moves the cursor to the beginning of the line.
 * @param y The y coordinate (0-based).
 * @return 1 on success, 0 when wtb is not initalized.
 */
extern "C" WINTERMBOX_API int wtb_clear_line(int y);

/**
 * @brief Sets the cursor position.
 * @param x The x coordinate (0-based).
 * @param y The y coordinate (0-based).
 * @return 1 on success, 0 when wtb is not initalized.
 */
extern "C" WINTERMBOX_API int wtb_set_cursor_pos(int x, int y);
/**
 * @brief Sets the cursor visibility.
 * @param visibility One of WTB_CURSOR_VISIBILITY_* constants.
 * @return 1 on success, 0 when wtb is not initalized or invalid visibility.
 */
extern "C" WINTERMBOX_API int wtb_set_cursor_visible(int visibility);
/**
 * @brief Sets the cursor shape.
 * @param shape One of WTB_CURSOR_SHAPE_* constants.
 * @return 1 on success, 0 when wtb is not initalized or invalid shape.
 */
extern "C" WINTERMBOX_API int wtb_set_cursor_shape(int shape);
/**
 * @brief Sets a cell at the given position to the given character and colors.
 * @param x The x coordinate (0-based).
 * @param y The y coordinate (0-based).
 * @param ch The character to set.
 * @param fg The foreground color (one of WTB_COLOR_* constants or 0 for default).
 * @param bg The background color (one of WTB_COLOR_* constants or 0 for default).
 * @return 1 on success, 0 when wtb is not initalized or invalid color.
 */
extern "C" WINTERMBOX_API int wtb_set_cell(int x, int y, wchar_t ch, int fg, int bg);
/**
 * @brief Prints a wide string at the given position with the given colors.
 * @param x The x coordinate (0-based).
 * @param y The y coordinate (0-based).
 * @param fg The foreground color (one of WTB_COLOR_* constants or 0 for default).
 * @param bg The background color (one of WTB_COLOR_* constants or 0 for default).
 * @param str The wide string to print.
 * @return 1 on success, 0 when wtb is not initalized or invalid color.
 */
extern "C" WINTERMBOX_API int wtb_wprint(int x, int y, int fg, int bg, const wchar_t* str);

/**
 * @brief Sets the active screen buffer.
 * @param buffer_id One of WTB_SCREEN_BUFFER_* constants.
 * @return 1 on success, 0 when wtb is not initalized or invalid buffer_id.
 */
extern "C" WINTERMBOX_API int wtb_set_screen_buffer(int buffer_id);
/**
 * @brief Sets the default colors for subsequent text output.
 * @param fg The foreground color (one of WTB_COLOR_* constants or 0 for default).
 * @param bg The background color (one of WTB_COLOR_* constants or 0 for default).
 * @return 1 on success, 0 when wtb is not initalized or invalid color.
 */
extern "C" WINTERMBOX_API int wtb_set_color(int fg, int bg);

/**
 * @brief Sets a custom foreground color using RGB values.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @return 1 on success, 0 when wtb is not initalized or invalid color.
 */
extern "C" WINTERMBOX_API int wtb_set_custom_foreground_color(int r, int g, int b);

/**
 * @brief Sets a custom background color using RGB values.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @return 1 on success, 0 when wtb is not initalized or invalid color.
 */
extern "C" WINTERMBOX_API int wtb_set_custom_background_color(int r, int g, int b);

#endif // !WINTERMBOX_H_INCL

#ifdef WINTERMBOX_IMPL

#include <windows.h>
#include <stdio.h>
#include <wchar.h>

int is_intialized = 0;

int wtb_init(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return 0;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) {
        return 0;
    }
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(hOut, mode)) {
        return 0;
    }
    is_intialized = 1;
    return 1;
}

int wtb_clear(void) {
    if (!is_intialized) {
        return 0;
    }
    wprintf(L"\x1b[2J\x1b[H");
    return 1;
}

int wtb_clear_line(int y) {
    if (!is_intialized) {
        return 0;
    }
    wprintf(L"\x1b[%d;1H\x1b[2K", y + 1);
    return 1;
}

int wtb_set_cursor_pos(int x, int y) {
    if (!is_intialized) {
        return 0;
    }
    wprintf(L"\x1b[%d;%dH", y + 1, x + 1);
    return 1;
}

int wtb_set_cursor_visible(int visibility) {
    if (!is_intialized) {
        return 0;
    }
    switch (visibility) {
    case WTB_CURSOR_VISIBILITY_OFF:
        wprintf(L"\x1b[?25l");
        break;
    case WTB_CURSOR_VISIBILITY_ON:
        wprintf(L"\x1b[?25h");
        break;
    case WTB_CURSOR_VISIBILITY_ENABLE_BLINK:
        wprintf(L"\x1b[?12h");
        break;
    case WTB_CURSOR_VISIBILITY_DISABLE_BLINK:
        wprintf(L"\x1b[?12l");
        break;
    default:
        return 0;
    }
    return 1;
}

int wtb_set_cursor_shape(int shape) {
    if (!is_intialized) {
        return 0;
    }
    switch (shape) {
    case WTB_CURSOR_SHAPE_DEFAULT:
        wprintf(L"\x1b[0 q");
        break;
    case WTB_CURSOR_SHAPE_BLINKING_BLOCK:
        wprintf(L"\x1b[1 q");
        break;
    case WTB_CURSOR_SHAPE_STEADY_BLOCK:
        wprintf(L"\x1b[2 q");
        break;
    case WTB_CURSOR_SHAPE_BLINKING_UNDERLINE:
        wprintf(L"\x1b[3 q");
        break;
    case WTB_CURSOR_SHAPE_STEADY_UNDERLINE:
        wprintf(L"\x1b[4 q");
        break;
    case WTB_CURSOR_SHAPE_BLINKING_BAR:
        wprintf(L"\x1b[5 q");
        break;
    case WTB_CURSOR_SHAPE_STEADY_BAR:
        wprintf(L"\x1b[6 q");
        break;
    default:
        return 0;
    }
    return 1;
}

int wtb_set_cell(int x, int y, wchar_t ch, int fg, int bg) {
    if (!is_intialized) {
        return 0;
    }
    if (fg > 37 && fg < 90) {
        return 0; // Invalid foreground color
    }
    if (bg > 37 && bg < 90) {
        return 0; // Invalid background color
    }
    if (bg >= 30) {
        bg += 10; // Convert to background color
    }
    if (fg == 0) {
        fg = 39; // Default foreground color
    }
    if (bg == 0) {
        bg = 49; // Default background color
    }
    wprintf(L"\x1b[%d;%dH\x1b[%d;%dm%c\x1b[0m", y + 1, x + 1, fg, bg, ch);
    return 1;
}

int wtb_wprint(int x, int y, int fg, int bg, const wchar_t* str) {
    if (!is_intialized) {
        return 0;
    }
    if (fg > 37 && fg < 90) {
        return 0; // Invalid foreground color
    }
    if (bg > 37 && bg < 90) {
        return 0; // Invalid background color
    }
    if (bg >= 30) {
        bg += 10; // Convert to background color
    }
    if (fg == 0) {
        fg = 39; // Default foreground color
    }
    if (bg == 0) {
        bg = 49; // Default background color
    }
    wprintf(L"\x1b[%d;%dH\x1b[%d;%dm%s\x1b[0m", y + 1, x + 1, fg, bg, str);
    return 1;
}

int wtb_set_screen_buffer(int buffer_id) {
    if (!is_intialized) {
        return 0;
    }
    if (buffer_id == WTB_SCREEN_BUFFER_ALT) {
        wprintf(L"\x1b[?1049h"); // Switch to alternate screen buffer
    }
    else if (buffer_id == WTB_SCREEN_BUFFER_MAIN) {
        wprintf(L"\x1b[?1049l"); // Switch to main screen buffer
    }
    else {
        return 0; // Invalid buffer ID
    }
    return 1;
}

int wtb_set_color(int fg, int bg) {
    if (!is_intialized) {
        return 0;
    }
    if (fg > 37 && fg < 90) {
        return 0; // Invalid foreground color
    }
    if (bg > 37 && bg < 90) {
        return 0; // Invalid background color
    }
    if (bg >= 30) {
        bg += 10; // Convert to background color
    }
    if (fg == 0) {
        fg = 39; // Default foreground color
    }
    if (bg == 0) {
        bg = 49; // Default background color
    }
    wprintf(L"\x1b[%d;%dm", fg, bg);
    return 1;
}

int wtb_set_custom_foreground_color(int r, int g, int b) {
    if (!is_intialized) {
        return 0;
    }
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return 0; // Invalid color component
    }
    wprintf(L"\x1b[38;2;%d;%d;%dm", r, g, b);
    return 1;
}

int wtb_set_custom_background_color(int r, int g, int b) {
    if (!is_intialized) {
        return 0;
    }
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return 0; // Invalid color component
    }
    wprintf(L"\x1b[48;2;%d;%d;%dm", r, g, b);
    return 1;
}

#endif // WINTERMBOX_IMPL
