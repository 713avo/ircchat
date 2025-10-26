#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <wchar.h>

/* Definiciones de caracteres Unicode para marcos */
#define UNICODE_HLINE "\u2500"      /* ─ línea horizontal */
#define UNICODE_VLINE "\u2502"      /* │ línea vertical */
#define UNICODE_JUNCTION "\u2534"   /* ┴ unión T invertida */

/* Constantes del sistema */
#define MAX_WINDOWS 20
#define MAX_NICK_LEN 32
#define MAX_SERVER_LEN 256
#define MAX_CHANNEL_LEN 64
#define MAX_MSG_LEN 512
#define MAX_INPUT_LEN 512
#define COMMAND_HISTORY_SIZE 15
#define DEFAULT_IRC_PORT 6667

/* Tipos de ventanas */
typedef enum {
    WIN_SYSTEM,
    WIN_CHANNEL,
    WIN_PRIVATE,
    WIN_LIST,
    WIN_DEBUG
} WindowType;

/* Códigos de escape ANSI para colores */
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"
#define ANSI_GRAY "\033[90m"

/* Códigos de escape ANSI para posicionamiento */
#define ANSI_CLEAR_SCREEN "\033[2J"
#define ANSI_HOME "\033[H"
#define ANSI_CLEAR_LINE "\033[2K"
#define ANSI_SAVE_CURSOR "\033[s"
#define ANSI_RESTORE_CURSOR "\033[u"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"

/* Macros útiles */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif /* COMMON_H */
