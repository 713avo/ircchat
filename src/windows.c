#include "windows.h"
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

/* Crear gestor de ventanas */
WindowManager* wm_create(void) {
    WindowManager *wm = malloc(sizeof(WindowManager));
    if (!wm) return NULL;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        wm->windows[i] = NULL;
    }

    wm->active_window = 0;
    wm->window_count = 0;

    /* Crear ventana de sistema (ID 0) */
    wm_create_window(wm, WIN_SYSTEM, "Sistema");

    return wm;
}

/* Destruir gestor de ventanas */
void wm_destroy(WindowManager *wm) {
    if (!wm) return;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm->windows[i]) {
            buffer_destroy(wm->windows[i]->buffer);
            window_clear_users(wm->windows[i]);
            free(wm->windows[i]);
        }
    }

    free(wm);
}

/* Crear una nueva ventana */
int wm_create_window(WindowManager *wm, WindowType type, const char *title) {
    if (!wm || wm->window_count >= MAX_WINDOWS) return -1;

    /* Encontrar el primer slot libre */
    int id = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!wm->windows[i]) {
            id = i;
            break;
        }
    }

    if (id == -1) return -1;

    Window *win = malloc(sizeof(Window));
    if (!win) return -1;

    win->id = id;
    win->type = type;
    strncpy(win->title, title, MAX_CHANNEL_LEN - 1);
    win->title[MAX_CHANNEL_LEN - 1] = '\0';
    win->buffer = buffer_create();
    win->users = NULL;
    win->user_count = 0;
    win->user_scroll_offset = 0;
    win->topic[0] = '\0';
    win->has_unread = false;
    win->is_new = (type == WIN_PRIVATE); /* Mensajes privados son nuevos por defecto */
    win->log_file = NULL;
    win->log_enabled = false;
    win->last_log_day = 0;
    /* Inicializar campos de lista */
    win->channel_list = NULL;
    win->channel_count = 0;
    win->list_receiving = false;
    win->list_ordered = false;
    win->list_filter[0] = '\0';
    win->list_limit = 0;

    wm->windows[id] = win;
    wm->window_count++;

    return id;
}

/* Cerrar una ventana */
void wm_close_window(WindowManager *wm, int id) {
    if (!wm || id < 0 || id >= MAX_WINDOWS) return;
    if (!wm->windows[id]) return;

    Window *win = wm->windows[id];

    /* Cerrar archivo de log si está abierto */
    if (win->log_file) {
        window_close_log(win);
    }

    /* Destruir buffer y usuarios */
    buffer_destroy(win->buffer);
    window_clear_users(win);

    /* Limpiar lista de canales si existe */
    if (win->type == WIN_LIST) {
        window_clear_channel_list(win);
    }

    free(win);

    wm->windows[id] = NULL;
    wm->window_count--;

    /* Si cerramos la ventana activa, cambiar a la ventana de sistema */
    if (wm->active_window == id) {
        wm->active_window = 0;
    }
}

/* Cambiar a una ventana */
void wm_switch_to(WindowManager *wm, int id) {
    if (!wm || id < 0 || id >= MAX_WINDOWS) return;
    if (!wm->windows[id]) return;

    wm->active_window = id;

    /* Limpiar flags al cambiar a la ventana */
    Window *win = wm->windows[id];
    if (win) {
        win->has_unread = false;
        win->is_new = false;
    }
}

/* Obtener una ventana por ID */
Window* wm_get_window(WindowManager *wm, int id) {
    if (!wm || id < 0 || id >= MAX_WINDOWS) return NULL;
    return wm->windows[id];
}

/* Obtener la ventana activa */
Window* wm_get_active_window(WindowManager *wm) {
    if (!wm) return NULL;
    return wm->windows[wm->active_window];
}

/* Añadir mensaje a una ventana específica con timestamps opcionales */
void wm_add_message_with_timestamp(WindowManager *wm, int window_id, const char *msg, bool add_timestamp, const char *format) {
    if (!wm || window_id < 0 || window_id >= MAX_WINDOWS) return;

    Window *win = wm->windows[window_id];
    if (win && win->buffer) {
        /* Convertir códigos mIRC a ANSI */
        char converted_msg[MAX_MSG_LEN * 2];
        convert_mirc_to_ansi(converted_msg, msg, sizeof(converted_msg));

        /* Añadir timestamp si está habilitado y la ventana es de canal o privado */
        char final_msg[MAX_MSG_LEN * 2];
        if (add_timestamp && (win->type == WIN_CHANNEL || win->type == WIN_PRIVATE)) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);

            char timestamp[16];
            if (format && strcmp(format, "HH:MM") == 0) {
                snprintf(timestamp, sizeof(timestamp), "%02d:%02d",
                         tm_info->tm_hour, tm_info->tm_min);
            } else {
                /* Por defecto HH:MM:SS */
                snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d",
                         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }

            snprintf(final_msg, sizeof(final_msg), ANSI_GRAY "%s>" ANSI_RESET " %s",
                     timestamp, converted_msg);
        } else {
            strncpy(final_msg, converted_msg, sizeof(final_msg) - 1);
            final_msg[sizeof(final_msg) - 1] = '\0';
        }

        buffer_add_message(win->buffer, final_msg);

        /* Escribir al log si está habilitado (siempre sin el timestamp de visualización) */
        if (win->log_enabled && win->log_file) {
            window_write_log(win, converted_msg);
        }
    }
}

/* Añadir mensaje a una ventana específica (sin timestamp) */
void wm_add_message(WindowManager *wm, int window_id, const char *msg) {
    wm_add_message_with_timestamp(wm, window_id, msg, false, NULL);
}

/* Añadir mensaje a la ventana activa */
void wm_add_message_to_active(WindowManager *wm, const char *msg) {
    if (!wm) return;
    wm_add_message(wm, wm->active_window, msg);
}

/* Obtener valor de orden para modo de usuario (menor = más privilegio) */
static int get_mode_priority(char mode) {
    switch (mode) {
        case '~': return 0;  /* Owner */
        case '&': return 1;  /* Admin */
        case '@': return 2;  /* Operator */
        case '%': return 3;  /* Half-op */
        case '+': return 4;  /* Voice */
        default:  return 5;  /* Normal */
    }
}

/* Comparar dos usuarios para ordenamiento
 * Retorna: < 0 si user1 debe ir antes que user2
 *          > 0 si user2 debe ir antes que user1
 *            0 si son iguales
 */
static int compare_users(const UserNode *user1, const UserNode *user2) {
    /* Primero comparar por privilegio */
    int prio1 = get_mode_priority(user1->mode);
    int prio2 = get_mode_priority(user2->mode);

    if (prio1 != prio2) {
        return prio1 - prio2;
    }

    /* Si tienen mismo privilegio, ordenar alfabéticamente (case-insensitive) */
    return strcasecmp(user1->nick, user2->nick);
}

/* Añadir usuario a un canal */
void window_add_user(Window *win, const char *nick) {
    window_add_user_with_mode(win, nick, ' ');
}

/* Añadir usuario a un canal con prefijo de modo */
void window_add_user_with_mode(Window *win, const char *nick, char mode) {
    if (!win || !nick || win->type != WIN_CHANNEL) return;

    /* Verificar que el usuario no exista ya */
    UserNode *current = win->users;
    UserNode *prev = NULL;

    while (current) {
        if (strcmp(current->nick, nick) == 0) {
            /* Usuario ya existe - actualizar modo si cambió */
            if (current->mode != mode) {
                current->mode = mode;

                /* Reordenar: eliminar de posición actual */
                if (prev) {
                    prev->next = current->next;
                } else {
                    win->users = current->next;
                }

                /* Re-insertar en posición correcta */
                UserNode *node = current;
                node->next = NULL;

                /* Buscar posición de inserción */
                UserNode *insert_prev = NULL;
                UserNode *insert_curr = win->users;

                while (insert_curr && compare_users(insert_curr, node) < 0) {
                    insert_prev = insert_curr;
                    insert_curr = insert_curr->next;
                }

                /* Insertar en la posición encontrada */
                if (insert_prev) {
                    insert_prev->next = node;
                } else {
                    win->users = node;
                }
                node->next = insert_curr;
            }
            return;
        }
        prev = current;
        current = current->next;
    }

    /* Usuario no existe - crear nuevo nodo */
    UserNode *node = malloc(sizeof(UserNode));
    if (!node) return;

    strncpy(node->nick, nick, MAX_NICK_LEN - 1);
    node->nick[MAX_NICK_LEN - 1] = '\0';
    node->mode = mode;
    node->next = NULL;

    /* Insertar en orden */
    UserNode *insert_prev = NULL;
    UserNode *insert_curr = win->users;

    while (insert_curr && compare_users(insert_curr, node) < 0) {
        insert_prev = insert_curr;
        insert_curr = insert_curr->next;
    }

    /* Insertar en la posición encontrada */
    if (insert_prev) {
        insert_prev->next = node;
    } else {
        win->users = node;
    }
    node->next = insert_curr;

    win->user_count++;
}

/* Eliminar usuario de un canal */
void window_remove_user(Window *win, const char *nick) {
    if (!win || !nick || win->type != WIN_CHANNEL) return;

    UserNode *current = win->users;
    UserNode *prev = NULL;

    while (current) {
        if (strcmp(current->nick, nick) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                win->users = current->next;
            }
            free(current);
            win->user_count--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

/* Limpiar todos los usuarios de un canal */
void window_clear_users(Window *win) {
    if (!win) return;

    UserNode *current = win->users;
    while (current) {
        UserNode *next = current->next;
        free(current);
        current = next;
    }

    win->users = NULL;
    win->user_count = 0;
    win->user_scroll_offset = 0;
}

/* Scroll hacia arriba en la lista de usuarios */
void window_scroll_users_up(Window *win) {
    if (!win || win->type != WIN_CHANNEL) return;

    /* Incrementar offset (mostrar usuarios más arriba en la lista) */
    if (win->user_scroll_offset < win->user_count - 1) {
        win->user_scroll_offset++;
    }
}

/* Scroll hacia abajo en la lista de usuarios */
void window_scroll_users_down(Window *win) {
    if (!win || win->type != WIN_CHANNEL) return;

    /* Decrementar offset (volver hacia el final de la lista) */
    if (win->user_scroll_offset > 0) {
        win->user_scroll_offset--;
    }
}

/* Marcar ventana con actividad si no está activa */
void wm_mark_window_activity(WindowManager *wm, int window_id) {
    if (!wm || window_id < 0 || window_id >= MAX_WINDOWS) return;

    Window *win = wm->windows[window_id];
    if (!win) return;

    /* Solo marcar si no es la ventana activa */
    if (window_id != wm->active_window) {
        win->has_unread = true;
    }
}

/* Verificar si hay ventanas privadas nuevas */
bool wm_has_new_privates(WindowManager *wm) {
    if (!wm) return false;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *win = wm->windows[i];
        if (win && win->type == WIN_PRIVATE && win->is_new) {
            return true;
        }
    }
    return false;
}

/* Verificar si hay ventanas con mensajes sin leer */
bool wm_has_unread_messages(WindowManager *wm) {
    if (!wm) return false;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *win = wm->windows[i];
        if (win && win->has_unread && !win->is_new) {
            return true;
        }
    }
    return false;
}

/* Función auxiliar para remover códigos ANSI de una cadena */
static void strip_ansi_codes(char *dest, const char *src, size_t dest_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dest_size - 1; i++) {
        if (src[i] == '\033' && src[i+1] == '[') {
            /* Saltar secuencia ANSI */
            i += 2;
            while (src[i] && !isalpha((unsigned char)src[i])) {
                i++;
            }
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

/* Abrir archivo de log para una ventana */
void window_open_log(Window *win) {
    if (!win || win->log_file) return;

    /* Obtener HOME */
    const char *home = getenv("HOME");
    if (!home) return;

    /* Crear directorio ~/.irclogs/ si no existe */
    char logdir[512];
    snprintf(logdir, sizeof(logdir), "%s/.irclogs", home);
    mkdir(logdir, 0755);

    /* Obtener fecha y hora actual */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    /* Formatear nombre de archivo basado en tipo de ventana */
    char logpath[1024];
    char safe_title[MAX_CHANNEL_LEN];
    
    /* Limpiar el título para usarlo como nombre de archivo */
    strncpy(safe_title, win->title, MAX_CHANNEL_LEN - 1);
    safe_title[MAX_CHANNEL_LEN - 1] = '\0';
    
    /* Reemplazar # por nada si es canal */
    char *title_ptr = safe_title;
    if (safe_title[0] == '#') {
        title_ptr = safe_title + 1;
    }

    /* Determinar prefijo según tipo */
    const char *type_prefix = "";
    if (win->type == WIN_CHANNEL) {
        type_prefix = "canal_";
    } else if (win->type == WIN_PRIVATE) {
        type_prefix = "privado_";
    } else {
        type_prefix = "sistema_";
    }

    /* Crear nombre de archivo: tipo_nombre_DD-MM-YY_HH:MM.txt */
    snprintf(logpath, sizeof(logpath), "%s/%s%s_%02d-%02d-%02d_%02d:%02d.txt",
             logdir, type_prefix, title_ptr,
             tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year % 100,
             tm_info->tm_hour, tm_info->tm_min);

    /* Abrir archivo en modo append */
    win->log_file = fopen(logpath, "a");
    if (win->log_file) {
        win->log_enabled = true;
        win->last_log_day = tm_info->tm_mday;
        /* Escribir marca de inicio */
        fprintf(win->log_file, "=== Sesión iniciada: %02d/%02d/%04d %02d:%02d:%02d ===\n",
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        fflush(win->log_file);
    }
}

/* Cerrar archivo de log para una ventana */
void window_close_log(Window *win) {
    if (!win || !win->log_file) return;

    /* Escribir marca de cierre */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    fprintf(win->log_file, "=== Sesión cerrada: %02d/%02d/%04d %02d:%02d:%02d ===\n\n",
            tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    fclose(win->log_file);
    win->log_file = NULL;
    win->log_enabled = false;
}

/* Escribir mensaje al log de una ventana */
void window_write_log(Window *win, const char *msg) {
    if (!win || !win->log_file || !win->log_enabled || !msg) return;

    /* Obtener timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    /* Detectar cambio de día */
    if (win->last_log_day != 0 && tm_info->tm_mday != win->last_log_day) {
        fprintf(win->log_file, "\n=== Cambio de día: %02d/%02d/%04d ===\n\n",
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);
        win->last_log_day = tm_info->tm_mday;
    }

    /* Remover códigos ANSI del mensaje */
    char clean_msg[MAX_MSG_LEN];
    strip_ansi_codes(clean_msg, msg, sizeof(clean_msg));

    /* Escribir al log con timestamp (siempre en formato HH:MM:SS) */
    fprintf(win->log_file, "[%02d:%02d:%02d] %s\n",
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, clean_msg);
    fflush(win->log_file);
}

/* Mapeo de colores mIRC a ANSI */
static const char* mirc_color_to_ansi(int color) {
    switch (color) {
        case 0: return "\033[37m";      /* White */
        case 1: return "\033[30m";      /* Black */
        case 2: return "\033[34m";      /* Blue */
        case 3: return "\033[32m";      /* Green */
        case 4: return "\033[31m";      /* Red */
        case 5: return "\033[31m";      /* Brown/Maroon (Red oscuro) */
        case 6: return "\033[35m";      /* Purple/Magenta */
        case 7: return "\033[33m";      /* Orange (Yellow) */
        case 8: return "\033[93m";      /* Yellow (bright) */
        case 9: return "\033[92m";      /* Light Green */
        case 10: return "\033[36m";     /* Cyan */
        case 11: return "\033[96m";     /* Light Cyan */
        case 12: return "\033[94m";     /* Light Blue */
        case 13: return "\033[95m";     /* Pink/Magenta (bright) */
        case 14: return "\033[90m";     /* Grey */
        case 15: return "\033[37m";     /* Light Grey (White) */
        default: return "\033[0m";      /* Reset */
    }
}

/* Convertir códigos mIRC a ANSI 
 * Soporta:
 * - ^C (0x03) + números para colores
 * - ^B (0x02) para negrita
 * - ^U (0x1F) para subrayado
 * - ^R (0x0F) para reset
 */
void convert_mirc_to_ansi(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return;

    size_t j = 0;
    for (size_t i = 0; src[i] && j < dest_size - 50; i++) {  /* -50 para espacio de códigos ANSI */
        /* Color code: ^C */
        if (src[i] == 0x03) {
            i++;
            if (isdigit((unsigned char)src[i])) {
                int fg = src[i] - '0';
                i++;
                if (isdigit((unsigned char)src[i])) {
                    fg = fg * 10 + (src[i] - '0');
                    i++;
                }

                /* Aplicar color foreground */
                const char *color = mirc_color_to_ansi(fg);
                while (*color && j < dest_size - 1) {
                    dest[j++] = *color++;
                }

                /* Verificar si hay color de fondo (,NN) */
                if (src[i] == ',' && isdigit((unsigned char)src[i+1])) {
                    i++;  /* Saltar coma */
                    i++;  /* Por ahora ignoramos color de fondo */
                    if (isdigit((unsigned char)src[i])) {
                        i++;
                    }
                }
                i--;  /* Ajustar porque el for hará i++ */
            } else {
                /* ^C sin número = reset color */
                const char *reset = "\033[0m";
                while (*reset && j < dest_size - 1) {
                    dest[j++] = *reset++;
                }
                i--;
            }
        }
        /* Bold: ^B */
        else if (src[i] == 0x02) {
            const char *bold = "\033[1m";
            while (*bold && j < dest_size - 1) {
                dest[j++] = *bold++;
            }
        }
        /* Underline: ^U o ^_ */
        else if (src[i] == 0x1F) {
            const char *underline = "\033[4m";
            while (*underline && j < dest_size - 1) {
                dest[j++] = *underline++;
            }
        }
        /* Reset: ^R o ^O */
        else if (src[i] == 0x0F) {
            const char *reset = "\033[0m";
            while (*reset && j < dest_size - 1) {
                dest[j++] = *reset++;
            }
        }
        /* Reverse: ^V */
        else if (src[i] == 0x16) {
            const char *reverse = "\033[7m";
            while (*reverse && j < dest_size - 1) {
                dest[j++] = *reverse++;
            }
        }
        /* Carácter normal */
        else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

/* Función para matching de wildcards (* y ?) */
static bool wildcard_match(const char *pattern, const char *text) {
    const char *p = pattern;
    const char *t = text;
    const char *star_p = NULL;
    const char *star_t = NULL;

    while (*t) {
        if (*p == '*') {
            /* Guardar posición del * para backtracking */
            star_p = p++;
            star_t = t;
        } else if (*p == '?' || tolower(*p) == tolower(*t)) {
            /* ? coincide con cualquier carácter, o coincidencia exacta */
            p++;
            t++;
        } else if (star_p) {
            /* Backtrack al último * */
            p = star_p + 1;
            t = ++star_t;
        } else {
            return false;
        }
    }

    /* Consumir * sobrantes al final */
    while (*p == '*') p++;

    return *p == '\0';
}

/* Añadir canal a la lista */
void window_add_channel_to_list(Window *win, const char *name, int users, const char *topic) {
    if (!win || win->type != WIN_LIST || !name) return;

    ChannelListItem *item = malloc(sizeof(ChannelListItem));
    if (!item) return;

    strncpy(item->name, name, MAX_CHANNEL_LEN - 1);
    item->name[MAX_CHANNEL_LEN - 1] = '\0';
    item->user_count = users;
    strncpy(item->topic, topic ? topic : "", sizeof(item->topic) - 1);
    item->topic[sizeof(item->topic) - 1] = '\0';
    item->next = win->channel_list;

    win->channel_list = item;
    win->channel_count++;
}

/* Limpiar lista de canales */
void window_clear_channel_list(Window *win) {
    if (!win || win->type != WIN_LIST) return;

    ChannelListItem *current = win->channel_list;
    while (current) {
        ChannelListItem *next = current->next;
        free(current);
        current = next;
    }

    win->channel_list = NULL;
    win->channel_count = 0;
}

/* Ordenar lista de canales por número de usuarios (mayor a menor) */
void window_sort_channel_list(Window *win) {
    if (!win || win->type != WIN_LIST || !win->channel_list) return;

    /* Bubble sort simple - suficiente para listas de canales */
    bool swapped;
    do {
        swapped = false;
        ChannelListItem **ptr = &win->channel_list;

        while (*ptr && (*ptr)->next) {
            ChannelListItem *current = *ptr;
            ChannelListItem *next = current->next;

            if (current->user_count < next->user_count) {
                /* Swap */
                current->next = next->next;
                next->next = current;
                *ptr = next;
                swapped = true;
            }

            ptr = &((*ptr)->next);
        }
    } while (swapped);

    win->list_ordered = true;
}

/* Filtrar lista de canales según patrón con wildcards */
void window_filter_channel_list(Window *win, const char *filter) {
    if (!win || win->type != WIN_LIST || !filter) return;

    /* Guardar filtro */
    strncpy(win->list_filter, filter, sizeof(win->list_filter) - 1);
    win->list_filter[sizeof(win->list_filter) - 1] = '\0';
}

/* Finalizar recepción de lista y actualizar vista */
void window_finalize_channel_list(Window *win) {
    if (!win || win->type != WIN_LIST) return;

    win->list_receiving = false;

    /* Aplicar filtro si existe */
    if (win->list_filter[0] != '\0') {
        ChannelListItem **ptr = &win->channel_list;
        int count = 0;

        while (*ptr) {
            if (!wildcard_match(win->list_filter, (*ptr)->name)) {
                /* No coincide, eliminar */
                ChannelListItem *to_remove = *ptr;
                *ptr = (*ptr)->next;
                free(to_remove);
            } else {
                /* Coincide, mantener */
                ptr = &((*ptr)->next);
                count++;
            }
        }

        win->channel_count = count;
    }

    /* Ordenar si se solicitó */
    if (win->list_ordered) {
        window_sort_channel_list(win);
    }

    /* Aplicar límite si se especificó - DESPUÉS de ordenar para obtener el top N */
    if (win->list_limit > 0 && win->channel_count > win->list_limit) {
        ChannelListItem *item = win->channel_list;
        int count = 0;

        /* Avanzar hasta el límite */
        while (item && count < win->list_limit) {
            item = item->next;
            count++;
        }

        /* Liberar el resto de elementos */
        if (item) {
            /* Cortar la lista */
            ChannelListItem *prev = win->channel_list;
            for (int i = 0; i < win->list_limit - 1 && prev; i++) {
                prev = prev->next;
            }
            if (prev) {
                prev->next = NULL;
            }

            /* Liberar elementos sobrantes */
            while (item) {
                ChannelListItem *to_free = item;
                item = item->next;
                free(to_free);
            }

            win->channel_count = win->list_limit;
        }
    }

    /* Mostrar resumen en el buffer */
    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_BOLD ANSI_CYAN "=== Lista de canales completada ===" ANSI_RESET);
    buffer_add_message(win->buffer, msg);

    snprintf(msg, sizeof(msg), ANSI_GREEN "Total: %d canales" ANSI_RESET, win->channel_count);
    buffer_add_message(win->buffer, msg);

    if (win->list_filter[0] != '\0') {
        snprintf(msg, sizeof(msg), ANSI_YELLOW "Filtro: %s" ANSI_RESET, win->list_filter);
        buffer_add_message(win->buffer, msg);
    }

    if (win->list_ordered) {
        snprintf(msg, sizeof(msg), ANSI_YELLOW "Ordenado por usuarios (mayor a menor)" ANSI_RESET);
        buffer_add_message(win->buffer, msg);
    }

    if (win->list_limit > 0) {
        snprintf(msg, sizeof(msg), ANSI_YELLOW "Límite aplicado: %d resultados" ANSI_RESET, win->list_limit);
        buffer_add_message(win->buffer, msg);
    }

    buffer_add_message(win->buffer, "");

    /* Añadir canales al buffer - limitar a 500 para no saturar la memoria */
    ChannelListItem *item = win->channel_list;
    int displayed = 0;
    int max_display = 500;

    while (item && displayed < max_display) {
        /* Truncar topic si es muy largo para que quepa en una línea */
        char truncated_topic[256];
        /* Calcular espacio: nombre_canal + " [" + num_usuarios + "] " ≈ 30 chars */
        /* Dejar espacio para 80 columnas típicas: 80 - 30 = 50 chars para topic */
        int max_topic_len = 60;
        if ((int)strlen(item->topic) > max_topic_len) {
            strncpy(truncated_topic, item->topic, max_topic_len - 3);
            truncated_topic[max_topic_len - 3] = '\0';
            strcat(truncated_topic, "...");
        } else {
            strncpy(truncated_topic, item->topic, sizeof(truncated_topic) - 1);
            truncated_topic[sizeof(truncated_topic) - 1] = '\0';
        }

        snprintf(msg, sizeof(msg), ANSI_CYAN "%s" ANSI_RESET " [" ANSI_GREEN "%d" ANSI_RESET "] %s",
                 item->name, item->user_count, truncated_topic);
        buffer_add_message(win->buffer, msg);
        item = item->next;
        displayed++;
    }

    if (displayed >= max_display && item) {
        snprintf(msg, sizeof(msg), ANSI_YELLOW "... y %d canales más (usa /list con filtros o límite)" ANSI_RESET,
                 win->channel_count - displayed);
        buffer_add_message(win->buffer, msg);
    }
}
