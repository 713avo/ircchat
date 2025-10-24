#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

/* Nodo de la lista de mensajes */
typedef struct MessageNode {
    char *message;
    struct MessageNode *next;
    struct MessageNode *prev;
} MessageNode;

/* Buffer de mensajes */
typedef struct {
    MessageNode *head;
    MessageNode *tail;
    MessageNode *current_view;  /* Para navegación con Ctrl-Arriba/Abajo */
    int count;
    int view_offset;            /* Offset desde el final del buffer */
    bool enabled;               /* Buffer activado/desactivado */
} MessageBuffer;

/* Funciones del buffer */
MessageBuffer* buffer_create(void);
void buffer_destroy(MessageBuffer *buf);
void buffer_add_message(MessageBuffer *buf, const char *msg);
void buffer_clear(MessageBuffer *buf);
void buffer_scroll_up(MessageBuffer *buf);
void buffer_scroll_down(MessageBuffer *buf);
void buffer_scroll_top(MessageBuffer *buf);
void buffer_scroll_bottom(MessageBuffer *buf);
char** buffer_get_visible_messages(MessageBuffer *buf, int max_lines, int *count);

#endif /* BUFFER_H */
