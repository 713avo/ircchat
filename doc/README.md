# Cliente IRC en C

Cliente IRC completo en C con interfaz de terminal, diseñado para ser ligero, rápido y funcional.

## Características

### 🎨 Interfaz
- **Múltiples ventanas**: Sistema, canales y mensajes privados
- **Lista de usuarios**: Muestra usuarios en canales con prefijos de modo (@, +, %, ~, &)
- **Word wrap**: Los mensajes largos se ajustan automáticamente al ancho de terminal
- **Scroll**: Desplazamiento en buffer de mensajes y lista de usuarios
- **UTF-8**: Soporte completo para caracteres UTF-8
- **Colores mIRC**: Renderiza códigos de color mIRC (^C, ^B, ^U, etc.)

### ⏰ Timestamps
- **Timestamps configurables**: Muestra hora en mensajes de canales y privados
- **Formatos disponibles**: HH:MM:SS o HH:MM
- **Comando**: `/timestamp on|off` y `/ttformat HH:MM:SS|HH:MM`
- **En logs**: Siempre se usa formato HH:MM:SS

### 📝 Logging
- **Logs automáticos**: Guarda conversaciones en `~/.irclogs/`
- **Formato de archivos**: `tipo_nombre_DD-MM-YY_HH:MM.txt`
  - Ejemplo: `canal_music_12-03-25_21:33.txt`
  - Ejemplo: `privado_alice_23-04-22_03:24.txt`
- **Timestamps siempre incluidos**: Formato [HH:MM:SS] en cada línea
- **Cambio de día**: Se indica automáticamente en el log
- **Sin códigos ANSI**: Los logs se guardan limpios
- **Comando**: `/log on|off`

### 🔔 Notificaciones
- **Sistema NOTIFY**: Vigila nicks específicos cada minuto
- **Indicador C** (verde, parpadeante): Nick vigilado se conectó
- **Indicador M** (magenta, parpadeante): Te mencionaron en un canal
- **Indicador \*** (rojo, parpadeante): Nueva ventana privada
- **Indicador +** (amarillo, parpadeante): Mensajes sin leer
- **Múltiples simultáneos**: Todos los indicadores activos se muestran juntos (ej: `C M +`)
- **Parpadeo visual**: Alternan entre normal y reverse video para máxima visibilidad
- **Comando**: `/ok` para limpiar todas las notificaciones (C, M, *, +)

### ⚡ Autocompletado
- **TAB en canales**: Autocompleta nicks de usuarios
- **Rotación**: Presiona TAB múltiples veces para rotar opciones
- **Auto-colon**: Si el nick está al inicio, añade `:` automáticamente
- **Case-insensitive**: Funciona sin importar mayúsculas/minúsculas

### 🤫 Modo Silencioso
- **Oculta ruido**: JOIN, QUIT, PART y PRIVMSG no aparecen en ventana sistema
- **Eventos visibles**: JOIN, QUIT, PART siempre se muestran en canales respectivos
- **Logs completos**: Todos los eventos se registran en logs independientemente del modo
- **Comando**: `/silent on|off`

### 🚀 Auto-join
- **Conexión automática**: Únete a canales al conectar
- **Configuración**: `AUTOJOIN=#linux,#python,programming`
- **# opcional**: El programa lo añade si falta

## Instalación

```bash
# Clonar el repositorio
git clone https://github.com/tu-usuario/ircchat.git
cd ircchat

# Compilar
make

# El binario estará en bin/ircchat
./bin/ircchat
```

## Configuración

### Archivo de configuración

Copia el archivo de ejemplo a tu home:

```bash
cp .ircchat.rc.example ~/.ircchat.rc
```

Edita `~/.ircchat.rc` con tu editor favorito:

```ini
# Conexión
NICK=minick
SERVER=irc.libera.chat:6667

# Opciones
BUFFER=on
SILENT=off
LOG=on
TIMESTAMP=on
TTFORMAT=HH:MM:SS

# Auto-join
AUTOJOIN=#linux,#python,programming

# Notify
NOTIFY=alice,bob,charlie
```

### Parámetros disponibles

| Parámetro | Valores | Descripción |
|-----------|---------|-------------|
| `NICK` | texto | Nick por defecto |
| `SERVER` | servidor[:puerto] | Servidor IRC |
| `BUFFER` | on/off | Buffer de mensajes |
| `SILENT` | on/off | Modo silencioso |
| `LOG` | on/off | Logging automático |
| `TIMESTAMP` | on/off | Timestamps en mensajes |
| `TTFORMAT` | HH:MM:SS o HH:MM | Formato de timestamp |
| `AUTOJOIN` | #canal,#canal | Canales auto-join |
| `NOTIFY` | nick,nick | Nicks a vigilar |

## Comandos

### Conexión y sesión
- `/connect [servidor] [puerto]` - Conectar a servidor
- `/nick <nickname>` - Cambiar nickname
- `/exit` o `/quit` - Salir del programa

### Canales
- `/join <#canal>` - Unirse a un canal
- `/part` - Salir del canal actual
- `/list [num <n>] [order] [search <patrón>]` - Listar canales del servidor
  - `num <n>` - Limitar a n resultados
  - `order` - Ordenar por número de usuarios (mayor a menor)
  - `search <patrón>` - Filtrar por patrón (wildcards * y ?)
  - Ejemplos: `/list`, `/list order`, `/list num 10 *linux* order`

### Mensajes
- `/msg <nick> <mensaje>` - Enviar mensaje privado
- Escribe directamente en canales sin comando

### Ventanas
- `/wl` - Listar ventanas abiertas
- `/wc [n]` - Cerrar ventana (actual o número)
- `/w1`, `/w2`, etc. - Cambiar a ventana específica
- `/clear` - Limpiar pantalla actual

### Configuración en tiempo real
- `/buffer on|off` - Activar/desactivar buffer
- `/silent on|off` - Activar/desactivar modo silencioso
- `/log on|off` - Activar/desactivar logging
- `/timestamp on|off` - Activar/desactivar timestamps
- `/ttformat HH:MM:SS` - Formato largo de timestamp
- `/ttformat HH:MM` - Formato corto de timestamp

### Utilidades
- `/ok` - Borrar todas las notificaciones (C, M, *, +)
- `/help` - Mostrar ayuda

### Comandos IRC avanzados
- `/raw <comando>` - Enviar comando IRC raw al servidor
  - Ejemplos: `/raw WHOIS usuario`, `/raw MODE #canal +m`, `/raw TOPIC #canal :Nuevo topic`
  - Permite usar cualquier comando IRC no implementado directamente

## Atajos de teclado

### Navegación de ventanas
- `Alt+0-9` - Cambiar a ventana 0-9
- `Alt+→` - Siguiente ventana (cíclico)
- `Alt+←` - Ventana anterior (cíclico)
- `Alt+.` - Limpiar pantalla actual

### Scroll de buffer
- `Ctrl+↑` - Scroll up en mensajes
- `Ctrl+↓` - Scroll down en mensajes
- `Ctrl+B` - Ir al inicio del buffer
- `Ctrl+E` - Ir al final del buffer

### Scroll de usuarios (en canales)
- `Ctrl+Shift+↑` - Scroll up en lista usuarios
- `Ctrl+Shift+↓` - Scroll down en lista usuarios
- `Ctrl+Shift+←` - Scroll izquierda (nicks largos)
- `Ctrl+Shift+→` - Scroll derecha (nicks largos)

### Otros
- `↑/↓` - Navegar historial de comandos
- `TAB` - Autocompletar nicks en canales
- `Ctrl+L` - Redibujar interfaz
- `Ctrl+C` - Salir del programa

## Colores mIRC

El cliente soporta los siguientes códigos de formato mIRC:

| Código | Efecto |
|--------|--------|
| `^C + número` | Color (0-15) |
| `^B` | Negrita |
| `^U` | Subrayado |
| `^O` o `^R` | Reset |
| `^V` | Video reverso |

### Colores disponibles
0=Blanco, 1=Negro, 2=Azul, 3=Verde, 4=Rojo, 5=Marrón, 6=Magenta, 7=Naranja, 8=Amarillo, 9=Verde claro, 10=Cyan, 11=Cyan claro, 12=Azul claro, 13=Rosa, 14=Gris, 15=Gris claro

## Ejemplos de uso

### Conexión básica
```
/connect irc.libera.chat
/nick miusuario
/join #linux
```

### Con timestamps
```
/timestamp on
/ttformat HH:MM
# Ahora los mensajes aparecerán con formato: 14:35> <usuario> mensaje
```

### Logging activado
```
/log on
# Los logs se guardarán en ~/.irclogs/
# Ejemplo: ~/.irclogs/canal_linux_12-03-25_14:35.txt
```

### Autocompletar
```
# En un canal, escribe las primeras letras de un nick y presiona TAB
Al<TAB>  -> Completa a "Alice: " si estás al inicio de línea
```

### Notificaciones
```
# Configura NOTIFY en ~/.ircchat.rc
NOTIFY=alice,bob

# Cuando alice o bob se conecten, verás:
> [comando]                                                           C
# Presiona /ok para limpiar
```

## Estructura de archivos

```
ircchat/
├── src/
│   ├── main.c           - Bucle principal y procesamiento IRC
│   ├── terminal.c/.h    - Manejo de terminal y rendering
│   ├── windows.c/.h     - Gestión de ventanas y mensajes
│   ├── buffer.c/.h      - Buffer de mensajes con scroll
│   ├── irc.c/.h         - Protocolo IRC
│   ├── commands.c/.h    - Comandos de usuario
│   ├── input.c/.h       - Manejo de entrada y teclas
│   ├── config.c/.h      - Configuración
│   └── common.h         - Definiciones comunes
├── doc/                - Documentación adicional
├── bin/                - Binarios compilados (ignorado por git)
├── Makefile            - Script de compilación
├── .ircchat.rc.example - Configuración de ejemplo
├── .gitignore          - Archivos ignorados por git
├── LICENSE             - Licencia MIT
└── README.md           - Este archivo
```

## Requisitos

- Compilador GCC con soporte C11
- Sistema Unix/Linux (probado en Termux)
- Terminal con soporte ANSI y UTF-8

## Compilación

El proyecto usa un Makefile simple:

```bash
# Compilar
make

# Limpiar
make clean

# Recompilar todo
make clean && make
```

## Licencia

Este proyecto está bajo la Licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.

## Características futuras

- [ ] Soporte SSL/TLS
- [ ] Configuración de colores personalizables
- [ ] Temas de color
- [ ] Búsqueda en buffer
- [ ] Scripts de automatización
- [ ] Notificaciones de sistema (libnotify)

## Créditos

Desarrollado con Claude Code - Cliente IRC moderno en C.
