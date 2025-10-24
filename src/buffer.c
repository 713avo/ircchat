#include "buffer.h"

/* Crear un nuevo buffer de mensajes */
MessageBuffer* buffer_create(void) {
    MessageBuffer *buf = malloc(sizeof(MessageBuffer));
    if (!buf) return NULL;

    buf->head = NULL;
    buf->tail = NULL;
    buf->current_view = NULL;
    buf->count = 0;
    buf->view_offset = 0;
    buf->enabled = true;

    return buf;
}

/* Destruir un buffer y liberar memoria */
void buffer_destroy(MessageBuffer *buf) {
    if (!buf) return;

    MessageNode *current = buf->head;
    while (current) {
        MessageNode *next = current->next;
        free(current->message);
        free(current);
        current = next;
    }

    free(buf);
}

/* Añadir un mensaje al buffer */
void buffer_add_message(MessageBuffer *buf, const char *msg) {
    if (!buf || !msg) return;

    if (!buf->enabled) {
        /* Si el buffer está desactivado, solo mantenemos el último mensaje */
        if (buf->head) {
            free(buf->head->message);
            buf->head->message = strdup(msg);
        } else {
            MessageNode *node = malloc(sizeof(MessageNode));
            if (!node) return;
            node->message = strdup(msg);
            node->next = NULL;
            node->prev = NULL;
            buf->head = buf->tail = node;
            buf->count = 1;
        }
        return;
    }

    MessageNode *node = malloc(sizeof(MessageNode));
    if (!node) return;

    node->message = strdup(msg);
    node->next = NULL;
    node->prev = buf->tail;

    if (buf->tail) {
        buf->tail->next = node;
    } else {
        buf->head = node;
    }

    buf->tail = node;
    buf->count++;

    /* Si no estamos navegando (view_offset == 0), mantener la vista al final */
    if (buf->view_offset == 0 || !buf->current_view) {
        buf->current_view = buf->tail;
    }
}

/* Limpiar todos los mensajes del buffer */
void buffer_clear(MessageBuffer *buf) {
    if (!buf) return;

    MessageNode *current = buf->head;
    while (current) {
        MessageNode *next = current->next;
        free(current->message);
        free(current);
        current = next;
    }

    buf->head = NULL;
    buf->tail = NULL;
    buf->current_view = NULL;
    buf->count = 0;
    buf->view_offset = 0;
}

/* Scroll hacia arriba (mensajes más antiguos) */
void buffer_scroll_up(MessageBuffer *buf) {
    if (!buf || !buf->enabled) return;

    if (buf->current_view && buf->current_view->prev) {
        buf->current_view = buf->current_view->prev;
        buf->view_offset++;
    }
}

/* Scroll hacia abajo (mensajes más recientes) */
void buffer_scroll_down(MessageBuffer *buf) {
    if (!buf || !buf->enabled) return;

    if (buf->current_view && buf->current_view->next) {
        buf->current_view = buf->current_view->next;
        buf->view_offset--;
        if (buf->view_offset < 0) buf->view_offset = 0;
    }
}

/* Ir al principio del buffer */
void buffer_scroll_top(MessageBuffer *buf) {
    if (!buf || !buf->enabled) return;

    buf->current_view = buf->head;
    buf->view_offset = buf->count > 0 ? buf->count - 1 : 0;
}

/* Ir al final del buffer */
void buffer_scroll_bottom(MessageBuffer *buf) {
    if (!buf || !buf->enabled) return;

    buf->current_view = buf->tail;
    buf->view_offset = 0;
}

/* Obtener mensajes visibles para renderizar */
char** buffer_get_visible_messages(MessageBuffer *buf, int max_messages, int *count) {
    if (!buf) {
        *count = 0;
        return NULL;
    }

    *count = 0;

    if (buf->count == 0) {
        return NULL;
    }

    /* Allocar array para máximo de mensajes */
    int alloc_size = (max_messages < buf->count) ? max_messages : buf->count;
    char **messages = malloc(sizeof(char*) * alloc_size);
    if (!messages) return NULL;

    /* Determinar punto final: current_view si estamos navegando, tail si no */
    MessageNode *end_node = buf->current_view ? buf->current_view : buf->tail;
    if (!end_node) {
        free(messages);
        return NULL;
    }

    /* Retroceder desde end_node para encontrar el punto inicial */
    MessageNode *start_node = end_node;
    int available = 1;

    MessageNode *temp = end_node->prev;
    while (temp && available < alloc_size) {
        start_node = temp;
        available++;
        temp = temp->prev;
    }

    /* Llenar el array en orden desde start_node hasta end_node (inclusive) */
    temp = start_node;
    while (temp) {
        if (*count >= alloc_size) break;

        messages[*count] = temp->message;
        (*count)++;

        if (temp == end_node) break;
        temp = temp->next;
    }

    return messages;
}
