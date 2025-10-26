#include "commands.h"
#include <time.h>
#include <stdarg.h>

/* Tabla de comandos */
static Command command_table[] = {
    {"help", cmd_help, "Mostrar ayuda de comandos"},
    {"exit", cmd_exit, "Salir del programa"},
    {"quit", cmd_exit, "Salir del programa (alias de /exit)"},
    {"connect", cmd_connect, "Conectar a servidor IRC: /connect <servidor> [puerto]"},
    {"nick", cmd_nick, "Cambiar nickname: /nick <nickname>"},
    {"join", cmd_join, "Unirse a un canal: /join <#canal>"},
    {"part", cmd_part, "Salir de un canal: /part [#canal]"},
    {"msg", cmd_msg, "Enviar mensaje privado: /msg <nick> <mensaje>"},
    {"wl", cmd_window_list, "Listar todas las ventanas"},
    {"wc", cmd_window_close, "Cerrar ventana: /wc [n] (sin número cierra la actual)"},
    {"clear", cmd_clear, "Limpiar pantalla de la ventana activa"},
    {"buffer", cmd_buffer, "Activar/desactivar buffer: /buffer on|off"},
    {"silent", cmd_silent, "Modo silencioso: /silent on|off (oculta JOIN/QUIT/PART)"},
    {"ok", cmd_ok, "Borrar todas las notificaciones (C, M, *, +)"},
    {"log", cmd_log, "Activar/desactivar logging: /log on|off"},
    {"timestamp", cmd_timestamp, "Activar/desactivar timestamps: /timestamp on|off"},
    {"ttformat", cmd_ttformat, "Formato de timestamp: /ttformat HH:MM:SS o /ttformat HH:MM"},
    {"list", cmd_list, "Listar canales: /list [num <n>] [users <n>|<min>-<max>] [order] [search <patrón>]"},
    {"raw", cmd_raw, "Enviar comando IRC raw: /raw <comando IRC>"},
    {"whois", cmd_whois, "Información de usuario: /whois <nick>"},
    {"wii", cmd_wii, "Información de usuario: /wii <nick> (whois + whowas)"},
    {"debug", cmd_debug, "Modo debug: /debug on|off (abre ventana de depuración)"},
    {NULL, NULL, NULL}
};

/* Comando: help */
void cmd_help(CommandContext *ctx, const char *args) {
    (void)args; /* No usado */

    wm_add_message(ctx->wm, 0, ANSI_BOLD ANSI_CYAN "=== Comandos disponibles ===" ANSI_RESET);

    for (int i = 0; command_table[i].name != NULL; i++) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_YELLOW "/%s" ANSI_RESET " - %s",
                 command_table[i].name, command_table[i].help);
        wm_add_message(ctx->wm, 0, msg);
    }

    wm_add_message(ctx->wm, 0, "");
    wm_add_message(ctx->wm, 0, ANSI_BOLD ANSI_CYAN "=== Comandos IRC avanzados ===" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GRAY "/raw permite enviar comandos IRC directamente al servidor" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GRAY "Ejemplos: /raw WHOIS nick, /raw MODE #canal +m, /raw TOPIC #canal :Nuevo topic" ANSI_RESET);
    wm_add_message(ctx->wm, 0, "");
    wm_add_message(ctx->wm, 0, ANSI_BOLD ANSI_CYAN "=== Notificaciones ===" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GREEN "C" ANSI_RESET " (verde, parpadeante) - Nick vigilado (NOTIFY) se conectó");
    wm_add_message(ctx->wm, 0, ANSI_MAGENTA "M" ANSI_RESET " (magenta, parpadeante) - Te mencionaron en un canal");
    wm_add_message(ctx->wm, 0, ANSI_RED "*" ANSI_RESET " (rojo, parpadeante) - Nueva ventana de mensaje privado");
    wm_add_message(ctx->wm, 0, ANSI_YELLOW "+" ANSI_RESET " (amarillo, parpadeante) - Mensajes sin leer");
    wm_add_message(ctx->wm, 0, ANSI_GRAY "Prioridad: C > M > * > +" ANSI_RESET);
    wm_add_message(ctx->wm, 0, "");
    wm_add_message(ctx->wm, 0, ANSI_BOLD ANSI_CYAN "=== Navegación ===" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GRAY "Cambio de ventanas: /w1, /w2, /w3, etc. o Alt+0-9" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GRAY "Navegación buffer: Ctrl-Arriba, Ctrl-Abajo, Ctrl-B (inicio), Ctrl-E (fin)" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GRAY "Historial comandos: Arriba, Abajo" ANSI_RESET);
    wm_add_message(ctx->wm, 0, ANSI_GRAY "Autocompletar nicks: TAB" ANSI_RESET);
}

/* Comando: exit */
void cmd_exit(CommandContext *ctx, const char *args) {
    (void)args; /* No usado */

    wm_add_message(ctx->wm, 0, ANSI_YELLOW "Saliendo del programa..." ANSI_RESET);
    *(ctx->running) = false;
}

/* Comando: connect */
void cmd_connect(CommandContext *ctx, const char *args) {
    char server[MAX_SERVER_LEN];
    int port = DEFAULT_IRC_PORT;

    /* Si no hay argumentos, usar servidor por defecto de configuración */
    if (!args || args[0] == '\0') {
        if (ctx->config && ctx->config->has_server) {
            strncpy(server, ctx->config->server, MAX_SERVER_LEN - 1);
            server[MAX_SERVER_LEN - 1] = '\0';
            port = ctx->config->port;
        } else {
            wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /connect <servidor> [puerto]" ANSI_RESET);
            wm_add_message(ctx->wm, 0, ANSI_GRAY "O define SERVER en ~/.ircchat.rc" ANSI_RESET);
            return;
        }
    } else {
        /* Parsear argumentos */
        if (sscanf(args, "%s %d", server, &port) < 1) {
            wm_add_message(ctx->wm, 0, ANSI_RED "Error: Servidor inválido" ANSI_RESET);
            return;
        }
    }

    /* Desconectar si ya estamos conectados */
    if (ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_YELLOW "Desconectando del servidor actual..." ANSI_RESET);
        irc_disconnect(ctx->irc);
    }

    /* Conectar */
    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_CYAN "Conectando a %s:%d..." ANSI_RESET, server, port);
    wm_add_message(ctx->wm, 0, msg);

    if (irc_connect(ctx->irc, server, port) == 0) {
        snprintf(msg, sizeof(msg), ANSI_GREEN "Conectado a %s:%d" ANSI_RESET, server, port);
        wm_add_message(ctx->wm, 0, msg);

        /* Si ya tenemos un nick, enviarlo */
        if (ctx->irc->nick[0] != '\0') {
            irc_set_nick(ctx->irc, ctx->irc->nick);
        }
    } else {
        snprintf(msg, sizeof(msg), ANSI_RED "Error: No se pudo conectar a %s:%d" ANSI_RESET, server, port);
        wm_add_message(ctx->wm, 0, msg);
    }
}

/* Comando: nick */
void cmd_nick(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /nick <nickname>" ANSI_RESET);
        return;
    }

    char nick[MAX_NICK_LEN];
    if (sscanf(args, "%s", nick) != 1) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Nickname inválido" ANSI_RESET);
        return;
    }

    irc_set_nick(ctx->irc, nick);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_CYAN "Nick cambiado a: %s" ANSI_RESET, nick);
    wm_add_message(ctx->wm, 0, msg);
}

/* Comando: join */
void cmd_join(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /join <#canal>" ANSI_RESET);
        return;
    }

    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    char channel[MAX_CHANNEL_LEN];
    if (sscanf(args, "%s", channel) != 1) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Canal inválido" ANSI_RESET);
        return;
    }

    /* Crear ventana para el canal */
    int win_id = wm_create_window(ctx->wm, WIN_CHANNEL, channel);
    if (win_id != -1) {
        /* Aplicar configuración y abrir log si está habilitado */
        Window *win = wm_get_window(ctx->wm, win_id);
        if (win) {
            if (win->buffer) {
                win->buffer->enabled = ctx->config->buffer_enabled;
            }
            if (ctx->config->log_enabled) {
                window_open_log(win);
            }
        }

        irc_join(ctx->irc, channel);

        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_GREEN "Uniéndose a %s (ventana %d)" ANSI_RESET, channel, win_id);
        wm_add_message(ctx->wm, 0, msg);

        /* Cambiar automáticamente a la ventana del canal */
        wm_switch_to(ctx->wm, win_id);
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No se pudo crear ventana para el canal" ANSI_RESET);
    }
}

/* Comando: part */
void cmd_part(CommandContext *ctx, const char *args) {
    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    Window *win = wm_get_active_window(ctx->wm);
    if (!win || win->type != WIN_CHANNEL) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: La ventana activa no es un canal" ANSI_RESET);
        return;
    }

    irc_part(ctx->irc, win->title);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_YELLOW "Saliendo de %s" ANSI_RESET, win->title);
    wm_add_message(ctx->wm, 0, msg);

    /* Cerrar la ventana */
    wm_close_window(ctx->wm, win->id);
}

/* Comando: msg */
void cmd_msg(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /msg <nick> <mensaje>" ANSI_RESET);
        return;
    }

    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    char target[MAX_NICK_LEN];
    char message[MAX_MSG_LEN];

    /* Parsear nick y mensaje */
    const char *msg_start = strchr(args, ' ');
    if (!msg_start) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Falta el mensaje" ANSI_RESET);
        return;
    }

    int nick_len = msg_start - args;
    if (nick_len >= MAX_NICK_LEN) nick_len = MAX_NICK_LEN - 1;

    strncpy(target, args, nick_len);
    target[nick_len] = '\0';

    msg_start++; /* Saltar el espacio */
    strncpy(message, msg_start, MAX_MSG_LEN - 1);
    message[MAX_MSG_LEN - 1] = '\0';

    /* Enviar mensaje */
    irc_privmsg(ctx->irc, target, message);

    /* Crear ventana privada si no existe */
    Window *priv_win = NULL;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *w = wm_get_window(ctx->wm, i);
        if (w && w->type == WIN_PRIVATE && strcmp(w->title, target) == 0) {
            priv_win = w;
            break;
        }
    }

    if (!priv_win) {
        int win_id = wm_create_window(ctx->wm, WIN_PRIVATE, target);
        priv_win = wm_get_window(ctx->wm, win_id);

        /* Aplicar configuración y abrir log si está habilitado */
        if (priv_win) {
            if (priv_win->buffer) {
                priv_win->buffer->enabled = ctx->config->buffer_enabled;
            }
            if (ctx->config->log_enabled) {
                window_open_log(priv_win);
            }
        }
    }

    /* Mostrar mensaje enviado */
    if (priv_win) {
        char display_msg[MAX_MSG_LEN];
        snprintf(display_msg, sizeof(display_msg), ANSI_CYAN "<%s>" ANSI_RESET " %s", ctx->irc->nick, message);
        wm_add_message_with_timestamp(ctx->wm, priv_win->id, display_msg,
                                       ctx->config->timestamp_enabled,
                                       ctx->config->timestamp_format);
    }
}

/* Comando: window list */
void cmd_window_list(CommandContext *ctx, const char *args) {
    (void)args; /* No usado */

    wm_add_message(ctx->wm, 0, ANSI_BOLD ANSI_CYAN "=== Ventanas abiertas ===" ANSI_RESET);

    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *win = wm_get_window(ctx->wm, i);
        if (win) {
            char msg[MAX_MSG_LEN];
            const char *type_str;

            switch (win->type) {
                case WIN_SYSTEM: type_str = "Sistema"; break;
                case WIN_CHANNEL: type_str = "Canal"; break;
                case WIN_PRIVATE: type_str = "Privado"; break;
                case WIN_LIST: type_str = "Lista"; break;
                default: type_str = "Desconocido"; break;
            }

            const char *active = (i == ctx->wm->active_window) ? ANSI_GREEN " [ACTIVA]" ANSI_RESET : "";

            /* Indicador de actividad para ventanas privadas */
            const char *activity = "";
            if (win->type == WIN_PRIVATE && (win->has_unread || win->is_new)) {
                activity = ANSI_RED " +" ANSI_RESET;
            }

            snprintf(msg, sizeof(msg), ANSI_YELLOW "[%d]" ANSI_RESET " %s - %s%s%s",
                     i, type_str, win->title, active, activity);
            wm_add_message(ctx->wm, 0, msg);
        }
    }
}

/* Comando: window switch (manejado por process_command) */
void cmd_window_switch(CommandContext *ctx, const char *args) {
    int win_id = atoi(args);

    if (win_id < 0 || win_id >= MAX_WINDOWS) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Número de ventana inválido" ANSI_RESET);
        return;
    }

    Window *win = wm_get_window(ctx->wm, win_id);
    if (!win) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_RED "Error: La ventana %d no existe" ANSI_RESET, win_id);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    wm_switch_to(ctx->wm, win_id);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_CYAN "Cambiado a ventana %d: %s" ANSI_RESET, win_id, win->title);
    wm_add_message(ctx->wm, 0, msg);
}

/* Comando: window close */
void cmd_window_close(CommandContext *ctx, const char *args) {
    int win_id;

    /* Si no hay argumentos, cerrar la ventana actual */
    if (!args || args[0] == '\0') {
        win_id = ctx->wm->active_window;
    } else {
        win_id = atoi(args);
    }

    if (win_id < 0 || win_id >= MAX_WINDOWS) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Número de ventana inválido" ANSI_RESET);
        return;
    }

    /* No permitir cerrar ventana 0 sin confirmación */
    if (win_id == 0) {
        wm_add_message(ctx->wm, 0, ANSI_YELLOW "¿Cerrar ventana de sistema? Usa /exit para salir" ANSI_RESET);
        return;
    }

    Window *win = wm_get_window(ctx->wm, win_id);
    if (!win) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_RED "Error: La ventana %d no existe" ANSI_RESET, win_id);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    /* Si es un canal, enviar PART */
    if (win->type == WIN_CHANNEL && ctx->irc->connected) {
        irc_part(ctx->irc, win->title);
    }

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_YELLOW "Cerrando ventana %d: %s" ANSI_RESET, win_id, win->title);
    wm_add_message(ctx->wm, 0, msg);

    wm_close_window(ctx->wm, win_id);
}

/* Comando: clear */
void cmd_clear(CommandContext *ctx, const char *args) {
    (void)args; /* No usado */

    Window *win = wm_get_active_window(ctx->wm);
    if (win && win->buffer) {
        buffer_clear(win->buffer);
        wm_add_message(ctx->wm, win->id, ANSI_GRAY "Pantalla limpiada" ANSI_RESET);
    }
}

/* Comando: buffer */
void cmd_buffer(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /buffer on|off" ANSI_RESET);
        return;
    }

    Window *win = wm_get_active_window(ctx->wm);
    if (!win || !win->buffer) return;

    if (strcmp(args, "on") == 0) {
        win->buffer->enabled = true;
        *(ctx->buffer_enabled) = true;
        wm_add_message(ctx->wm, 0, ANSI_GREEN "Buffer activado" ANSI_RESET);
    } else if (strcmp(args, "off") == 0) {
        win->buffer->enabled = false;
        *(ctx->buffer_enabled) = false;
        wm_add_message(ctx->wm, 0, ANSI_YELLOW "Buffer desactivado - Los mensajes no se almacenarán" ANSI_RESET);
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /buffer on|off" ANSI_RESET);
    }
}

/* Procesar comando */
bool process_command(CommandContext *ctx, const char *input) {
    if (!input || input[0] != '/') return false;

    char command[MAX_INPUT_LEN];
    char args[MAX_INPUT_LEN];

    /* Parsear comando y argumentos */
    const char *space = strchr(input + 1, ' ');
    if (space) {
        int cmd_len = space - (input + 1);
        strncpy(command, input + 1, cmd_len);
        command[cmd_len] = '\0';
        strcpy(args, space + 1);
    } else {
        strcpy(command, input + 1);
        args[0] = '\0';
    }

    /* Comandos especiales /w<n> para cambiar ventana */
    if (command[0] == 'w' && command[1] >= '0' && command[1] <= '9') {
        cmd_window_switch(ctx, command + 1);
        return true;
    }

    /* Buscar en la tabla de comandos */
    for (int i = 0; command_table[i].name != NULL; i++) {
        if (strcmp(command, command_table[i].name) == 0) {
            command_table[i].func(ctx, args);
            return true;
        }
    }

    /* Si no se encuentra, enviar al servidor IRC como comando raw */
    if (ctx->irc->connected) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), "%s %s", command, args);
        irc_send(ctx->irc, msg);

        char display[MAX_MSG_LEN];
        snprintf(display, sizeof(display), ANSI_GRAY ">> %s" ANSI_RESET, msg);
        wm_add_message(ctx->wm, 0, display);
        return true;
    }

    /* Comando no reconocido */
    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_RED "Error: Comando desconocido '%s'" ANSI_RESET, command);
    wm_add_message(ctx->wm, 0, msg);

    return true;
}

/* Comando: silent */
void cmd_silent(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        /* Mostrar estado actual */
        const char *state = *ctx->silent_mode ? "ON" : "OFF";
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_CYAN "Modo silencioso: %s" ANSI_RESET, state);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    if (strcasecmp(args, "on") == 0 || strcasecmp(args, "1") == 0) {
        *ctx->silent_mode = true;
        wm_add_message(ctx->wm, 0, ANSI_GREEN "Modo silencioso activado (JOIN/QUIT/PART ocultos)" ANSI_RESET);
    } else if (strcasecmp(args, "off") == 0 || strcasecmp(args, "0") == 0) {
        *ctx->silent_mode = false;
        wm_add_message(ctx->wm, 0, ANSI_YELLOW "Modo silencioso desactivado" ANSI_RESET);
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /silent on|off" ANSI_RESET);
    }
}

/* Comando: ok */
void cmd_ok(CommandContext *ctx, const char *args) {
    (void)args; /* No usado */

    /* Borrar alertas globales */
    *ctx->notify_alert = false;
    *ctx->mention_alert = false;

    /* Borrar flags de actividad en todas las ventanas */
    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *win = wm_get_window(ctx->wm, i);
        if (win) {
            win->has_unread = false;
            win->is_new = false;
        }
    }

    wm_add_message(ctx->wm, 0, ANSI_GREEN "Todas las notificaciones borradas" ANSI_RESET);
}

/* Comando: log */
void cmd_log(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        /* Mostrar estado actual */
        const char *state = ctx->config->log_enabled ? "ON" : "OFF";
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_CYAN "Logging: %s" ANSI_RESET, state);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    if (strcasecmp(args, "on") == 0 || strcasecmp(args, "1") == 0) {
        ctx->config->log_enabled = true;

        /* Abrir logs para todas las ventanas existentes */
        for (int i = 0; i < MAX_WINDOWS; i++) {
            Window *win = wm_get_window(ctx->wm, i);
            if (win && !win->log_file) {
                window_open_log(win);
            }
        }

        wm_add_message(ctx->wm, 0, ANSI_GREEN "Logging activado - logs en ~/.irclogs/" ANSI_RESET);
    } else if (strcasecmp(args, "off") == 0 || strcasecmp(args, "0") == 0) {
        ctx->config->log_enabled = false;

        /* Cerrar logs de todas las ventanas */
        for (int i = 0; i < MAX_WINDOWS; i++) {
            Window *win = wm_get_window(ctx->wm, i);
            if (win && win->log_file) {
                window_close_log(win);
            }
        }

        wm_add_message(ctx->wm, 0, ANSI_YELLOW "Logging desactivado" ANSI_RESET);
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /log on|off" ANSI_RESET);
    }
}

/* Comando: timestamp */
void cmd_timestamp(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        /* Mostrar estado actual */
        const char *state = ctx->config->timestamp_enabled ? "ON" : "OFF";
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_CYAN "Timestamps: %s (formato: %s)" ANSI_RESET, 
                 state, ctx->config->timestamp_format);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    if (strcasecmp(args, "on") == 0 || strcasecmp(args, "1") == 0) {
        ctx->config->timestamp_enabled = true;
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_GREEN "Timestamps activados (formato: %s)" ANSI_RESET,
                 ctx->config->timestamp_format);
        wm_add_message(ctx->wm, 0, msg);
    } else if (strcasecmp(args, "off") == 0 || strcasecmp(args, "0") == 0) {
        ctx->config->timestamp_enabled = false;
        wm_add_message(ctx->wm, 0, ANSI_YELLOW "Timestamps desactivados" ANSI_RESET);
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /timestamp on|off" ANSI_RESET);
    }
}

/* Comando: ttformat */
void cmd_ttformat(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        /* Mostrar formato actual */
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_CYAN "Formato de timestamp actual: %s" ANSI_RESET,
                 ctx->config->timestamp_format);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    /* Validar formato */
    if (strcmp(args, "HH:MM:SS") == 0) {
        strncpy(ctx->config->timestamp_format, "HH:MM:SS", sizeof(ctx->config->timestamp_format) - 1);
        ctx->config->timestamp_format[sizeof(ctx->config->timestamp_format) - 1] = '\0';
        wm_add_message(ctx->wm, 0, ANSI_GREEN "Formato de timestamp: HH:MM:SS" ANSI_RESET);
    } else if (strcmp(args, "HH:MM") == 0) {
        strncpy(ctx->config->timestamp_format, "HH:MM", sizeof(ctx->config->timestamp_format) - 1);
        ctx->config->timestamp_format[sizeof(ctx->config->timestamp_format) - 1] = '\0';
        wm_add_message(ctx->wm, 0, ANSI_GREEN "Formato de timestamp: HH:MM" ANSI_RESET);
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /ttformat HH:MM:SS o /ttformat HH:MM" ANSI_RESET);
    }
}

/* Comando: list */
void cmd_list(CommandContext *ctx, const char *args) {
    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    /* Buscar si ya existe una ventana LIST */
    Window *list_win = NULL;
    int list_win_id = -1;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *w = wm_get_window(ctx->wm, i);
        if (w && w->type == WIN_LIST) {
            list_win = w;
            list_win_id = i;
            break;
        }
    }

    /* Si no existe, crear ventana LIST */
    if (!list_win) {
        list_win_id = wm_create_window(ctx->wm, WIN_LIST, "Lista de Canales");
        if (list_win_id == -1) {
            wm_add_message(ctx->wm, 0, ANSI_RED "Error: No se pudo crear ventana de lista" ANSI_RESET);
            return;
        }
        list_win = wm_get_window(ctx->wm, list_win_id);

        /* Aplicar configuración del buffer */
        if (list_win && list_win->buffer) {
            list_win->buffer->enabled = ctx->config->buffer_enabled;
        }
    }

    /* Limpiar lista anterior */
    window_clear_channel_list(list_win);
    list_win->list_receiving = true;
    list_win->list_ordered = false;
    list_win->list_filter[0] = '\0';
    list_win->list_limit = 0;
    list_win->list_min_users = 0;
    list_win->list_max_users = 0;

    /* Parsear argumentos */
    bool order = false;
    char search_pattern[256] = "";
    int limit = 0;
    int min_users = 0;
    int max_users = 0;

    if (args && args[0] != '\0') {
        char args_copy[MAX_MSG_LEN];
        strncpy(args_copy, args, sizeof(args_copy) - 1);
        args_copy[sizeof(args_copy) - 1] = '\0';

        char *token = strtok(args_copy, " ");
        while (token) {
            if (strcasecmp(token, "order") == 0) {
                order = true;
            } else if (strcasecmp(token, "search") == 0) {
                /* Siguiente token es el patrón */
                token = strtok(NULL, " ");
                if (token) {
                    strncpy(search_pattern, token, sizeof(search_pattern) - 1);
                    search_pattern[sizeof(search_pattern) - 1] = '\0';
                }
            } else if (strcasecmp(token, "num") == 0) {
                /* Siguiente token es el límite de resultados */
                token = strtok(NULL, " ");
                if (token) {
                    limit = atoi(token);
                    if (limit < 0) limit = 0;
                }
            } else if (strcasecmp(token, "users") == 0) {
                /* Siguiente token es el rango de usuarios o número específico */
                token = strtok(NULL, " ");
                if (token) {
                    /* Verificar si es un rango (contiene '-') */
                    char *dash = strchr(token, '-');
                    if (dash) {
                        /* Es un rango: min-max */
                        *dash = '\0';  /* Separar las dos partes */
                        min_users = atoi(token);
                        max_users = atoi(dash + 1);

                        /* Validar rango */
                        if (min_users < 0) min_users = 0;
                        if (max_users < 0) max_users = 0;
                        if (max_users > 0 && min_users > max_users) {
                            /* Intercambiar si están al revés */
                            int temp = min_users;
                            min_users = max_users;
                            max_users = temp;
                        }
                    } else {
                        /* Es un número específico de usuarios */
                        int users = atoi(token);
                        if (users > 0) {
                            min_users = users;
                            max_users = users;
                        }
                    }
                }
            } else if (token[0] == '*' || token[0] == '#' || token[0] == '?') {
                /* Token que empieza con wildcard o # es un patrón de búsqueda */
                if (search_pattern[0] == '\0') {
                    strncpy(search_pattern, token, sizeof(search_pattern) - 1);
                    search_pattern[sizeof(search_pattern) - 1] = '\0';
                }
            }
            token = strtok(NULL, " ");
        }
    }

    /* Configurar opciones de la lista */
    if (order) {
        list_win->list_ordered = true;
    }

    if (search_pattern[0] != '\0') {
        window_filter_channel_list(list_win, search_pattern);
    }

    if (limit > 0) {
        list_win->list_limit = limit;
    }

    if (min_users > 0 || max_users > 0) {
        list_win->list_min_users = min_users;
        list_win->list_max_users = max_users;
    }

    /* Enviar comando LIST al servidor */
    irc_send(ctx->irc, "LIST");

    /* Cambiar a la ventana de lista */
    wm_switch_to(ctx->wm, list_win_id);

    /* Mostrar mensaje inicial */
    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_BOLD ANSI_CYAN "=== Solicitando lista de canales ===" ANSI_RESET);
    wm_add_message(ctx->wm, list_win_id, msg);

    if (order) {
        wm_add_message(ctx->wm, list_win_id, ANSI_YELLOW "Opción: Ordenar por usuarios" ANSI_RESET);
    }

    if (search_pattern[0] != '\0') {
        snprintf(msg, sizeof(msg), ANSI_YELLOW "Filtro de búsqueda: %s" ANSI_RESET, search_pattern);
        wm_add_message(ctx->wm, list_win_id, msg);
    }

    if (limit > 0) {
        snprintf(msg, sizeof(msg), ANSI_YELLOW "Límite: %d resultados" ANSI_RESET, limit);
        wm_add_message(ctx->wm, list_win_id, msg);
    }

    if (min_users > 0 || max_users > 0) {
        if (min_users > 0 && max_users > 0) {
            snprintf(msg, sizeof(msg), ANSI_YELLOW "Rango de usuarios: %d-%d" ANSI_RESET, min_users, max_users);
        } else if (min_users > 0) {
            snprintf(msg, sizeof(msg), ANSI_YELLOW "Usuarios mínimos: %d" ANSI_RESET, min_users);
        } else {
            snprintf(msg, sizeof(msg), ANSI_YELLOW "Usuarios máximos: %d" ANSI_RESET, max_users);
        }
        wm_add_message(ctx->wm, list_win_id, msg);
    }

    wm_add_message(ctx->wm, list_win_id, ANSI_GRAY "Recibiendo datos del servidor..." ANSI_RESET);
}

/* Comando: raw */
void cmd_raw(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /raw <comando IRC>" ANSI_RESET);
        wm_add_message(ctx->wm, 0, ANSI_GRAY "Ejemplo: /raw WHOIS usuario" ANSI_RESET);
        wm_add_message(ctx->wm, 0, ANSI_GRAY "Ejemplo: /raw MODE #canal +m" ANSI_RESET);
        return;
    }

    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    /* Enviar comando raw al servidor */
    irc_send(ctx->irc, args);

    /* Mostrar comando enviado */
    char display[MAX_MSG_LEN];
    snprintf(display, sizeof(display), ANSI_GRAY ">> %s" ANSI_RESET, args);
    wm_add_message(ctx->wm, 0, display);
}

/* Comando: whois */
void cmd_whois(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /whois <nick>" ANSI_RESET);
        wm_add_message(ctx->wm, 0, ANSI_GRAY "Ejemplo: /whois alice" ANSI_RESET);
        return;
    }

    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    /* Construir comando WHOIS */
    char cmd[MAX_MSG_LEN];
    snprintf(cmd, sizeof(cmd), "WHOIS %s", args);

    /* Enviar comando al servidor */
    irc_send(ctx->irc, cmd);

    /* Mostrar comando enviado */
    char display[MAX_MSG_LEN];
    snprintf(display, sizeof(display), ANSI_GRAY ">> %s" ANSI_RESET, cmd);
    wm_add_message(ctx->wm, 0, display);
}

/* Comando: wii (whois + whowas) */
void cmd_wii(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /wii <nick>" ANSI_RESET);
        wm_add_message(ctx->wm, 0, ANSI_GRAY "Ejemplo: /wii alice" ANSI_RESET);
        wm_add_message(ctx->wm, 0, ANSI_GRAY "Nota: Ejecuta WHOIS y WHOWAS para información completa" ANSI_RESET);
        return;
    }

    if (!ctx->irc->connected) {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: No estás conectado a un servidor" ANSI_RESET);
        return;
    }

    /* Construir y enviar comando WHOIS */
    char cmd_whois[MAX_MSG_LEN];
    snprintf(cmd_whois, sizeof(cmd_whois), "WHOIS %s", args);
    irc_send(ctx->irc, cmd_whois);

    /* Construir y enviar comando WHOWAS */
    char cmd_whowas[MAX_MSG_LEN];
    snprintf(cmd_whowas, sizeof(cmd_whowas), "WHOWAS %s", args);
    irc_send(ctx->irc, cmd_whowas);

    /* Mostrar comandos enviados */
    char display[MAX_MSG_LEN];
    snprintf(display, sizeof(display), ANSI_GRAY ">> %s" ANSI_RESET, cmd_whois);
    wm_add_message(ctx->wm, 0, display);

    snprintf(display, sizeof(display), ANSI_GRAY ">> %s" ANSI_RESET, cmd_whowas);
    wm_add_message(ctx->wm, 0, display);
}

/* Comando: debug */
void cmd_debug(CommandContext *ctx, const char *args) {
    if (!args || args[0] == '\0') {
        /* Mostrar estado actual */
        const char *state = (*ctx->debug_window_id != -1) ? "ON" : "OFF";
        char msg[MAX_MSG_LEN];
        snprintf(msg, sizeof(msg), ANSI_CYAN "Modo debug: %s" ANSI_RESET, state);
        wm_add_message(ctx->wm, 0, msg);
        return;
    }

    if (strcasecmp(args, "on") == 0 || strcasecmp(args, "1") == 0) {
        /* Activar debug */
        if (*ctx->debug_window_id == -1) {
            *ctx->debug_window_id = wm_create_window(ctx->wm, WIN_DEBUG, "[DEBUG]");
            if (*ctx->debug_window_id != -1) {
                wm_add_message(ctx->wm, 0, ANSI_GREEN "Modo debug activado (ventana de depuración creada)" ANSI_RESET);
                wm_switch_to(ctx->wm, *ctx->debug_window_id);
                debug_log(ctx->wm, *ctx->debug_window_id, "=== DEBUG MODE ACTIVADO ===");
                debug_log(ctx->wm, *ctx->debug_window_id, "Esta ventana muestra información de depuración interna");
            } else {
                wm_add_message(ctx->wm, 0, ANSI_RED "Error: No se pudo crear ventana de debug" ANSI_RESET);
            }
        } else {
            wm_add_message(ctx->wm, 0, ANSI_YELLOW "Modo debug ya está activado" ANSI_RESET);
        }
    } else if (strcasecmp(args, "off") == 0 || strcasecmp(args, "0") == 0) {
        /* Desactivar debug */
        if (*ctx->debug_window_id != -1) {
            wm_close_window(ctx->wm, *ctx->debug_window_id);
            *ctx->debug_window_id = -1;
            wm_add_message(ctx->wm, 0, ANSI_YELLOW "Modo debug desactivado" ANSI_RESET);
        } else {
            wm_add_message(ctx->wm, 0, ANSI_YELLOW "Modo debug ya está desactivado" ANSI_RESET);
        }
    } else {
        wm_add_message(ctx->wm, 0, ANSI_RED "Error: Uso /debug on|off" ANSI_RESET);
    }
}

/* Función de logging de debug */
void debug_log(WindowManager *wm, int debug_window_id, const char *format, ...) {
    if (debug_window_id == -1) return;

    char buffer[MAX_MSG_LEN];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    /* Añadir timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "[%H:%M:%S]", tm_info);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), ANSI_GRAY "%s" ANSI_RESET " %s", timestamp, buffer);

    wm_add_message(wm, debug_window_id, msg);
}
