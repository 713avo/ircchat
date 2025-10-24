#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"
#include "windows.h"
#include <termios.h>

/* Estado del terminal */
typedef struct {
    struct termios original_termios;
    int rows;
    int cols;
    bool raw_mode;
    bool blink_state;       /* Estado del parpadeo para notificaciones */
    int blink_frame_count;  /* Contador de frames para controlar velocidad de parpadeo */
} TerminalState;

/* Funciones de control del terminal */
void term_init(TerminalState *term);
void term_cleanup(TerminalState *term);
void term_enter_raw_mode(TerminalState *term);
void term_exit_raw_mode(TerminalState *term);
void term_get_size(TerminalState *term);
void term_clear_screen(void);
void term_move_cursor(int row, int col);
void term_hide_cursor(void);
void term_show_cursor(void);

/* Funciones de dibujo */
void term_draw_interface(TerminalState *term, WindowManager *wm, const char *input_line, int cursor_pos, bool notify_alert, bool mention_alert);
void term_draw_separator(int col_width);
void term_draw_system_window(TerminalState *term, Window *win);
void term_draw_channel_window(TerminalState *term, Window *win);
void term_draw_private_window(TerminalState *term, Window *win);
void term_draw_prompt(TerminalState *term, WindowManager *wm, const char *input_line, int cursor_pos, bool notify_alert, bool mention_alert);
void term_draw_horizontal_line(int row, int width);
void term_draw_vertical_line(int col, int start_row, int end_row);

#endif /* TERMINAL_H */
