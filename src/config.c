#include "config.h"
#include <ctype.h>

/* Crear estructura de configuración */
Config* config_create(void) {
    Config *cfg = malloc(sizeof(Config));
    if (!cfg) return NULL;

    config_set_defaults(cfg);
    return cfg;
}

/* Destruir configuración */
void config_destroy(Config *cfg) {
    if (cfg) {
        free(cfg);
    }
}

/* Establecer valores por defecto */
void config_set_defaults(Config *cfg) {
    if (!cfg) return;

    cfg->nick[0] = '\0';
    cfg->server[0] = '\0';
    cfg->port = DEFAULT_IRC_PORT;
    cfg->buffer_enabled = true;
    cfg->has_nick = false;
    cfg->has_server = false;
    cfg->silent_mode = false;
    cfg->log_enabled = false;
    cfg->timestamp_enabled = false;
    strncpy(cfg->timestamp_format, "HH:MM:SS", sizeof(cfg->timestamp_format) - 1);
    cfg->timestamp_format[sizeof(cfg->timestamp_format) - 1] = '\0';
    cfg->autojoin_count = 0;
    cfg->notify_count = 0;

    for (int i = 0; i < MAX_AUTOJOIN_CHANNELS; i++) {
        cfg->autojoin_channels[i][0] = '\0';
    }

    for (int i = 0; i < MAX_NOTIFY_NICKS; i++) {
        cfg->notify_nicks[i][0] = '\0';
    }
}

/* Eliminar espacios al inicio y final de una cadena */
static char* trim(char *str) {
    char *end;

    /* Eliminar espacios al inicio */
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    /* Eliminar espacios al final */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

/* Cargar configuración desde archivo */
int config_load(Config *cfg, const char *filename) {
    if (!cfg || !filename) return -1;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        /* No es error crítico si no existe el archivo */
        return 0;
    }

    char line[512];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        /* Eliminar salto de línea */
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';

        /* Eliminar carriage return */
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';

        /* Saltar líneas vacías y comentarios */
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        /* Buscar el separador '=' */
        char *eq = strchr(trimmed, '=');
        if (!eq) {
            /* Línea inválida, ignorar */
            continue;
        }

        /* Separar clave y valor */
        *eq = '\0';
        char *key = trim(trimmed);
        char *value = trim(eq + 1);

        /* Procesar configuración */
        if (strcasecmp(key, "NICK") == 0) {
            if (strlen(value) > 0 && strlen(value) < MAX_NICK_LEN) {
                strncpy(cfg->nick, value, MAX_NICK_LEN - 1);
                cfg->nick[MAX_NICK_LEN - 1] = '\0';
                cfg->has_nick = true;
            }
        }
        else if (strcasecmp(key, "SERVER") == 0) {
            if (strlen(value) > 0 && strlen(value) < MAX_SERVER_LEN) {
                /* Verificar si tiene puerto */
                char *colon = strchr(value, ':');
                if (colon) {
                    *colon = '\0';
                    int port_val = atoi(colon + 1);
                    if (port_val > 0 && port_val < 65536) {
                        cfg->port = port_val;
                    }
                }

                strncpy(cfg->server, value, MAX_SERVER_LEN - 1);
                cfg->server[MAX_SERVER_LEN - 1] = '\0';
                cfg->has_server = true;
            }
        }
        else if (strcasecmp(key, "BUFFER") == 0) {
            if (strcasecmp(value, "on") == 0 || strcasecmp(value, "yes") == 0 ||
                strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                cfg->buffer_enabled = true;
            }
            else if (strcasecmp(value, "off") == 0 || strcasecmp(value, "no") == 0 ||
                     strcasecmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                cfg->buffer_enabled = false;
            }
        }
        else if (strcasecmp(key, "SILENT") == 0) {
            if (strcasecmp(value, "on") == 0 || strcasecmp(value, "yes") == 0 ||
                strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                cfg->silent_mode = true;
            }
            else if (strcasecmp(value, "off") == 0 || strcasecmp(value, "no") == 0 ||
                     strcasecmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                cfg->silent_mode = false;
            }
        }
        else if (strcasecmp(key, "LOG") == 0) {
            if (strcasecmp(value, "on") == 0 || strcasecmp(value, "yes") == 0 ||
                strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                cfg->log_enabled = true;
            }
            else if (strcasecmp(value, "off") == 0 || strcasecmp(value, "no") == 0 ||
                     strcasecmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                cfg->log_enabled = false;
            }
        }
        else if (strcasecmp(key, "TIMESTAMP") == 0) {
            if (strcasecmp(value, "on") == 0 || strcasecmp(value, "yes") == 0 ||
                strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                cfg->timestamp_enabled = true;
            }
            else if (strcasecmp(value, "off") == 0 || strcasecmp(value, "no") == 0 ||
                     strcasecmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                cfg->timestamp_enabled = false;
            }
        }
        else if (strcasecmp(key, "TTFORMAT") == 0) {
            /* Validar formato: debe ser "HH:MM:SS" o "HH:MM" */
            if (strcmp(value, "HH:MM:SS") == 0 || strcmp(value, "HH:MM") == 0) {
                strncpy(cfg->timestamp_format, value, sizeof(cfg->timestamp_format) - 1);
                cfg->timestamp_format[sizeof(cfg->timestamp_format) - 1] = '\0';
            }
        }
        else if (strcasecmp(key, "AUTOJOIN") == 0) {
            /* Parsear lista de canales separados por comas */
            char *token = strtok(value, ",");
            while (token && cfg->autojoin_count < MAX_AUTOJOIN_CHANNELS) {
                char *trimmed_token = trim(token);
                if (strlen(trimmed_token) > 0) {
                    /* Añadir # si no lo tiene */
                    if (trimmed_token[0] != '#') {
                        snprintf(cfg->autojoin_channels[cfg->autojoin_count],
                                MAX_CHANNEL_LEN, "#%s", trimmed_token);
                    } else {
                        strncpy(cfg->autojoin_channels[cfg->autojoin_count],
                               trimmed_token, MAX_CHANNEL_LEN - 1);
                    }
                    cfg->autojoin_channels[cfg->autojoin_count][MAX_CHANNEL_LEN - 1] = '\0';
                    cfg->autojoin_count++;
                }
                token = strtok(NULL, ",");
            }
        }
        else if (strcasecmp(key, "NOTIFY") == 0) {
            /* Parsear lista de nicks separados por comas */
            char *token = strtok(value, ",");
            while (token && cfg->notify_count < MAX_NOTIFY_NICKS) {
                char *trimmed_token = trim(token);
                if (strlen(trimmed_token) > 0) {
                    strncpy(cfg->notify_nicks[cfg->notify_count],
                           trimmed_token, MAX_NICK_LEN - 1);
                    cfg->notify_nicks[cfg->notify_count][MAX_NICK_LEN - 1] = '\0';
                    cfg->notify_count++;
                }
                token = strtok(NULL, ",");
            }
        }
    }

    fclose(fp);
    return 0;
}
