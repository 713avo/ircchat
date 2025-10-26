#include "terminal.h"
#include <unistd.h>
#include <sys/ioctl.h>

/* Dividir texto en líneas según ancho máximo preservando formatos ANSI
 * Retorna número de líneas generadas
 * lines debe ser un array de punteros a char con espacio suficiente
 * max_lines es el número máximo de líneas a generar
 */
static int wrap_text(const char *text, int max_width, char **lines, int max_lines) {
    if (!text || !lines || max_width <= 0 || max_lines <= 0) return 0;

    int line_count = 0;
    int text_len = strlen(text);
    int pos = 0;

    /* Buffer para almacenar códigos ANSI activos */
    char active_codes[512] = "";
    int active_codes_len = 0;

    while (pos < text_len && line_count < max_lines) {
        int line_start = pos;
        int line_end = pos;
        int visible_chars = 0;

        /* Avanzar hasta llenar el ancho máximo (contando solo caracteres visibles) */
        while (pos < text_len && visible_chars < max_width) {
            /* Detectar secuencia ANSI */
            if (text[pos] == '\033' && pos + 1 < text_len && text[pos + 1] == '[') {
                int ansi_start = pos;
                /* Saltar secuencia ANSI completa */
                pos += 2;
                /* Limitar búsqueda de 'm' a máximo 20 caracteres para evitar bucles infinitos */
                int search_limit = 20;
                while (pos < text_len && text[pos] != 'm' && search_limit > 0) {
                    pos++;
                    search_limit--;
                }

                /* Si encontramos la 'm', avanzar; si no, resetear a posición segura */
                if (pos < text_len && text[pos] == 'm') {
                    pos++; /* Saltar 'm' */
                } else {
                    /* Secuencia ANSI malformada, volver atrás y tratar como texto normal */
                    pos = ansi_start + 1;
                    line_end = pos;
                    continue;
                }

                int ansi_len = pos - ansi_start;

                /* Verificar si es un código de reset */
                if (ansi_len >= 4 && ansi_start + 3 < text_len &&
                    text[ansi_start + 2] == '0' && text[ansi_start + 3] == 'm') {
                    /* Reset: limpiar códigos activos */
                    active_codes[0] = '\0';
                    active_codes_len = 0;
                } else {
                    /* Guardar código ANSI en active_codes (evitar overflow) */
                    if (ansi_len > 0 && ansi_len < 50 &&
                        active_codes_len + ansi_len < (int)sizeof(active_codes) - 1) {
                        memcpy(active_codes + active_codes_len, text + ansi_start, ansi_len);
                        active_codes_len += ansi_len;
                        active_codes[active_codes_len] = '\0';
                    }
                }

                line_end = pos;
                continue;
            }

            /* Contar carácter visible UTF-8 */
            unsigned char c = text[pos];
            int char_bytes = 1;

            if ((c & 0x80) == 0x00) char_bytes = 1;
            else if ((c & 0xE0) == 0xC0) char_bytes = 2;
            else if ((c & 0xF0) == 0xE0) char_bytes = 3;
            else if ((c & 0xF8) == 0xF0) char_bytes = 4;

            /* Verificar que no nos salgamos del buffer */
            if (pos + char_bytes > text_len) {
                char_bytes = text_len - pos;
                if (char_bytes <= 0) break;
            }

            /* Verificar si el carácter cabe */
            if (visible_chars + 1 > max_width) break;

            pos += char_bytes;
            visible_chars++;
            line_end = pos;
        }

        /* Crear línea */
        int line_bytes = line_end - line_start;
        if (line_bytes > 0 && line_bytes < MAX_MSG_LEN) {  /* Validar tamaño razonable */
            /* Calcular tamaño total: códigos activos previos + contenido + reset ANSI */
            int prefix_len = (line_count > 0 && active_codes_len > 0) ? active_codes_len : 0;
            int total_size = prefix_len + line_bytes + 5; /* +5 para \033[0m + \0 */

            /* Validar que el tamaño total sea razonable */
            if (total_size > 0 && total_size < MAX_MSG_LEN * 2) {
                lines[line_count] = malloc(total_size);
                if (lines[line_count]) {
                    char *ptr = lines[line_count];
                    char *end_ptr = lines[line_count] + total_size;

                    /* Si no es la primera línea, añadir códigos activos al inicio */
                    if (line_count > 0 && active_codes_len > 0 && ptr + active_codes_len < end_ptr) {
                        memcpy(ptr, active_codes, active_codes_len);
                        ptr += active_codes_len;
                    }

                    /* Copiar contenido de la línea con verificación de límites */
                    if (line_start >= 0 && line_end <= text_len &&
                        line_bytes > 0 && ptr + line_bytes < end_ptr) {
                        memcpy(ptr, text + line_start, line_bytes);
                        ptr += line_bytes;
                    }

                    /* Añadir reset ANSI al final */
                    if (ptr + 5 <= end_ptr) {
                        memcpy(ptr, "\033[0m", 4);
                        ptr += 4;
                        *ptr = '\0';
                    }

                    line_count++;
                }
            }
        }

        /* Si no avanzamos, salir para evitar loop infinito */
        if (line_end == line_start) break;
    }

    return line_count;
}

/* Inicializar terminal */
void term_init(TerminalState *term) {
    if (!term) return;

    /* Guardar configuración original del terminal */
    tcgetattr(STDIN_FILENO, &term->original_termios);
    term->raw_mode = false;
    term->blink_state = false;       /* Inicializar estado de parpadeo */
    term->blink_frame_count = 0;     /* Inicializar contador de frames */

    /* Obtener tamaño del terminal */
    term_get_size(term);

    /* Configurar locale para UTF-8 */
    setlocale(LC_ALL, "");
}

/* Limpiar terminal y restaurar estado original */
void term_cleanup(TerminalState *term) {
    if (!term) return;

    if (term->raw_mode) {
        term_exit_raw_mode(term);
    }

    term_show_cursor();
    term_clear_screen();
    printf(ANSI_HOME);
    fflush(stdout);
}

/* Entrar en modo raw (sin buffering, sin echo) */
void term_enter_raw_mode(TerminalState *term) {
    if (!term || term->raw_mode) return;

    struct termios raw = term->original_termios;

    /* Configurar modo raw */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);

    /* Configurar timeout para read */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    term->raw_mode = true;
}

/* Salir del modo raw */
void term_exit_raw_mode(TerminalState *term) {
    if (!term || !term->raw_mode) return;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term->original_termios);
    term->raw_mode = false;
}

/* Obtener tamaño del terminal */
void term_get_size(TerminalState *term) {
    if (!term) return;

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        term->rows = 24;
        term->cols = 80;
    } else {
        term->rows = ws.ws_row;
        term->cols = ws.ws_col;
    }
}

/* Limpiar pantalla */
void term_clear_screen(void) {
    printf(ANSI_CLEAR_SCREEN);
    printf(ANSI_HOME);
    fflush(stdout);
}

/* Mover cursor a una posición */
void term_move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

/* Ocultar cursor */
void term_hide_cursor(void) {
    printf(ANSI_HIDE_CURSOR);
    fflush(stdout);
}

/* Mostrar cursor */
void term_show_cursor(void) {
    printf(ANSI_SHOW_CURSOR);
    fflush(stdout);
}

/* Dibujar línea horizontal */
void term_draw_horizontal_line(int row, int width) {
    term_move_cursor(row, 1);
    for (int i = 0; i < width; i++) {
        printf(UNICODE_HLINE);
    }
    fflush(stdout);
}

/* Dibujar línea vertical */
void term_draw_vertical_line(int col, int start_row, int end_row) {
    for (int row = start_row; row <= end_row; row++) {
        term_move_cursor(row, col);
        printf(UNICODE_VLINE);
    }
    fflush(stdout);
}

/* Dibujar separador horizontal */
void term_draw_separator(int col_width) {
    for (int i = 0; i < col_width; i++) {
        printf(UNICODE_HLINE);
    }
}

/* Dibujar interfaz completa */
void term_draw_interface(TerminalState *term, WindowManager *wm, const char *input_line, int cursor_pos, bool notify_alert, bool mention_alert) {
    if (!term || !wm) return;

    /* Alternar estado de parpadeo cada 10 frames (aproximadamente 1 segundo) */
    term->blink_frame_count++;
    if (term->blink_frame_count >= 10) {
        term->blink_state = !term->blink_state;
        term->blink_frame_count = 0;
    }

    term_hide_cursor();
    term_clear_screen();

    Window *active_win = wm_get_active_window(wm);
    if (!active_win) {
        term_show_cursor();
        return;
    }

    /* Dibujar contenido según el tipo de ventana */
    switch (active_win->type) {
        case WIN_SYSTEM:
        case WIN_PRIVATE:
        case WIN_LIST:
            term_draw_system_window(term, active_win);
            break;
        case WIN_CHANNEL:
            term_draw_channel_window(term, active_win);
            break;
    }

    /* Dibujar línea separadora */
    int separator_row = term->rows - 1;
    term_move_cursor(separator_row, 1);
    printf(ANSI_BOLD ANSI_CYAN);
    term_draw_separator(term->cols);
    printf(ANSI_RESET);

    /* Dibujar prompt */
    term_draw_prompt(term, wm, input_line, cursor_pos, notify_alert, mention_alert);

    term_show_cursor();
    fflush(stdout);
}

/* Dibujar ventana de sistema o privado */
void term_draw_system_window(TerminalState *term, Window *win) {
    if (!term || !win) return;

    /* Título de la ventana */
    term_move_cursor(1, 1);
    printf(ANSI_BOLD ANSI_BLUE "[%s]" ANSI_RESET, win->title);

    int max_lines = term->rows - 3; /* Espacio disponible para mensajes */
    int max_width = term->cols - 2;

    /* Obtener una cantidad generosa de mensajes (200 o todos) */
    int msg_count = 0;
    char **messages = buffer_get_visible_messages(win->buffer, 200, &msg_count);

    if (!messages || msg_count == 0) {
        if (messages) free(messages);
        return;
    }

    /* Primero: hacer word wrap de TODOS los mensajes y contar líneas totales */
    char ***all_wrapped = malloc(sizeof(char**) * msg_count);
    int *wrapped_counts = malloc(sizeof(int) * msg_count);
    int total_lines = 0;

    for (int i = 0; i < msg_count; i++) {
        all_wrapped[i] = malloc(sizeof(char*) * 10);
        wrapped_counts[i] = wrap_text(messages[i], max_width, all_wrapped[i], 10);
        total_lines += wrapped_counts[i];
    }

    /* Calcular qué líneas mostrar (las últimas max_lines) */
    int skip_lines = (total_lines > max_lines) ? (total_lines - max_lines) : 0;

    /* Renderizar solo las líneas que caben en pantalla */
    int current_row = 2;
    int lines_skipped = 0;

    for (int i = 0; i < msg_count && current_row < term->rows - 1; i++) {
        for (int j = 0; j < wrapped_counts[i] && current_row < term->rows - 1; j++) {
            if (lines_skipped < skip_lines) {
                lines_skipped++;
            } else {
                term_move_cursor(current_row, 1);
                printf(ANSI_CLEAR_LINE "%s", all_wrapped[i][j]);
                current_row++;
            }
        }
    }

    /* Liberar memoria */
    for (int i = 0; i < msg_count; i++) {
        for (int j = 0; j < wrapped_counts[i]; j++) {
            free(all_wrapped[i][j]);
        }
        free(all_wrapped[i]);
    }
    free(all_wrapped);
    free(wrapped_counts);
    free(messages);
}

/* Dibujar ventana de canal */
void term_draw_channel_window(TerminalState *term, Window *win) {
    if (!term || !win) return;

    /* Calcular anchos */
    int user_list_width = 16;  /* Ancho de la lista de usuarios */
    int separator_col = term->cols - user_list_width;
    int chat_width = separator_col - 1;

    /* Título de la ventana con topic */
    term_move_cursor(1, 1);
    printf(ANSI_CLEAR_LINE);

    /* Calcular espacio disponible para el topic */
    char header[MAX_MSG_LEN];
    snprintf(header, sizeof(header), "[%s] (%d usuarios)", win->title, win->user_count);
    int header_len = strlen(header);
    int available_space = chat_width - header_len - 3;  /* -3 para " | " */

    if (win->topic[0] != '\0' && available_space > 20) {
        /* Truncar topic si es necesario */
        char truncated_topic[256];
        if ((int)strlen(win->topic) > available_space) {
            strncpy(truncated_topic, win->topic, available_space - 3);
            truncated_topic[available_space - 3] = '\0';
            strcat(truncated_topic, "...");
        } else {
            strncpy(truncated_topic, win->topic, sizeof(truncated_topic) - 1);
            truncated_topic[sizeof(truncated_topic) - 1] = '\0';
        }

        printf(ANSI_BOLD ANSI_GREEN "[%s]" ANSI_RESET " (%d usuarios) " ANSI_GRAY "| %s" ANSI_RESET,
               win->title, win->user_count, truncated_topic);
    } else {
        printf(ANSI_BOLD ANSI_GREEN "[%s]" ANSI_RESET " (%d usuarios)", win->title, win->user_count);
    }

    int max_lines = term->rows - 3;
    int max_msg_width = chat_width - 1;

    /* Obtener una cantidad generosa de mensajes */
    int msg_count = 0;
    char **messages = buffer_get_visible_messages(win->buffer, 200, &msg_count);

    if (messages && msg_count > 0) {
        /* Hacer word wrap de TODOS los mensajes y contar líneas totales */
        char ***all_wrapped = malloc(sizeof(char**) * msg_count);
        int *wrapped_counts = malloc(sizeof(int) * msg_count);
        int total_lines = 0;

        for (int i = 0; i < msg_count; i++) {
            all_wrapped[i] = malloc(sizeof(char*) * 10);
            wrapped_counts[i] = wrap_text(messages[i], max_msg_width, all_wrapped[i], 10);
            total_lines += wrapped_counts[i];
        }

        /* Calcular qué líneas mostrar (las últimas max_lines) */
        int skip_lines = (total_lines > max_lines) ? (total_lines - max_lines) : 0;

        /* Renderizar solo las líneas que caben */
        int current_row = 2;
        int lines_skipped = 0;

        for (int i = 0; i < msg_count && current_row < term->rows - 1; i++) {
            for (int j = 0; j < wrapped_counts[i] && current_row < term->rows - 1; j++) {
                if (lines_skipped < skip_lines) {
                    lines_skipped++;
                } else {
                    term_move_cursor(current_row, 1);
                    printf(ANSI_CLEAR_LINE "%s", all_wrapped[i][j]);
                    current_row++;
                }
            }
        }

        /* Liberar memoria */
        for (int i = 0; i < msg_count; i++) {
            for (int j = 0; j < wrapped_counts[i]; j++) {
                free(all_wrapped[i][j]);
            }
            free(all_wrapped[i]);
        }
        free(all_wrapped);
        free(wrapped_counts);
        free(messages);
    }

    /* Dibujar línea vertical separadora */
    term_draw_vertical_line(separator_col, 2, term->rows - 2);

    /* Dibujar unión en el separador horizontal */
    term_move_cursor(term->rows - 1, separator_col);
    printf(UNICODE_JUNCTION);

    /* Dibujar lista de usuarios */
    int user_row = 2;
    int max_user_rows = term->rows - 3;
    int max_nick_display = user_list_width - 4; /* Espacio para nick (descontando márgenes) */

    term_move_cursor(user_row, separator_col + 2);
    printf(ANSI_BOLD "Usuarios:" ANSI_RESET);

    /* Verificar si hay nicks largos para mostrar indicador */
    UserNode *check = win->users;
    bool has_long_nicks = false;
    while (check) {
        if ((int)strlen(check->nick) > max_nick_display) {
            has_long_nicks = true;
            break;
        }
        check = check->next;
    }
    if (has_long_nicks) {
        printf(" " ANSI_YELLOW "→" ANSI_RESET);
    }

    user_row++;

    /* Aplicar scroll vertical - saltar usuarios al inicio */
    UserNode *user = win->users;
    int skip = win->user_scroll_offset;
    while (user && skip > 0) {
        user = user->next;
        skip--;
    }

    /* Dibujar usuarios visibles */
    while (user && user_row <= max_user_rows + 1) {
        term_move_cursor(user_row, separator_col + 2);

        /* Mostrar prefijo de modo si existe */
        const char *color = ANSI_GRAY;
        char prefix = ' ';

        if (user->mode == '@') {
            prefix = '@';
            color = ANSI_GREEN;  /* Operadores en verde */
        } else if (user->mode == '+') {
            prefix = '+';
            color = ANSI_CYAN;   /* Voz en cyan */
        } else if (user->mode == '%') {
            prefix = '%';
            color = ANSI_BLUE;   /* Half-op en azul */
        } else if (user->mode == '~') {
            prefix = '~';
            color = ANSI_RED;    /* Owner en rojo */
        } else if (user->mode == '&') {
            prefix = '&';
            color = ANSI_MAGENTA; /* Admin en magenta */
        }

        /* Truncar nick si es más largo que el espacio disponible */
        char display_nick[MAX_NICK_LEN];
        int nick_len = strlen(user->nick);
        if (nick_len > max_nick_display) {
            strncpy(display_nick, user->nick, max_nick_display);
            display_nick[max_nick_display] = '\0';
        } else {
            strncpy(display_nick, user->nick, MAX_NICK_LEN - 1);
            display_nick[MAX_NICK_LEN - 1] = '\0';
        }

        if (prefix != ' ') {
            printf("%s%c%s" ANSI_RESET, color, prefix, display_nick);
        } else {
            printf("%s%s" ANSI_RESET, color, display_nick);
        }

        user = user->next;
        user_row++;
    }

    /* Indicador de scroll si hay más usuarios */
    if (win->user_scroll_offset > 0) {
        term_move_cursor(2, separator_col + user_list_width - 1);
        printf(ANSI_YELLOW "↑" ANSI_RESET);
    }
    if (user != NULL) {
        term_move_cursor(max_user_rows + 1, separator_col + user_list_width - 1);
        printf(ANSI_YELLOW "↓" ANSI_RESET);
    }
}

/* Dibujar ventana privada (similar a sistema) */
void term_draw_private_window(TerminalState *term, Window *win) {
    term_draw_system_window(term, win);
}

/* Dibujar prompt de entrada */
void term_draw_prompt(TerminalState *term, WindowManager *wm, const char *input_line, int cursor_pos, bool notify_alert, bool mention_alert) {
    if (!term) return;

    int prompt_row = term->rows;
    term_move_cursor(prompt_row, 1);
    printf(ANSI_CLEAR_LINE);

    /* Mostrar prompt */
    printf(ANSI_BOLD ANSI_YELLOW "> " ANSI_RESET);

    /* Mostrar línea de entrada */
    if (input_line) {
        printf("%s", input_line);
    }

    /* Mostrar indicadores de actividad al final de la línea */
    if (wm) {
        /* Construir string con todos los indicadores activos */
        char indicators[32] = "";
        int ind_count = 0;

        /* C para nicks conectados (notify alert) - verde */
        if (notify_alert) {
            if (term->blink_state) {
                strcat(indicators, "\033[1;7;32mC\033[0m ");  /* Bold + Reverse + verde */
            } else {
                strcat(indicators, "\033[1;32mC\033[0m ");     /* Bold + verde */
            }
            ind_count++;
        }

        /* M para menciones en canales - magenta */
        if (mention_alert) {
            if (term->blink_state) {
                strcat(indicators, "\033[1;7;35mM\033[0m ");  /* Bold + Reverse + magenta */
            } else {
                strcat(indicators, "\033[1;35mM\033[0m ");     /* Bold + magenta */
            }
            ind_count++;
        }

        /* * para ventanas privadas nuevas - rojo */
        if (wm_has_new_privates(wm)) {
            if (term->blink_state) {
                strcat(indicators, "\033[1;7;31m*\033[0m ");  /* Bold + Reverse + rojo */
            } else {
                strcat(indicators, "\033[1;31m*\033[0m ");     /* Bold + rojo */
            }
            ind_count++;
        }

        /* + para ventanas con mensajes sin leer - amarillo */
        if (wm_has_unread_messages(wm)) {
            if (term->blink_state) {
                strcat(indicators, "\033[1;7;33m+\033[0m ");  /* Bold + Reverse + amarillo */
            } else {
                strcat(indicators, "\033[1;33m+\033[0m ");     /* Bold + amarillo */
            }
            ind_count++;
        }

        /* Mostrar todos los indicadores si hay alguno */
        if (ind_count > 0) {
            /* Calcular columna considerando número de indicadores (cada uno ocupa 2 espacios) */
            int indicator_col = term->cols - (ind_count * 2);
            term_move_cursor(prompt_row, indicator_col);
            printf("%s", indicators);
        }
    }

    /* Posicionar cursor */
    term_move_cursor(prompt_row, 3 + cursor_pos);
}
