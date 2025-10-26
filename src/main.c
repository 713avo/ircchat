#include "common.h"
#include "terminal.h"
#include "windows.h"
#include "buffer.h"
#include "irc.h"
#include "commands.h"
#include "input.h"
#include "config.h"
#include <signal.h>
#include <sys/select.h>
#include <unistd.h>

/* Variables globales para manejo de señales */
static volatile bool g_running = true;

/* Manejador de señal SIGINT (Ctrl-C) */
void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

/* Procesar mensajes IRC recibidos */
void process_irc_messages(IRCConnection *irc, WindowManager *wm, Config *config,
                         bool *notify_status, bool *notify_alert, bool *mention_alert, bool silent_mode, int debug_window_id) {
    if (!irc || !irc->connected) return;

    char buffer[4096];
    int n = irc_recv(irc, buffer, sizeof(buffer));

    if (n > 0) {
        /* Procesar mensajes línea por línea */
        char *line = strtok(buffer, "\r\n");
        while (line) {
            /* Determinar si mostrar en sistema según silent_mode o tipo de mensaje */
            bool show_in_system = true;

            if (silent_mode) {
                /* Ocultar JOIN, QUIT, PART, PRIVMSG en modo silencioso */
                if (strstr(line, "JOIN") || strstr(line, "QUIT") ||
                    strstr(line, "PART") || strstr(line, "PRIVMSG")) {
                    show_in_system = false;
                }
            }

            /* Filtrar mensajes MOTD y otros mensajes informativos no críticos */
            if (strstr(line, " 372 ") ||  /* MOTD line */
                strstr(line, " 375 ") ||  /* MOTD start */
                strstr(line, " 376 ") ||  /* MOTD end */
                strstr(line, " 252 ") ||  /* Operator count */
                strstr(line, " 253 ") ||  /* Unknown connections */
                strstr(line, " 254 ") ||  /* Channels count */
                strstr(line, " 255 ") ||  /* Clients and servers */
                strstr(line, " 265 ") ||  /* Local users */
                strstr(line, " 266 ") ||  /* Global users */
                strstr(line, " 321 ") ||  /* RPL_LISTSTART */
                strstr(line, " 322 ") ||  /* RPL_LIST */
                strstr(line, " 323 ")) {  /* RPL_LISTEND */
                show_in_system = false;
            }

            /* Mostrar mensaje raw en ventana de sistema si corresponde */
            if (show_in_system) {
                char display[MAX_MSG_LEN];
                snprintf(display, sizeof(display), ANSI_GRAY "< %s" ANSI_RESET, line);
                wm_add_message(wm, 0, display);
            }

            /* Procesar el mensaje (incluyendo PINGs) */
            irc_process_message(irc, line, wm);

            /* Detectar mensaje 001 (RPL_WELCOME) para autojoin */
            if (strstr(line, " 001 ") != NULL && line[0] == ':') {
                /* Realizar autojoin si hay canales configurados */
                for (int i = 0; i < config->autojoin_count; i++) {
                    const char *channel = config->autojoin_channels[i];

                    /* Crear ventana para el canal */
                    int win_id = wm_create_window(wm, WIN_CHANNEL, channel);
                    if (win_id != -1) {
                        /* Aplicar configuración y abrir log si está habilitado */
                        Window *win = wm_get_window(wm, win_id);
                        if (win) {
                            if (win->buffer) {
                                win->buffer->enabled = config->buffer_enabled;
                            }
                            if (config->log_enabled) {
                                window_open_log(win);
                            }
                        }

                        /* Enviar comando JOIN al servidor */
                        char join_cmd[MAX_MSG_LEN];
                        snprintf(join_cmd, sizeof(join_cmd), "JOIN %s", channel);
                        irc_send(irc, join_cmd);

                        /* NAMES se solicitará cuando el servidor confirme el JOIN */

                        char msg[MAX_MSG_LEN];
                        snprintf(msg, sizeof(msg), ANSI_CYAN "* Auto-join: %s (ventana %d)" ANSI_RESET,
                                 channel, win_id);
                        wm_add_message(wm, 0, msg);
                    }
                }
            }

            /* Parseo básico de mensajes IRC */
            /* Formato: :nick!user@host PRIVMSG #channel :mensaje */
            if (strstr(line, "PRIVMSG")) {
                char *privmsg = strstr(line, "PRIVMSG");
                if (privmsg) {
                    char sender[MAX_NICK_LEN] = "";
                    char target[MAX_CHANNEL_LEN] = "";
                    char *msg_text = NULL;

                    /* Extraer nick del sender */
                    if (line[0] == ':') {
                        char *excl = strchr(line, '!');
                        if (excl) {
                            int nick_len = excl - (line + 1);
                            if (nick_len < MAX_NICK_LEN) {
                                strncpy(sender, line + 1, nick_len);
                                sender[nick_len] = '\0';
                            }
                        }
                    }

                    /* Extraer target y mensaje */
                    if (sscanf(privmsg, "PRIVMSG %s :", target) == 1) {
                        msg_text = strstr(privmsg, " :");
                        if (msg_text) {
                            msg_text += 2; /* Saltar " :" */

                            /* Crear copia limpia del mensaje y eliminar \r\n */
                            char clean_msg[MAX_MSG_LEN];
                            strncpy(clean_msg, msg_text, sizeof(clean_msg) - 1);
                            clean_msg[sizeof(clean_msg) - 1] = '\0';

                            /* Eliminar \r y \n del final */
                            char *newline = strchr(clean_msg, '\r');
                            if (newline) *newline = '\0';
                            newline = strchr(clean_msg, '\n');
                            if (newline) *newline = '\0';

                            /* Actualizar msg_text para apuntar a la versión limpia */
                            msg_text = clean_msg;

                            /* Buscar ventana apropiada */
                            Window *dest_win = NULL;

                            /* Si el target es un canal */
                            if (target[0] == '#') {
                                for (int i = 0; i < MAX_WINDOWS; i++) {
                                    Window *w = wm_get_window(wm, i);
                                    if (w && w->type == WIN_CHANNEL && strcmp(w->title, target) == 0) {
                                        dest_win = w;
                                        break;
                                    }
                                }

                                /* Detectar mención del nick del usuario */
                                if (mention_alert && irc->nick[0] != '\0' && strcasestr(msg_text, irc->nick)) {
                                    *mention_alert = true;
                                }
                            } else {
                                /* Mensaje privado - buscar o crear ventana */
                                for (int i = 0; i < MAX_WINDOWS; i++) {
                                    Window *w = wm_get_window(wm, i);
                                    if (w && w->type == WIN_PRIVATE && strcmp(w->title, sender) == 0) {
                                        dest_win = w;
                                        break;
                                    }
                                }

                                /* Crear ventana si no existe */
                                if (!dest_win) {
                                    int win_id = wm_create_window(wm, WIN_PRIVATE, sender);
                                    dest_win = wm_get_window(wm, win_id);

                                    /* Aplicar configuración y abrir log si está habilitado */
                                    if (config && dest_win) {
                                        if (dest_win->buffer) {
                                            dest_win->buffer->enabled = config->buffer_enabled;
                                        }
                                        if (config->log_enabled) {
                                            window_open_log(dest_win);
                                        }
                                    }
                                }
                            }

                            /* Mostrar mensaje */
                            if (dest_win) {
                                char msg[MAX_MSG_LEN];
                                snprintf(msg, sizeof(msg), ANSI_GREEN "<%s>" ANSI_RESET " %s", sender, msg_text);
                                wm_add_message_with_timestamp(wm, dest_win->id, msg,
                                                               config->timestamp_enabled,
                                                               config->timestamp_format);

                                /* Marcar actividad si no es la ventana activa */
                                wm_mark_window_activity(wm, dest_win->id);
                            }
                        }
                    }
                }
            }
            /* JOIN message: :nick!user@host JOIN :#channel */
            else if (strstr(line, " JOIN ") || strstr(line, " JOIN :")) {
                char sender[MAX_NICK_LEN] = "";
                char channel[MAX_CHANNEL_LEN] = "";

                /* Extraer nick */
                if (line[0] == ':') {
                    char *excl = strchr(line, '!');
                    if (excl) {
                        int nick_len = excl - (line + 1);
                        if (nick_len < MAX_NICK_LEN) {
                            strncpy(sender, line + 1, nick_len);
                            sender[nick_len] = '\0';
                        }
                    }
                }

                /* Extraer canal */
                char *join_cmd = strstr(line, " JOIN");
                if (join_cmd) {
                    join_cmd++; /* Saltar el espacio inicial */
                    if (sscanf(join_cmd, "JOIN :%s", channel) == 1 ||
                        sscanf(join_cmd, "JOIN %s", channel) == 1) {

                        debug_log(wm, debug_window_id, "JOIN recibido: sender='%s', canal='%s', mi_nick='%s'",
                                 sender, channel, irc->nick);

                        /* Buscar ventana del canal */
                        Window *found_win = NULL;
                        int found_win_id = -1;

                        for (int i = 0; i < MAX_WINDOWS; i++) {
                            Window *w = wm_get_window(wm, i);
                            if (w && w->type == WIN_CHANNEL && strcmp(w->title, channel) == 0) {
                                found_win = w;
                                found_win_id = i;
                                break;
                            }
                        }

                        debug_log(wm, debug_window_id, "JOIN: found_win=%p, es_mio=%d",
                                 (void*)found_win, strcasecmp(sender, irc->nick) == 0);

                        /* Si el JOIN es nuestro (estamos confirmados en el canal) */
                        if (strcasecmp(sender, irc->nick) == 0) {
                            /* Si no existe ventana, crearla */
                            if (!found_win) {
                                found_win_id = wm_create_window(wm, WIN_CHANNEL, channel);
                                found_win = wm_get_window(wm, found_win_id);

                                /* Aplicar configuración y abrir log si está habilitado */
                                if (config && found_win) {
                                    if (found_win->buffer) {
                                        found_win->buffer->enabled = config->buffer_enabled;
                                    }
                                    if (config->log_enabled) {
                                        window_open_log(found_win);
                                    }
                                }
                            }

                            /* Solicitar lista de usuarios AHORA que estamos confirmados en el canal */
                            char names_cmd[MAX_MSG_LEN];
                            snprintf(names_cmd, sizeof(names_cmd), "NAMES %s", channel);
                            irc_send(irc, names_cmd);
                            debug_log(wm, debug_window_id, "JOIN confirmado: solicitando NAMES para %s", channel);
                        }

                        /* Añadir usuario a la ventana */
                        if (found_win) {
                            window_add_user(found_win, sender);

                            char msg[MAX_MSG_LEN];
                            snprintf(msg, sizeof(msg), ANSI_GREEN "* %s se ha unido a %s" ANSI_RESET,
                                     sender, channel);
                            wm_add_message(wm, found_win_id, msg);
                        }
                    }
                }
            }
            /* QUIT message: :nick!user@host QUIT :mensaje */
            else if (strstr(line, " QUIT ")) {
                char sender[MAX_NICK_LEN] = "";
                char quit_msg[MAX_MSG_LEN] = "";

                /* Extraer nick */
                if (line[0] == ':') {
                    char *excl = strchr(line, '!');
                    if (excl) {
                        int nick_len = excl - (line + 1);
                        if (nick_len < MAX_NICK_LEN) {
                            strncpy(sender, line + 1, nick_len);
                            sender[nick_len] = '\0';
                        }
                    }
                }

                /* Extraer mensaje de salida (opcional) */
                char *quit_ptr = strstr(line, " :");
                if (quit_ptr) {
                    quit_ptr += 2; /* Saltar " :" */
                    strncpy(quit_msg, quit_ptr, sizeof(quit_msg) - 1);
                    quit_msg[sizeof(quit_msg) - 1] = '\0';
                    /* Limpiar \r\n */
                    char *newline = strchr(quit_msg, '\r');
                    if (newline) *newline = '\0';
                    newline = strchr(quit_msg, '\n');
                    if (newline) *newline = '\0';
                }

                /* Remover usuario de TODOS los canales donde esté presente */
                if (sender[0] != '\0') {
                    for (int i = 0; i < MAX_WINDOWS; i++) {
                        Window *w = wm_get_window(wm, i);
                        if (w && w->type == WIN_CHANNEL) {
                            /* Verificar si el usuario está en este canal */
                            UserNode *user = w->users;
                            bool found = false;
                            while (user) {
                                if (strcmp(user->nick, sender) == 0) {
                                    found = true;
                                    break;
                                }
                                user = user->next;
                            }

                            /* Si está en el canal, removerlo y mostrar mensaje */
                            if (found) {
                                window_remove_user(w, sender);

                                char msg[MAX_MSG_LEN];
                                if (quit_msg[0] != '\0') {
                                    snprintf(msg, sizeof(msg), ANSI_RED "* %s ha salido del servidor (%s)" ANSI_RESET,
                                             sender, quit_msg);
                                } else {
                                    snprintf(msg, sizeof(msg), ANSI_RED "* %s ha salido del servidor" ANSI_RESET,
                                             sender);
                                }
                                wm_add_message(wm, i, msg);
                            }
                        }
                    }
                }
            }
            /* PART message */
            else if (strstr(line, " PART ")) {
                char sender[MAX_NICK_LEN] = "";
                char channel[MAX_CHANNEL_LEN] = "";

                if (line[0] == ':') {
                    char *excl = strchr(line, '!');
                    if (excl) {
                        int nick_len = excl - (line + 1);
                        if (nick_len < MAX_NICK_LEN) {
                            strncpy(sender, line + 1, nick_len);
                            sender[nick_len] = '\0';
                        }
                    }
                }

                char *part_cmd = strstr(line, " PART");
                if (part_cmd) part_cmd++; /* Saltar el espacio inicial */
                if (part_cmd) {
                    if (sscanf(part_cmd, "PART %s", channel) == 1) {
                        for (int i = 0; i < MAX_WINDOWS; i++) {
                            Window *w = wm_get_window(wm, i);
                            if (w && w->type == WIN_CHANNEL && strcmp(w->title, channel) == 0) {
                                window_remove_user(w, sender);

                                char msg[MAX_MSG_LEN];
                                snprintf(msg, sizeof(msg), ANSI_YELLOW "* %s ha salido de %s" ANSI_RESET,
                                         sender, channel);
                                wm_add_message(wm, i, msg);
                                break;
                            }
                        }
                    }
                }
            }
            /* NAMES reply (353) - lista de usuarios en el canal */
            else if (strstr(line, " 353 ") || strstr(line, " RPL_NAMREPLY ")) {
                /* Formato: :server 353 nick = #channel :user1 @user2 +user3 ... */
                char *names_start = strstr(line, " :");
                if (names_start) {
                    names_start += 2; /* Saltar " :" */

                    /* Crear copia limpia para strtok (no modifica el original) */
                    char names_copy[MAX_MSG_LEN];
                    strncpy(names_copy, names_start, sizeof(names_copy) - 1);
                    names_copy[sizeof(names_copy) - 1] = '\0';

                    /* Limpiar \r\n del final */
                    char *newline = strchr(names_copy, '\r');
                    if (newline) *newline = '\0';
                    newline = strchr(names_copy, '\n');
                    if (newline) *newline = '\0';

                    /* Encontrar el canal en esta línea */
                    char channel[MAX_CHANNEL_LEN] = "";
                    char *chan_ptr = strchr(line, '#');
                    if (chan_ptr) {
                        int chan_len = 0;
                        while (chan_ptr[chan_len] && chan_ptr[chan_len] != ' ' && chan_len < MAX_CHANNEL_LEN - 1) {
                            channel[chan_len] = chan_ptr[chan_len];
                            chan_len++;
                        }
                        channel[chan_len] = '\0';

                        debug_log(wm, debug_window_id, "NAMES: canal=%s, nicks='%s'", channel, names_copy);

                        /* Buscar la ventana del canal */
                        for (int i = 0; i < MAX_WINDOWS; i++) {
                            Window *w = wm_get_window(wm, i);
                            if (w && w->type == WIN_CHANNEL && strcmp(w->title, channel) == 0) {
                                /* Parsear lista de nicks */
                                char *nick_token = strtok(names_copy, " ");
                                int nick_count = 0;
                                int nick_skipped = 0;

                                while (nick_token && nick_count < MAX_USERS_PER_CHANNEL) {
                                    char mode = ' ';
                                    char *nick_start = nick_token;

                                    /* Validar que el token no esté vacío y tenga longitud razonable */
                                    size_t token_len = strlen(nick_token);
                                    if (token_len == 0 || token_len >= MAX_NICK_LEN) {
                                        nick_skipped++;
                                        nick_token = strtok(NULL, " ");
                                        continue;
                                    }

                                    /* Detectar prefijo de modo */
                                    if (*nick_start == '@') {
                                        mode = '@';
                                        nick_start++;
                                    } else if (*nick_start == '+') {
                                        mode = '+';
                                        nick_start++;
                                    } else if (*nick_start == '%') {
                                        mode = '%';  /* Half-op */
                                        nick_start++;
                                    } else if (*nick_start == '~') {
                                        mode = '~';  /* Owner */
                                        nick_start++;
                                    } else if (*nick_start == '&') {
                                        mode = '&';  /* Admin */
                                        nick_start++;
                                    }

                                    /* Validar que el nick después del modo no esté vacío */
                                    if (strlen(nick_start) > 0 && strlen(nick_start) < MAX_NICK_LEN) {
                                        window_add_user_with_mode(w, nick_start, mode);
                                        nick_count++;
                                    } else {
                                        nick_skipped++;
                                    }

                                    nick_token = strtok(NULL, " ");
                                }

                                debug_log(wm, debug_window_id, "NAMES: añadidos %d usuarios a %s (saltados: %d, total ventana: %d)",
                                         nick_count, channel, nick_skipped, w->user_count);

                                if (nick_count >= MAX_USERS_PER_CHANNEL) {
                                    debug_log(wm, debug_window_id, "NAMES: ADVERTENCIA - límite de usuarios alcanzado en %s", channel);
                                }
                                break;
                            }
                        }
                    }
                }
            }
            /* ISON reply (303) - para sistema notify */
            else if (strstr(line, " 303 ")) {
                /* Formato: :server 303 nick :nick1 nick2 nick3 */
                char *ison_start = strstr(line, " :");
                if (ison_start && config && notify_alert) {
                    ison_start += 2; /* Saltar " :" */

                    /* Marcar todos como offline inicialmente */
                    bool current_status[MAX_NOTIFY_NICKS] = {false};

                    /* Verificar qué nicks están online */
                    for (int i = 0; i < config->notify_count; i++) {
                        if (strcasestr(ison_start, config->notify_nicks[i])) {
                            current_status[i] = true;

                            /* Si cambió de offline a online, activar alerta */
                            if (!notify_status[i]) {
                                *notify_alert = true;
                            }
                        }
                    }

                    /* Actualizar estados */
                    for (int i = 0; i < config->notify_count; i++) {
                        notify_status[i] = current_status[i];
                    }
                }
            }
            /* LIST reply (322) - item de lista de canales */
            else if (strstr(line, " 322 ")) {
                /* Formato: :server 322 nick #canal users :topic */
                /* Buscar ventana LIST */
                Window *list_win = NULL;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    Window *w = wm_get_window(wm, i);
                    if (w && w->type == WIN_LIST && w->list_receiving) {
                        list_win = w;
                        break;
                    }
                }

                if (list_win) {
                    char channel[MAX_CHANNEL_LEN] = "";
                    int users = 0;
                    char topic[512] = "";

                    /* Parsear: :server 322 nick #canal users :topic */
                    char *ptr = line;
                    /* Saltar :server 322 nick */
                    for (int i = 0; i < 3 && ptr; i++) {
                        ptr = strchr(ptr, ' ');
                        if (ptr) ptr++;
                    }

                    if (ptr) {
                        /* Leer nombre del canal */
                        if (sscanf(ptr, "%s %d", channel, &users) == 2) {
                            /* Buscar topic después de ":" */
                            char *topic_ptr = strstr(ptr, " :");
                            if (topic_ptr) {
                                topic_ptr += 2;  /* Saltar " :" */
                                strncpy(topic, topic_ptr, sizeof(topic) - 1);
                                topic[sizeof(topic) - 1] = '\0';
                                /* Limpiar \r\n del topic */
                                char *newline = strchr(topic, '\r');
                                if (newline) *newline = '\0';
                                newline = strchr(topic, '\n');
                                if (newline) *newline = '\0';
                            }

                            /* Añadir canal a la lista */
                            window_add_channel_to_list(list_win, channel, users, topic);
                        }
                    }
                }
            }
            /* End of LIST (323) */
            else if (strstr(line, " 323 ")) {
                /* Buscar ventana LIST */
                Window *list_win = NULL;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    Window *w = wm_get_window(wm, i);
                    if (w && w->type == WIN_LIST && w->list_receiving) {
                        list_win = w;
                        break;
                    }
                }

                if (list_win) {
                    /* Finalizar recepción de lista */
                    window_finalize_channel_list(list_win);
                }
            }
            /* TOPIC message (332) - topic del canal */
            else if (strstr(line, " 332 ")) {
                /* Formato: :server 332 nick #canal :topic */
                char channel[MAX_CHANNEL_LEN] = "";
                char topic[512] = "";

                /* Parsear canal y topic */
                char *ptr = line;
                /* Saltar :server 332 nick */
                for (int i = 0; i < 3 && ptr; i++) {
                    ptr = strchr(ptr, ' ');
                    if (ptr) ptr++;
                }

                if (ptr && sscanf(ptr, "%s", channel) == 1) {
                    /* Buscar topic después de ":" */
                    char *topic_ptr = strstr(ptr, " :");
                    if (topic_ptr) {
                        topic_ptr += 2;  /* Saltar " :" */
                        strncpy(topic, topic_ptr, sizeof(topic) - 1);
                        topic[sizeof(topic) - 1] = '\0';

                        /* Limpiar \r\n del topic */
                        char *newline = strchr(topic, '\r');
                        if (newline) *newline = '\0';
                        newline = strchr(topic, '\n');
                        if (newline) *newline = '\0';
                    }

                    /* Buscar ventana del canal */
                    for (int i = 0; i < MAX_WINDOWS; i++) {
                        Window *w = wm_get_window(wm, i);
                        if (w && w->type == WIN_CHANNEL && strcmp(w->title, channel) == 0) {
                            strncpy(w->topic, topic, sizeof(w->topic) - 1);
                            w->topic[sizeof(w->topic) - 1] = '\0';
                            break;
                        }
                    }
                }
            }

            line = strtok(NULL, "\r\n");
        }
    } else if (n < 0) {
        /* Error en la conexión */
        wm_add_message(wm, 0, ANSI_RED "Error: Conexión IRC perdida" ANSI_RESET);
        irc->connected = false;
    }
}

/* Bucle principal */
int main(void) {
    /* Configurar manejador de señales */
    signal(SIGINT, signal_handler);

    /* Inicializar subsistemas */
    TerminalState term;
    term_init(&term);
    term_enter_raw_mode(&term);

    WindowManager *wm = wm_create();
    IRCConnection *irc = irc_create();
    InputState input;
    input_init(&input);

    /* Cargar configuración */
    Config *config = config_create();
    char config_path[512];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(config_path, sizeof(config_path), "%s/.ircchat.rc", home);
        config_load(config, config_path);
    }

    bool running = true;
    bool buffer_enabled = config->buffer_enabled;
    bool silent_mode = config->silent_mode;
    bool notify_alert = false;
    bool mention_alert = false;
    int debug_window_id = -1;  /* -1 = debug desactivado */

    /* Aplicar configuración del buffer a todas las ventanas existentes */
    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *win = wm->windows[i];
        if (win && win->buffer) {
            win->buffer->enabled = config->buffer_enabled;
        }
    }

    /* Sistema de notify - tracking de nicks */
    bool notify_status[MAX_NOTIFY_NICKS] = {false};  /* Estado anterior: false=offline, true=online */
    time_t last_notify_check = 0;

    /* Sistema de autocompletado */
    char autocomplete_prefix[MAX_NICK_LEN] = "";
    int autocomplete_index = 0;
    bool autocomplete_active = false;

    /* Aplicar nick por defecto si está configurado */
    if (config->has_nick) {
        irc_set_nick(irc, config->nick);
    }

    /* Contexto de comandos */
    CommandContext cmd_ctx = {
        .wm = wm,
        .irc = irc,
        .config = config,
        .running = &running,
        .buffer_enabled = &buffer_enabled,
        .silent_mode = &silent_mode,
        .notify_alert = &notify_alert,
        .mention_alert = &mention_alert,
        .debug_window_id = &debug_window_id
    };

    /* Mensaje de bienvenida */
    wm_add_message(wm, 0, ANSI_BOLD ANSI_CYAN "=== Cliente IRC ===" ANSI_RESET);
    if (config->has_server) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), "Servidor por defecto: " ANSI_CYAN "%s:%d" ANSI_RESET,
                 config->server, config->port);
        wm_add_message(wm, 0, msg);
    }
    if (config->has_nick) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), "Nick por defecto: " ANSI_CYAN "%s" ANSI_RESET, config->nick);
        wm_add_message(wm, 0, msg);
    }
    wm_add_message(wm, 0, "Escribe " ANSI_YELLOW "/help" ANSI_RESET " para ver los comandos disponibles");
    wm_add_message(wm, 0, "Conecta con " ANSI_YELLOW "/connect" ANSI_RESET " o " ANSI_YELLOW "/connect <servidor> [puerto]" ANSI_RESET);
    wm_add_message(wm, 0, "");

    /* Dibujar interfaz inicial */
    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);

    /* Bucle principal */
    while (running && g_running) {
        /* Actualizar tamaño del terminal */
        term_get_size(&term);

        /* Preparar select para I/O no bloqueante */
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_fd = STDIN_FILENO;

        if (irc->connected) {
            FD_SET(irc->sockfd, &readfds);
            if (irc->sockfd > max_fd) {
                max_fd = irc->sockfd;
            }
        }

        tv.tv_sec = 0;
        tv.tv_usec = 100000; /* 100ms */

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        /* Procesar mensajes IRC */
        if (ret > 0 && irc->connected && FD_ISSET(irc->sockfd, &readfds)) {
            process_irc_messages(irc, wm, config, notify_status, &notify_alert, &mention_alert, silent_mode, debug_window_id);
            term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
        }

        /* Sistema de notify: revisar cada 60 segundos */
        if (irc->connected && config->notify_count > 0) {
            time_t now = time(NULL);
            if (now - last_notify_check >= 60) {
                last_notify_check = now;

                /* Construir comando ISON con todos los nicks */
                char ison_cmd[MAX_MSG_LEN] = "ISON";
                for (int i = 0; i < config->notify_count; i++) {
                    strcat(ison_cmd, " ");
                    strcat(ison_cmd, config->notify_nicks[i]);
                }
                irc_send(irc, ison_cmd);
            }
        }

        /* Procesar entrada del usuario */
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            int key = input_read_key();

            if (key == KEY_ENTER || key == '\r') {
                /* Procesar entrada */
                if (input.length > 0) {
                    input_history_add(&input, input.line);

                    /* Es un comando? */
                    if (input.line[0] == '/') {
                        process_command(&cmd_ctx, input.line);
                    } else {
                        /* Enviar a la ventana activa */
                        Window *win = wm_get_active_window(wm);
                        if (win && irc->connected) {
                            if (win->type == WIN_CHANNEL) {
                                irc_privmsg(irc, win->title, input.line);

                                char msg[MAX_MSG_LEN];
                                snprintf(msg, sizeof(msg), ANSI_CYAN "<%s>" ANSI_RESET " %s",
                                         irc->nick, input.line);
                                wm_add_message_with_timestamp(wm, win->id, msg,
                                                               config->timestamp_enabled,
                                                               config->timestamp_format);
                            } else if (win->type == WIN_PRIVATE) {
                                irc_privmsg(irc, win->title, input.line);

                                char msg[MAX_MSG_LEN];
                                snprintf(msg, sizeof(msg), ANSI_CYAN "<%s>" ANSI_RESET " %s",
                                         irc->nick, input.line);
                                wm_add_message_with_timestamp(wm, win->id, msg,
                                                               config->timestamp_enabled,
                                                               config->timestamp_format);
                            } else {
                                wm_add_message(wm, 0, ANSI_RED "No puedes enviar mensajes desde la ventana de sistema" ANSI_RESET);
                            }
                        } else if (!irc->connected) {
                            wm_add_message(wm, 0, ANSI_RED "No estás conectado a un servidor" ANSI_RESET);
                        }
                    }

                    input_clear_line(&input);
                }

                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_BACKSPACE) {
                input_backspace(&input);
                autocomplete_active = false;
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_TAB) {
                /* Autocompletado de nicks */
                debug_log(wm, debug_window_id, "TAB: inicio autocompletado");
                Window *win = wm_get_active_window(wm);
                if (win && win->type == WIN_CHANNEL && win->users) {
                    debug_log(wm, debug_window_id, "TAB: ventana canal detectada, cursor_pos=%d, line_len=%d",
                              input.cursor_pos, input.length);

                    /* Buscar inicio de la palabra actual */
                    int word_start = input.cursor_pos;
                    while (word_start > 0 && input.line[word_start - 1] != ' ') {
                        word_start--;
                    }

                    /* Extraer palabra parcial con validación */
                    int word_len = input.cursor_pos - word_start;
                    char partial[MAX_NICK_LEN] = "";

                    if (word_len > 0 && word_len < MAX_NICK_LEN && word_start >= 0 &&
                        word_start < MAX_INPUT_LEN && input.cursor_pos <= input.length) {

                        /* Asegurar que no copiamos más allá del buffer */
                        int safe_len = MIN(word_len, MAX_NICK_LEN - 1);
                        safe_len = MIN(safe_len, input.length - word_start);

                        if (safe_len > 0) {
                            strncpy(partial, input.line + word_start, safe_len);
                            partial[safe_len] = '\0';
                        }
                    }

                    debug_log(wm, debug_window_id, "TAB: palabra='%s', word_start=%d, word_len=%d",
                              partial, word_start, word_len);

                    /* Si es nueva búsqueda o prefijo cambió */
                    if (!autocomplete_active || strcmp(partial, autocomplete_prefix) != 0) {
                        strncpy(autocomplete_prefix, partial, MAX_NICK_LEN - 1);
                        autocomplete_prefix[MAX_NICK_LEN - 1] = '\0';
                        autocomplete_index = 0;
                        autocomplete_active = true;
                        debug_log(wm, debug_window_id, "TAB: nueva búsqueda, prefix='%s'", autocomplete_prefix);
                    } else {
                        /* Rotar al siguiente */
                        autocomplete_index++;
                        debug_log(wm, debug_window_id, "TAB: rotar a index=%d", autocomplete_index);
                    }

                    /* Buscar nick que coincida */
                    UserNode *user = win->users;
                    int match_count = 0;
                    char match[MAX_NICK_LEN] = "";

                    while (user) {
                        if (strlen(autocomplete_prefix) == 0 ||
                            strncasecmp(user->nick, autocomplete_prefix, strlen(autocomplete_prefix)) == 0) {
                            if (match_count == autocomplete_index) {
                                strncpy(match, user->nick, MAX_NICK_LEN - 1);
                                match[MAX_NICK_LEN - 1] = '\0';
                                debug_log(wm, debug_window_id, "TAB: match encontrado='%s'", match);
                                break;
                            }
                            match_count++;
                        }
                        user = user->next;
                    }

                    /* Si encontramos match, completar */
                    if (match[0] != '\0') {
                        /* Borrar palabra parcial con validación */
                        int backspace_count = 0;
                        while (input.cursor_pos > word_start && backspace_count < MAX_NICK_LEN) {
                            input_backspace(&input);
                            backspace_count++;
                        }

                        debug_log(wm, debug_window_id, "TAB: backspaces=%d, insertando '%s'",
                                  backspace_count, match);

                        /* Insertar nick completo con verificación de longitud */
                        size_t match_len = strlen(match);
                        for (size_t i = 0; i < match_len && input.length < MAX_INPUT_LEN - 3; i++) {
                            input_add_char(&input, match[i]);
                        }

                        /* Si está al inicio de línea, añadir : y espacio */
                        if (word_start == 0 && input.length < MAX_INPUT_LEN - 2) {
                            input_add_char(&input, ':');
                            input_add_char(&input, ' ');
                        }
                    } else {
                        /* No hay más matches, volver al inicio */
                        autocomplete_index = 0;
                        debug_log(wm, debug_window_id, "TAB: no hay más matches, reset index");
                    }

                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                } else {
                    debug_log(wm, debug_window_id, "TAB: no aplicable (win=%p, type=%d)",
                              (void*)win, win ? win->type : -1);
                }
            }
            else if (key == KEY_ARROW_UP) {
                input_history_prev(&input);
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_ARROW_DOWN) {
                input_history_next(&input);
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_ARROW_LEFT) {
                input_move_left(&input);
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_ARROW_RIGHT) {
                input_move_right(&input);
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_CTRL_ARROW_UP) {
                Window *win = wm_get_active_window(wm);
                if (win && win->buffer && buffer_enabled) {
                    buffer_scroll_up(win->buffer);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            else if (key == KEY_CTRL_ARROW_DOWN) {
                Window *win = wm_get_active_window(wm);
                if (win && win->buffer && buffer_enabled) {
                    buffer_scroll_down(win->buffer);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            else if (key == KEY_CTRL_B) {
                Window *win = wm_get_active_window(wm);
                if (win && win->buffer && buffer_enabled) {
                    buffer_scroll_top(win->buffer);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            else if (key == KEY_CTRL_E) {
                Window *win = wm_get_active_window(wm);
                if (win && win->buffer && buffer_enabled) {
                    buffer_scroll_bottom(win->buffer);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            else if (key == KEY_CTRL_SHIFT_ARROW_UP) {
                Window *win = wm_get_active_window(wm);
                if (win && win->type == WIN_CHANNEL) {
                    window_scroll_users_up(win);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            else if (key == KEY_CTRL_SHIFT_ARROW_DOWN) {
                Window *win = wm_get_active_window(wm);
                if (win && win->type == WIN_CHANNEL) {
                    window_scroll_users_down(win);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            /* Alt + número para cambiar de ventana */
            else if (key >= KEY_ALT_0 && key <= KEY_ALT_9) {
                int win_num = key - KEY_ALT_0;
                Window *win = wm_get_window(wm, win_num);
                if (win) {
                    wm_switch_to(wm, win_num);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            /* Alt + → para ventana siguiente (cíclico) */
            else if (key == KEY_ALT_ARROW_RIGHT) {
                int next_win = (wm->active_window + 1) % MAX_WINDOWS;
                /* Buscar la siguiente ventana que existe */
                int count = 0;
                while (count < MAX_WINDOWS && !wm_get_window(wm, next_win)) {
                    next_win = (next_win + 1) % MAX_WINDOWS;
                    count++;
                }
                if (wm_get_window(wm, next_win)) {
                    wm_switch_to(wm, next_win);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            /* Alt + ← para ventana anterior (cíclico) */
            else if (key == KEY_ALT_ARROW_LEFT) {
                int prev_win = (wm->active_window - 1 + MAX_WINDOWS) % MAX_WINDOWS;
                /* Buscar la ventana anterior que existe */
                int count = 0;
                while (count < MAX_WINDOWS && !wm_get_window(wm, prev_win)) {
                    prev_win = (prev_win - 1 + MAX_WINDOWS) % MAX_WINDOWS;
                    count++;
                }
                if (wm_get_window(wm, prev_win)) {
                    wm_switch_to(wm, prev_win);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            /* Alt+. para /clear */
            else if (key == KEY_ALT_PERIOD) {
                Window *win = wm_get_active_window(wm);
                if (win && win->buffer) {
                    buffer_clear(win->buffer);
                    wm_add_message(wm, win->id, ANSI_GRAY "Pantalla limpiada" ANSI_RESET);
                    term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
                }
            }
            else if (key == KEY_CTRL_L) {
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if (key == KEY_CTRL_C) {
                running = false;
            }
            else if (key >= 32 && key < 127) {
                /* Carácter imprimible ASCII */
                input_add_char(&input, (char)key);
                autocomplete_active = false;
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
            else if ((unsigned char)key >= 128) {
                /* Carácter UTF-8 multibyte */
                char utf8_buf[5] = {0};
                utf8_buf[0] = (char)key;

                /* Determinar cuántos bytes más leer */
                int len = 1;
                if ((key & 0xE0) == 0xC0) len = 2;      /* 110xxxxx - 2 bytes */
                else if ((key & 0xF0) == 0xE0) len = 3; /* 1110xxxx - 3 bytes */
                else if ((key & 0xF8) == 0xF0) len = 4; /* 11110xxx - 4 bytes */

                /* Leer los bytes restantes */
                for (int i = 1; i < len; i++) {
                    unsigned char next_byte;
                    if (read(STDIN_FILENO, &next_byte, 1) == 1) {
                        utf8_buf[i] = next_byte;
                    } else {
                        break;
                    }
                }

                input_add_utf8(&input, utf8_buf, len);
                autocomplete_active = false;
                term_draw_interface(&term, wm, input.line, input.cursor_pos, notify_alert, mention_alert);
            }
        }
    }

    /* Limpieza */
    if (irc->connected) {
        irc_disconnect(irc);
    }

    irc_destroy(irc);
    wm_destroy(wm);
    config_destroy(config);
    term_cleanup(&term);

    printf("¡Hasta pronto!\n");

    return 0;
}
