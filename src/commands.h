#ifndef COMMANDS_H
#define COMMANDS_H

#include "common.h"
#include "windows.h"
#include "irc.h"
#include "config.h"

/* Estructura para el contexto de comandos */
typedef struct {
    WindowManager *wm;
    IRCConnection *irc;
    Config *config;
    bool *running;
    bool *buffer_enabled;
    bool *silent_mode;
    bool *notify_alert;
    bool *mention_alert;
} CommandContext;

/* Función de comando */
typedef void (*CommandFunc)(CommandContext *ctx, const char *args);

/* Estructura de comando */
typedef struct {
    const char *name;
    CommandFunc func;
    const char *help;
} Command;

/* Funciones de comandos */
void cmd_help(CommandContext *ctx, const char *args);
void cmd_exit(CommandContext *ctx, const char *args);
void cmd_connect(CommandContext *ctx, const char *args);
void cmd_nick(CommandContext *ctx, const char *args);
void cmd_join(CommandContext *ctx, const char *args);
void cmd_part(CommandContext *ctx, const char *args);
void cmd_msg(CommandContext *ctx, const char *args);
void cmd_window_list(CommandContext *ctx, const char *args);
void cmd_window_switch(CommandContext *ctx, const char *args);
void cmd_window_close(CommandContext *ctx, const char *args);
void cmd_clear(CommandContext *ctx, const char *args);
void cmd_buffer(CommandContext *ctx, const char *args);
void cmd_silent(CommandContext *ctx, const char *args);
void cmd_ok(CommandContext *ctx, const char *args);
void cmd_log(CommandContext *ctx, const char *args);
void cmd_timestamp(CommandContext *ctx, const char *args);
void cmd_ttformat(CommandContext *ctx, const char *args);
void cmd_list(CommandContext *ctx, const char *args);
void cmd_raw(CommandContext *ctx, const char *args);
void cmd_whois(CommandContext *ctx, const char *args);
void cmd_wii(CommandContext *ctx, const char *args);

/* Procesador de comandos */
bool process_command(CommandContext *ctx, const char *input);

#endif /* COMMANDS_H */
