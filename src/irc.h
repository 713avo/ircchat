#ifndef IRC_H
#define IRC_H

#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

/* Estado de la conexión IRC */
typedef struct {
    int sockfd;
    bool connected;
    char server[MAX_SERVER_LEN];
    int port;
    char nick[MAX_NICK_LEN];
    time_t last_ping;
    time_t last_pong;
    char recv_buffer[8192];  /* Buffer para mensajes parciales */
    int recv_buffer_len;     /* Cantidad de datos en el buffer */
} IRCConnection;

/* Funciones de conexión IRC */
IRCConnection* irc_create(void);
void irc_destroy(IRCConnection *irc);
int irc_connect(IRCConnection *irc, const char *server, int port);
void irc_disconnect(IRCConnection *irc);
int irc_send(IRCConnection *irc, const char *message);
int irc_send_raw(IRCConnection *irc, const char *format, ...);
int irc_recv(IRCConnection *irc, char *buffer, size_t size);

/* Comandos IRC básicos */
void irc_set_nick(IRCConnection *irc, const char *nick);
void irc_join(IRCConnection *irc, const char *channel);
void irc_part(IRCConnection *irc, const char *channel);
void irc_privmsg(IRCConnection *irc, const char *target, const char *message);
void irc_pong(IRCConnection *irc, const char *server);

/* Procesamiento de mensajes IRC */
void irc_process_message(IRCConnection *irc, const char *message, void *user_data);

#endif /* IRC_H */
