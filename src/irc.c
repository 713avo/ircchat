#include "irc.h"
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/* Crear conexión IRC */
IRCConnection* irc_create(void) {
    IRCConnection *irc = malloc(sizeof(IRCConnection));
    if (!irc) return NULL;

    irc->sockfd = -1;
    irc->connected = false;
    irc->server[0] = '\0';
    irc->port = DEFAULT_IRC_PORT;
    irc->nick[0] = '\0';
    irc->last_ping = 0;
    irc->last_pong = 0;
    irc->recv_buffer[0] = '\0';
    irc->recv_buffer_len = 0;

    return irc;
}

/* Destruir conexión IRC */
void irc_destroy(IRCConnection *irc) {
    if (!irc) return;

    if (irc->connected) {
        irc_disconnect(irc);
    }

    free(irc);
}

/* Conectar a servidor IRC */
int irc_connect(IRCConnection *irc, const char *server, int port) {
    if (!irc || !server) return -1;

    struct addrinfo hints, *servinfo, *p;
    int rv;
    char port_str[16];

    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server, port_str, &hints, &servinfo)) != 0) {
        return -1;
    }

    /* Intentar conectar con cada dirección disponible */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((irc->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (connect(irc->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(irc->sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        return -1;
    }

    /* Configurar socket como no bloqueante */
    int flags = fcntl(irc->sockfd, F_GETFL, 0);
    fcntl(irc->sockfd, F_SETFL, flags | O_NONBLOCK);

    strncpy(irc->server, server, MAX_SERVER_LEN - 1);
    irc->server[MAX_SERVER_LEN - 1] = '\0';
    irc->port = port;
    irc->connected = true;
    irc->last_ping = time(NULL);
    irc->last_pong = time(NULL);

    return 0;
}

/* Desconectar del servidor IRC */
void irc_disconnect(IRCConnection *irc) {
    if (!irc || !irc->connected) return;

    /* Enviar QUIT */
    irc_send_raw(irc, "QUIT :Cliente IRC saliendo\r\n");

    close(irc->sockfd);
    irc->sockfd = -1;
    irc->connected = false;
}

/* Enviar mensaje al servidor IRC */
int irc_send(IRCConnection *irc, const char *message) {
    if (!irc || !irc->connected || !message) return -1;

    char buffer[MAX_MSG_LEN + 3];
    snprintf(buffer, sizeof(buffer), "%s\r\n", message);

    int len = strlen(buffer);
    int sent = send(irc->sockfd, buffer, len, 0);

    return sent;
}

/* Enviar mensaje formateado al servidor IRC */
int irc_send_raw(IRCConnection *irc, const char *format, ...) {
    if (!irc || !irc->connected || !format) return -1;

    char buffer[MAX_MSG_LEN];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int len = strlen(buffer);
    int sent = send(irc->sockfd, buffer, len, 0);

    return sent;
}

/* Recibir mensaje del servidor IRC */
int irc_recv(IRCConnection *irc, char *buffer, size_t size) {
    if (!irc || !irc->connected || !buffer) return -1;

    /* Intentar recibir más datos */
    int space_left = sizeof(irc->recv_buffer) - irc->recv_buffer_len - 1;
    if (space_left > 0) {
        int n = recv(irc->sockfd, irc->recv_buffer + irc->recv_buffer_len, space_left, 0);

        if (n > 0) {
            irc->recv_buffer_len += n;
            irc->recv_buffer[irc->recv_buffer_len] = '\0';
        } else if (n == 0) {
            /* Conexión cerrada */
            irc->connected = false;
            return 0;
        } else {
            /* Error o no hay datos disponibles */
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }
        }
    }

    /* Si no hay datos en el buffer, retornar 0 */
    if (irc->recv_buffer_len == 0) {
        return 0;
    }

    /* Buscar líneas completas (terminadas con \r\n) */
    char *line_end = strstr(irc->recv_buffer, "\r\n");
    if (!line_end) {
        /* No hay línea completa aún */
        /* Si el buffer está lleno, es un error - mensaje demasiado largo */
        if (irc->recv_buffer_len >= (int)sizeof(irc->recv_buffer) - 1) {
            /* Descartar buffer y reiniciar */
            irc->recv_buffer_len = 0;
            irc->recv_buffer[0] = '\0';
        }
        return 0;
    }

    /* Copiar todas las líneas completas al buffer de salida */
    int output_len = 0;
    char *search_pos = irc->recv_buffer;

    while (line_end && (size_t)output_len < size - 1) {
        int line_len = line_end - search_pos;

        /* Copiar la línea (sin \r\n) al output */
        if (output_len + line_len + 2 < (int)size) {  /* +2 para \r\n */
            memcpy(buffer + output_len, search_pos, line_len);
            output_len += line_len;
            buffer[output_len++] = '\r';
            buffer[output_len++] = '\n';

            /* Buscar siguiente línea */
            search_pos = line_end + 2;  /* Saltar \r\n */
            line_end = strstr(search_pos, "\r\n");
        } else {
            break;
        }
    }

    buffer[output_len] = '\0';

    /* Mover el contenido restante al inicio del buffer */
    int consumed = search_pos - irc->recv_buffer;
    if (consumed > 0) {
        irc->recv_buffer_len -= consumed;
        if (irc->recv_buffer_len > 0) {
            memmove(irc->recv_buffer, search_pos, irc->recv_buffer_len);
        }
        irc->recv_buffer[irc->recv_buffer_len] = '\0';
    }

    return output_len;
}

/* Establecer nickname */
void irc_set_nick(IRCConnection *irc, const char *nick) {
    if (!irc || !nick) return;

    strncpy(irc->nick, nick, MAX_NICK_LEN - 1);
    irc->nick[MAX_NICK_LEN - 1] = '\0';

    if (irc->connected) {
        irc_send_raw(irc, "NICK %s\r\n", nick);
        irc_send_raw(irc, "USER %s 0 * :%s\r\n", nick, nick);
    }
}

/* Unirse a un canal */
void irc_join(IRCConnection *irc, const char *channel) {
    if (!irc || !irc->connected || !channel) return;

    irc_send_raw(irc, "JOIN %s\r\n", channel);
}

/* Salir de un canal */
void irc_part(IRCConnection *irc, const char *channel) {
    if (!irc || !irc->connected || !channel) return;

    irc_send_raw(irc, "PART %s\r\n", channel);
}

/* Enviar mensaje privado o a canal */
void irc_privmsg(IRCConnection *irc, const char *target, const char *message) {
    if (!irc || !irc->connected || !target || !message) return;

    irc_send_raw(irc, "PRIVMSG %s :%s\r\n", target, message);
}

/* Responder a PING */
void irc_pong(IRCConnection *irc, const char *server) {
    if (!irc || !irc->connected || !server) return;

    irc_send_raw(irc, "PONG %s\r\n", server);
    irc->last_pong = time(NULL);
}

/* Procesar mensajes IRC recibidos */
void irc_process_message(IRCConnection *irc, const char *message, void *user_data) {
    if (!irc || !message) return;

    /* Parsear mensaje IRC básico */
    char msg_copy[MAX_MSG_LEN];
    strncpy(msg_copy, message, MAX_MSG_LEN - 1);
    msg_copy[MAX_MSG_LEN - 1] = '\0';

    /* Detectar y responder a PING */
    if (strncmp(msg_copy, "PING ", 5) == 0) {
        char *server = msg_copy + 5;
        /* Eliminar \r\n */
        char *end = strchr(server, '\r');
        if (end) *end = '\0';
        end = strchr(server, '\n');
        if (end) *end = '\0';

        irc_pong(irc, server);
        irc->last_ping = time(NULL);
    }
}
