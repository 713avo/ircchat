#ifndef WINDOWS_H
#define WINDOWS_H

#include "common.h"
#include "buffer.h"

/* Lista de usuarios en un canal */
typedef struct UserNode {
    char nick[MAX_NICK_LEN];
    char mode;                  /* @=op, +=voz, ' '=normal */
    struct UserNode *next;
} UserNode;

/* Item de lista de canales para ventana LIST */
typedef struct ChannelListItem {
    char name[MAX_CHANNEL_LEN];
    int user_count;
    char topic[512];
    struct ChannelListItem *next;
} ChannelListItem;

/* Estructura de ventana */
typedef struct {
    int id;
    WindowType type;
    char title[MAX_CHANNEL_LEN];
    MessageBuffer *buffer;
    UserNode *users;            /* Lista de usuarios (solo para canales) */
    int user_count;
    int user_scroll_offset;     /* Offset de scroll vertical para lista de usuarios */
    char topic[512];            /* Topic del canal (solo para WIN_CHANNEL) */
    bool has_unread;            /* Tiene mensajes sin leer */
    bool is_new;                /* Es una ventana nueva sin visitar */
    FILE *log_file;             /* Archivo de log si logging está habilitado */
    bool log_enabled;           /* Indica si el logging está habilitado para esta ventana */
    int last_log_day;           /* Último día en que se escribió al log (día del mes) */
    /* Campos para ventana LIST */
    ChannelListItem *channel_list;  /* Lista de canales (solo para WIN_LIST) */
    int channel_count;              /* Número de canales en la lista */
    bool list_receiving;            /* Indica si está recibiendo datos de LIST */
    bool list_ordered;              /* Indica si la lista está ordenada */
    char list_filter[256];          /* Filtro de búsqueda (con wildcards) */
    int list_limit;                 /* Límite de resultados (0 = sin límite) */
} Window;

/* Gestor de ventanas */
typedef struct {
    Window *windows[MAX_WINDOWS];
    int active_window;
    int window_count;
} WindowManager;

/* Funciones de gestión de ventanas */
WindowManager* wm_create(void);
void wm_destroy(WindowManager *wm);
int wm_create_window(WindowManager *wm, WindowType type, const char *title);
void wm_close_window(WindowManager *wm, int id);
void wm_switch_to(WindowManager *wm, int id);
Window* wm_get_window(WindowManager *wm, int id);
Window* wm_get_active_window(WindowManager *wm);
void wm_add_message(WindowManager *wm, int window_id, const char *msg);
void wm_add_message_to_active(WindowManager *wm, const char *msg);
void wm_add_message_with_timestamp(WindowManager *wm, int window_id, const char *msg, bool add_timestamp, const char *format);
void wm_mark_window_activity(WindowManager *wm, int window_id);
bool wm_has_new_privates(WindowManager *wm);
bool wm_has_unread_messages(WindowManager *wm);

/* Funciones de logging */
void window_open_log(Window *win);
void window_close_log(Window *win);
void window_write_log(Window *win, const char *msg);

/* Funciones de conversión de colores */
void convert_mirc_to_ansi(char *dest, const char *src, size_t dest_size);

/* Funciones para gestión de usuarios en canales */
void window_add_user(Window *win, const char *nick);
void window_add_user_with_mode(Window *win, const char *nick, char mode);
void window_remove_user(Window *win, const char *nick);
void window_clear_users(Window *win);
void window_scroll_users_up(Window *win);
void window_scroll_users_down(Window *win);

/* Funciones para ventana LIST */
void window_add_channel_to_list(Window *win, const char *name, int users, const char *topic);
void window_clear_channel_list(Window *win);
void window_sort_channel_list(Window *win);
void window_filter_channel_list(Window *win, const char *filter);
void window_finalize_channel_list(Window *win);

#endif /* WINDOWS_H */
