#include "input.h"
#include <unistd.h>

/* Obtener número de bytes de un carácter UTF-8 */
static int utf8_char_length(unsigned char c) {
    if ((c & 0x80) == 0x00) return 1;  /* 0xxxxxxx - ASCII */
    if ((c & 0xE0) == 0xC0) return 2;  /* 110xxxxx - 2 bytes */
    if ((c & 0xF0) == 0xE0) return 3;  /* 1110xxxx - 3 bytes */
    if ((c & 0xF8) == 0xF0) return 4;  /* 11110xxx - 4 bytes */
    return 1;  /* Por defecto, asumir 1 byte */
}

/* Añadir secuencia UTF-8 al buffer */
void input_add_utf8(InputState *state, const char *utf8_char, int len) {
    if (!state || !utf8_char || len <= 0) return;
    if (state->length + len >= MAX_INPUT_LEN - 1) return;

    /* Si el cursor está al final */
    if (state->cursor_pos == state->length) {
        memcpy(&state->line[state->cursor_pos], utf8_char, len);
        state->cursor_pos += len;
        state->length += len;
        state->line[state->length] = '\0';
    } else {
        /* Insertar en medio: desplazar caracteres a la derecha */
        memmove(&state->line[state->cursor_pos + len],
                &state->line[state->cursor_pos],
                state->length - state->cursor_pos);
        memcpy(&state->line[state->cursor_pos], utf8_char, len);
        state->cursor_pos += len;
        state->length += len;
        state->line[state->length] = '\0';
    }
}

/* Inicializar estado de entrada */
void input_init(InputState *state) {
    if (!state) return;

    memset(state->line, 0, MAX_INPUT_LEN);
    state->cursor_pos = 0;
    state->length = 0;

    /* Inicializar historial */
    for (int i = 0; i < COMMAND_HISTORY_SIZE; i++) {
        state->history.history[i][0] = '\0';
    }
    state->history.count = 0;
    state->history.current = 0;
    state->history.position = -1;
}

/* Añadir carácter en la posición del cursor */
void input_add_char(InputState *state, char c) {
    if (!state || state->length >= MAX_INPUT_LEN - 1) return;

    /* Si el cursor está al final */
    if (state->cursor_pos == state->length) {
        state->line[state->cursor_pos] = c;
        state->cursor_pos++;
        state->length++;
        state->line[state->length] = '\0';
    } else {
        /* Insertar en medio: desplazar caracteres a la derecha */
        memmove(&state->line[state->cursor_pos + 1],
                &state->line[state->cursor_pos],
                state->length - state->cursor_pos);
        state->line[state->cursor_pos] = c;
        state->cursor_pos++;
        state->length++;
        state->line[state->length] = '\0';
    }
}

/* Eliminar carácter antes del cursor (backspace) */
void input_backspace(InputState *state) {
    if (!state || state->cursor_pos == 0) return;

    if (state->cursor_pos == state->length) {
        /* Eliminar al final */
        state->cursor_pos--;
        state->length--;
        state->line[state->length] = '\0';
    } else {
        /* Eliminar en medio: desplazar caracteres a la izquierda */
        memmove(&state->line[state->cursor_pos - 1],
                &state->line[state->cursor_pos],
                state->length - state->cursor_pos);
        state->cursor_pos--;
        state->length--;
        state->line[state->length] = '\0';
    }
}

/* Eliminar carácter en la posición del cursor (delete) */
void input_delete_char(InputState *state) {
    if (!state || state->cursor_pos >= state->length) return;

    memmove(&state->line[state->cursor_pos],
            &state->line[state->cursor_pos + 1],
            state->length - state->cursor_pos - 1);
    state->length--;
    state->line[state->length] = '\0';
}

/* Mover cursor a la izquierda */
void input_move_left(InputState *state) {
    if (!state || state->cursor_pos == 0) return;
    state->cursor_pos--;
}

/* Mover cursor a la derecha */
void input_move_right(InputState *state) {
    if (!state || state->cursor_pos >= state->length) return;
    state->cursor_pos++;
}

/* Mover cursor al inicio */
void input_move_home(InputState *state) {
    if (!state) return;
    state->cursor_pos = 0;
}

/* Mover cursor al final */
void input_move_end(InputState *state) {
    if (!state) return;
    state->cursor_pos = state->length;
}

/* Limpiar línea */
void input_clear_line(InputState *state) {
    if (!state) return;

    memset(state->line, 0, MAX_INPUT_LEN);
    state->cursor_pos = 0;
    state->length = 0;
}

/* Añadir línea al historial */
void input_history_add(InputState *state, const char *line) {
    if (!state || !line || line[0] == '\0') return;

    /* Evitar duplicados consecutivos */
    if (state->history.count > 0) {
        int last = (state->history.current - 1 + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;
        if (strcmp(state->history.history[last], line) == 0) {
            return;
        }
    }

    /* Añadir al historial circular */
    strncpy(state->history.history[state->history.current], line, MAX_INPUT_LEN - 1);
    state->history.history[state->history.current][MAX_INPUT_LEN - 1] = '\0';

    state->history.current = (state->history.current + 1) % COMMAND_HISTORY_SIZE;

    if (state->history.count < COMMAND_HISTORY_SIZE) {
        state->history.count++;
    }

    state->history.position = -1;
}

/* Navegar al comando anterior en el historial */
void input_history_prev(InputState *state) {
    if (!state || state->history.count == 0) return;

    if (state->history.position == -1) {
        /* Primera vez navegando: empezar desde el último comando */
        state->history.position = (state->history.current - 1 + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;
    } else {
        /* Ir al comando anterior */
        int prev = (state->history.position - 1 + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;

        /* Verificar que no volvamos al principio */
        int oldest = (state->history.current - state->history.count + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;
        if (prev == oldest && state->history.count == COMMAND_HISTORY_SIZE) {
            return;
        }

        if (state->history.count < COMMAND_HISTORY_SIZE || prev != state->history.current) {
            state->history.position = prev;
        }
    }

    /* Copiar comando al buffer de entrada */
    strncpy(state->line, state->history.history[state->history.position], MAX_INPUT_LEN - 1);
    state->line[MAX_INPUT_LEN - 1] = '\0';
    state->length = strlen(state->line);
    state->cursor_pos = state->length;
}

/* Navegar al comando siguiente en el historial */
void input_history_next(InputState *state) {
    if (!state || state->history.position == -1) return;

    int next = (state->history.position + 1) % COMMAND_HISTORY_SIZE;

    if (next == state->history.current) {
        /* Llegamos al final: limpiar línea */
        input_clear_line(state);
        state->history.position = -1;
    } else {
        state->history.position = next;
        strncpy(state->line, state->history.history[state->history.position], MAX_INPUT_LEN - 1);
        state->line[MAX_INPUT_LEN - 1] = '\0';
        state->length = strlen(state->line);
        state->cursor_pos = state->length;
    }
}

/* Obtener línea actual */
char* input_get_line(InputState *state) {
    if (!state) return NULL;
    return state->line;
}

/* Leer tecla del terminal */
int input_read_key(void) {
    char c;
    int nread;

    /* Leer un carácter - select() ya garantizó que hay datos */
    nread = read(STDIN_FILENO, &c, 1);

    if (nread != 1) return -1;

    /* Detectar secuencias de escape */
    if (c == '\033') {
        char seq[5];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;

        /* Alt + número (ESC seguido de dígito) */
        if (seq[0] >= '0' && seq[0] <= '9') {
            switch (seq[0]) {
                case '0': return KEY_ALT_0;
                case '1': return KEY_ALT_1;
                case '2': return KEY_ALT_2;
                case '3': return KEY_ALT_3;
                case '4': return KEY_ALT_4;
                case '5': return KEY_ALT_5;
                case '6': return KEY_ALT_6;
                case '7': return KEY_ALT_7;
                case '8': return KEY_ALT_8;
                case '9': return KEY_ALT_9;
            }
        }

        /* Alt + . (ESC seguido de punto) */
        if (seq[0] == '.') {
            return KEY_ALT_PERIOD;
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

        if (seq[0] == '[') {
            /* Secuencias de teclas de cursor */
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return KEY_ESC;

                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '3': return KEY_DELETE;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                    }
                } else if (seq[2] == ';') {
                    /* Secuencias con modificadores (Ctrl, Ctrl+Shift, Alt) */
                    char mod[2];
                    if (read(STDIN_FILENO, &mod[0], 1) != 1) return KEY_ESC;
                    if (read(STDIN_FILENO, &mod[1], 1) != 1) return KEY_ESC;

                    if (mod[0] == '5') {
                        /* Ctrl modificador */
                        switch (mod[1]) {
                            case 'A': return KEY_CTRL_ARROW_UP;
                            case 'B': return KEY_CTRL_ARROW_DOWN;
                        }
                    } else if (mod[0] == '6') {
                        /* Ctrl+Shift modificador */
                        switch (mod[1]) {
                            case 'A': return KEY_CTRL_SHIFT_ARROW_UP;
                            case 'B': return KEY_CTRL_SHIFT_ARROW_DOWN;
                            case 'C': return KEY_CTRL_SHIFT_ARROW_RIGHT;
                            case 'D': return KEY_CTRL_SHIFT_ARROW_LEFT;
                        }
                    } else if (mod[0] == '3') {
                        /* Alt modificador */
                        switch (mod[1]) {
                            case 'C': return KEY_ALT_ARROW_RIGHT;
                            case 'D': return KEY_ALT_ARROW_LEFT;
                        }
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                }
            }
        }

        return KEY_ESC;
    }

    return c;
}
