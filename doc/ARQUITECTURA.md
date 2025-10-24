# Arquitectura del Cliente IRC

## Visión General

El cliente IRC está diseñado de forma modular, con cada módulo responsable de una funcionalidad específica.

## Módulos

### 1. common.h - Definiciones Comunes

Contiene:
- Constantes globales del sistema
- Definiciones de caracteres Unicode para marcos
- Códigos de escape ANSI para colores y posicionamiento
- Enumeraciones de tipos de ventanas
- Macros útiles (MIN, MAX)

### 2. buffer.c/h - Gestión de Buffer de Mensajes

**Responsabilidad**: Almacenar y gestionar el historial de mensajes.

**Estructuras principales**:
```c
typedef struct MessageNode {
    char *message;
    struct MessageNode *next;
    struct MessageNode *prev;
} MessageNode;

typedef struct {
    MessageNode *head;
    MessageNode *tail;
    MessageNode *current_view;
    int count;
    int view_offset;
    bool enabled;
} MessageBuffer;
```

**Funciones clave**:
- `buffer_create()` - Crear nuevo buffer
- `buffer_add_message()` - Añadir mensaje
- `buffer_scroll_up/down()` - Navegar por historial
- `buffer_get_visible_messages()` - Obtener mensajes para renderizar

**Características**:
- Lista doblemente enlazada para navegación bidireccional
- Soporte para buffer activado/desactivado
- Navegación por vista (scroll) independiente del final del buffer

### 3. windows.c/h - Gestión de Ventanas

**Responsabilidad**: Gestionar múltiples ventanas (sistema, canales, privados).

**Estructuras principales**:
```c
typedef struct {
    int id;
    WindowType type;
    char title[MAX_CHANNEL_LEN];
    MessageBuffer *buffer;
    UserNode *users;
    int user_count;
} Window;

typedef struct {
    Window *windows[MAX_WINDOWS];
    int active_window;
    int window_count;
} WindowManager;
```

**Funciones clave**:
- `wm_create()` - Crear gestor de ventanas
- `wm_create_window()` - Crear nueva ventana
- `wm_switch_to()` - Cambiar ventana activa
- `wm_add_message()` - Añadir mensaje a ventana
- `window_add/remove_user()` - Gestionar usuarios en canales

**Características**:
- Array de punteros para acceso O(1) por ID
- Ventana 0 siempre es la ventana de sistema
- Cada ventana tiene su propio buffer de mensajes
- Canales mantienen lista de usuarios

### 4. terminal.c/h - Control del Terminal

**Responsabilidad**: Manejo del terminal, modo raw, y renderizado de interfaz.

**Estructuras principales**:
```c
typedef struct {
    struct termios original_termios;
    int rows;
    int cols;
    bool raw_mode;
} TerminalState;
```

**Funciones clave**:
- `term_init()` - Inicializar terminal
- `term_enter_raw_mode()` - Activar modo raw
- `term_draw_interface()` - Dibujar interfaz completa
- `term_draw_channel_window()` - Dibujar ventana de canal
- `term_move_cursor()` - Posicionar cursor

**Características**:
- Modo raw para captura inmediata de teclas
- Uso de secuencias ANSI para posicionamiento y colores
- Renderizado diferenciado por tipo de ventana
- Soporte para redimensionamiento del terminal

### 5. irc.c/h - Conexión y Protocolo IRC

**Responsabilidad**: Gestionar conexión TCP y protocolo IRC.

**Estructuras principales**:
```c
typedef struct {
    int sockfd;
    bool connected;
    char server[MAX_SERVER_LEN];
    int port;
    char nick[MAX_NICK_LEN];
    time_t last_ping;
    time_t last_pong;
} IRCConnection;
```

**Funciones clave**:
- `irc_connect()` - Conectar a servidor
- `irc_send_raw()` - Enviar comando raw
- `irc_privmsg()` - Enviar mensaje privado
- `irc_join/part()` - Unirse/salir de canal
- `irc_process_message()` - Procesar mensajes recibidos

**Características**:
- Socket no bloqueante
- Gestión automática de PING/PONG
- Soporte para comandos IRC básicos
- Parser básico de mensajes IRC

### 6. input.c/h - Manejo de Entrada

**Responsabilidad**: Capturar y procesar entrada del usuario.

**Estructuras principales**:
```c
typedef struct {
    char history[COMMAND_HISTORY_SIZE][MAX_INPUT_LEN];
    int count;
    int current;
    int position;
} CommandHistory;

typedef struct {
    char line[MAX_INPUT_LEN];
    int cursor_pos;
    int length;
    CommandHistory history;
} InputState;
```

**Funciones clave**:
- `input_read_key()` - Leer tecla del terminal
- `input_add_char()` - Añadir carácter
- `input_backspace()` - Borrar carácter
- `input_history_prev/next()` - Navegar historial

**Características**:
- Buffer circular de comandos (15 entradas)
- Detección de secuencias de escape (flechas, Ctrl+teclas)
- Edición de línea con inserción en medio
- Historial de comandos sin duplicados consecutivos

### 7. commands.c/h - Procesamiento de Comandos

**Responsabilidad**: Ejecutar comandos del usuario.

**Estructuras principales**:
```c
typedef struct {
    WindowManager *wm;
    IRCConnection *irc;
    bool *running;
    bool *buffer_enabled;
} CommandContext;

typedef struct {
    const char *name;
    CommandFunc func;
    const char *help;
} Command;
```

**Funciones clave**:
- `process_command()` - Parsear y ejecutar comando
- `cmd_connect()` - Comando /connect
- `cmd_join()` - Comando /join
- `cmd_window_switch()` - Cambiar de ventana

**Características**:
- Tabla de comandos para dispatch
- Comandos no reconocidos se envían al servidor
- Validación de parámetros
- Retroalimentación a usuario en ventana sistema

### 8. main.c - Programa Principal

**Responsabilidad**: Bucle principal y coordinación de módulos.

**Flujo principal**:
1. Inicializar subsistemas (terminal, ventanas, IRC, input)
2. Configurar manejadores de señales
3. Bucle principal:
   - Usar `select()` para I/O multiplexado
   - Procesar mensajes IRC
   - Procesar entrada de usuario
   - Redibujar interfaz según necesidad
4. Limpieza al salir

**Características**:
- I/O no bloqueante con `select()`
- Gestión de señales (SIGINT)
- Procesamiento de mensajes IRC en tiempo real
- Actualización automática de interfaz

## Flujo de Datos

### Envío de Mensaje

```
Usuario escribe texto
    ↓
input.c captura teclas
    ↓
main.c detecta ENTER
    ↓
¿Es comando (/)?
    ├─ Sí → commands.c procesa
    │         ↓
    │       irc.c envía al servidor
    │         ↓
    │       windows.c añade a buffer
    │
    └─ No → Mensaje directo
              ↓
            irc.c envía a canal/privado
              ↓
            windows.c añade a buffer
              ↓
            terminal.c redibuja
```

### Recepción de Mensaje

```
Servidor IRC envía datos
    ↓
select() detecta datos en socket
    ↓
irc.c recibe y parsea
    ↓
main.c procesa mensaje IRC
    ↓
¿Tipo de mensaje?
    ├─ PING → irc.c responde PONG
    ├─ PRIVMSG → windows.c añade a buffer apropiado
    ├─ JOIN → windows.c añade usuario a canal
    └─ PART → windows.c elimina usuario
    ↓
terminal.c redibuja interfaz
```

## Patrones de Diseño

### 1. Separación de Responsabilidades

Cada módulo tiene una responsabilidad clara y bien definida.

### 2. Gestión de Recursos

- Funciones `*_create()` y `*_destroy()` para inicialización/limpieza
- Liberación explícita de memoria
- Restauración del estado del terminal al salir

### 3. Abstracción de Capas

```
┌─────────────────────────┐
│   main.c (Control)      │
├─────────────────────────┤
│ commands.c (Lógica)     │
├─────────────────────────┤
│ windows.c  │  irc.c     │
│ input.c    │  terminal.c│
├─────────────────────────┤
│ buffer.c (Datos)        │
└─────────────────────────┘
```

### 4. Contexto de Comandos

Los comandos reciben un `CommandContext` con todas las referencias necesarias, evitando variables globales.

## Consideraciones de Rendimiento

### Buffer de Mensajes

- Lista enlazada permite crecimiento dinámico
- Modo sin buffer para reducir uso de memoria
- Navegación O(n) pero limitada por tamaño de pantalla

### Renderizado

- Solo redibujar cuando hay cambios
- Usar ANSI para actualización eficiente
- Ocultar cursor durante redibujado

### Red

- Socket no bloqueante evita congelación
- `select()` permite I/O multiplexado eficiente
- Buffer de recepción de 4KB

## Extensibilidad

### Añadir Nuevo Comando

1. Añadir prototipo en `commands.h`
2. Implementar función en `commands.c`
3. Añadir entrada a `command_table[]`

### Añadir Nuevo Tipo de Ventana

1. Añadir tipo a enum `WindowType` en `common.h`
2. Implementar función de dibujado en `terminal.c`
3. Añadir caso en `term_draw_interface()`

### Añadir Nuevo Atajo de Teclado

1. Definir código en `input.h` (enum `KeyCode`)
2. Añadir detección en `input_read_key()`
3. Procesar en bucle principal de `main.c`

## Mejoras Futuras Posibles

1. **SSL/TLS**: Añadir soporte para conexiones cifradas
2. **Configuración**: Archivo de configuración (.ircrc)
3. **Logging**: Guardar conversaciones en archivos
4. **Notificaciones**: Alertas de menciones
5. **Autocompletado**: Tab completion para nicks y comandos
6. **Formateo IRC**: Soporte para colores IRC (^C codes)
7. **CTCP**: Soporte para VERSION, TIME, etc.
8. **DCC**: Transferencia de archivos
9. **Modo servidor**: Actuar como servidor IRC simple
10. **Scripting**: Plugin system con scripts Lua/Python
