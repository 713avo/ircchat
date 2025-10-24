#ifndef INPUT_H
#define INPUT_H

#include "common.h"

/* Buffer circular de historial de comandos */
typedef struct {
    char history[COMMAND_HISTORY_SIZE][MAX_INPUT_LEN];
    int count;
    int current;
    int position;
} CommandHistory;

/* Estado de la entrada */
typedef struct {
    char line[MAX_INPUT_LEN];
    int cursor_pos;
    int length;
    CommandHistory history;
} InputState;

/* Teclas especiales */
typedef enum {
    KEY_ENTER = 10,
    KEY_ESC = 27,
    KEY_BACKSPACE = 127,
    KEY_TAB = 9,
    KEY_CTRL_A = 1,
    KEY_CTRL_E = 5,
    KEY_CTRL_B = 2,
    KEY_CTRL_D = 4,
    KEY_CTRL_K = 11,
    KEY_CTRL_U = 21,
    KEY_CTRL_L = 12,
    KEY_CTRL_C = 3
} SpecialKeys;

/* Funciones de manejo de entrada */
void input_init(InputState *state);
void input_add_char(InputState *state, char c);
void input_add_utf8(InputState *state, const char *utf8_char, int len);
void input_backspace(InputState *state);
void input_delete_char(InputState *state);
void input_move_left(InputState *state);
void input_move_right(InputState *state);
void input_move_home(InputState *state);
void input_move_end(InputState *state);
void input_clear_line(InputState *state);
void input_history_add(InputState *state, const char *line);
void input_history_prev(InputState *state);
void input_history_next(InputState *state);
char* input_get_line(InputState *state);

/* Código especial de tecla */
typedef enum {
    KEY_NORMAL = 0,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    KEY_CTRL_ARROW_UP,
    KEY_CTRL_ARROW_DOWN,
    KEY_CTRL_SHIFT_ARROW_UP,
    KEY_CTRL_SHIFT_ARROW_DOWN,
    KEY_CTRL_SHIFT_ARROW_LEFT,
    KEY_CTRL_SHIFT_ARROW_RIGHT,
    KEY_ALT_ARROW_LEFT,
    KEY_ALT_ARROW_RIGHT,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE,
    KEY_ALT_0,
    KEY_ALT_1,
    KEY_ALT_2,
    KEY_ALT_3,
    KEY_ALT_4,
    KEY_ALT_5,
    KEY_ALT_6,
    KEY_ALT_7,
    KEY_ALT_8,
    KEY_ALT_9,
    KEY_ALT_PERIOD
} KeyCode;

/* Leer tecla del terminal */
int input_read_key(void);

#endif /* INPUT_H */
