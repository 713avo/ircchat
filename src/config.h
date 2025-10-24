#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

/* Lista de canales para autojoin */
#define MAX_AUTOJOIN_CHANNELS 10

/* Lista de nicks para notify */
#define MAX_NOTIFY_NICKS 20

/* Estructura de configuración */
typedef struct {
    char nick[MAX_NICK_LEN];
    char server[MAX_SERVER_LEN];
    int port;
    bool buffer_enabled;
    bool has_nick;
    bool has_server;
    bool silent_mode;
    bool log_enabled;
    bool timestamp_enabled;
    char timestamp_format[16];  /* "HH:MM:SS" o "HH:MM" */
    char autojoin_channels[MAX_AUTOJOIN_CHANNELS][MAX_CHANNEL_LEN];
    int autojoin_count;
    char notify_nicks[MAX_NOTIFY_NICKS][MAX_NICK_LEN];
    int notify_count;
} Config;

/* Funciones de configuración */
Config* config_create(void);
void config_destroy(Config *cfg);
int config_load(Config *cfg, const char *filename);
void config_set_defaults(Config *cfg);

#endif /* CONFIG_H */
