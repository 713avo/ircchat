# Changelog - Cliente IRC

## Versión 1.1.0 - 26 de Octubre de 2025

### 📋 Resumen de la Versión

Esta versión introduce mejoras significativas en renderizado, comandos de información de usuarios y filtrado avanzado de canales.

**Principales mejoras**:
- ✨ Word wrap con preservación de colores ANSI
- 🔍 Comandos `/whois` y `/wii` (con WHOWAS)
- 📊 Filtrado por rango de usuarios en `/list`
- 🐛 Corrección de autojoin incorrecto

**Archivos modificados**:
- `src/terminal.c` - Algoritmo de word wrap mejorado
- `src/commands.c` - Nuevos comandos whois/wii y filtrado list
- `src/windows.h` - Campos para filtro de usuarios
- `src/main.c` - Fix detección RPL_WELCOME

---

### Nuevas Funcionalidades

#### Word Wrap con Preservación de Colores

**Corrección crítica en el renderizado de mensajes largos**:
- **Problema corregido**: Las líneas largas con formato de color perdían el color al hacer word wrap
- **Solución implementada** (`src/terminal.c:10-109`):
  - Nueva función `wrap_text()` que rastrea códigos ANSI activos
  - Buffer de códigos activos (`active_codes`) que se preserva entre líneas
  - Detección de códigos reset (`\033[0m`) para limpiar formatos
  - Re-aplicación automática de formatos al inicio de líneas continuadas
- **Comportamiento mejorado**:
  - Los colores se mantienen consistentes en todas las líneas wrapeadas
  - La negrita, subrayado y otros formatos también se preservan
  - Los códigos reset funcionan correctamente limpiando formatos
- **Impacto**: Mejora significativa en la legibilidad de mensajes largos con formato

#### Comando `/whois` y `/wii` Mejorado

**Nuevo comando para obtener información de usuarios**:
- Comando `/whois <nick>` - Obtiene información de usuario conectado (solo WHOIS)
- Comando `/wii <nick>` - Obtiene información completa (WHOIS + WHOWAS)
- **Diferencia clave**:
  - `/whois` envía solo WHOIS (información de usuario actualmente conectado)
  - `/wii` envía WHOIS y WHOWAS (información actual + historial)
- **Ventaja de WHOWAS**:
  - Proporciona información de usuarios que ya se desconectaron
  - Muestra última vez que estuvieron conectados
  - Útil para verificar identidad de nicks desconectados

**Información típica proporcionada**:
- Hostname y dirección del usuario
- Nombre real configurado
- Canales en los que está (WHOIS) o estuvo (WHOWAS)
- Servidor IRC al que está conectado
- Tiempo de inactividad (idle time)
- Si es operador del servidor
- Última vez conectado (WHOWAS)

**Ejemplos de uso**:
```
/whois alice          # Solo información actual de alice
/wii bob              # Información actual + historial de bob
/wii charlie          # Funciona incluso si charlie ya se desconectó
```

#### Comando `/list` con Filtro por Usuarios

**Filtrado avanzado de canales por número de usuarios**:
- Nueva opción `users <n>` - Filtrar por número exacto de usuarios
  - Ejemplo: `/list users 50` - Solo canales con exactamente 50 usuarios
- Nueva opción `users <min>-<max>` - Filtrar por rango de usuarios
  - Ejemplo: `/list users 10-40` - Solo canales con entre 10 y 40 usuarios
- La opción `num` ahora se distingue claramente como límite de resultados
  - `num <n>` - Limita cuántos canales mostrar en total
  - `users <rango>` - Filtra qué canales incluir según usuarios
- Todas las opciones son combinables en cualquier orden
  - Ejemplo completo: `/list num 10 users 10-40 order search *linux*`
    - Máximo 10 canales
    - Con 10-40 usuarios
    - Ordenados por usuarios
    - Que contengan "linux" en el nombre

**Orden de procesamiento**:
1. Filtro por rango de usuarios (`users`)
2. Filtro por patrón de búsqueda (`search`)
3. Ordenamiento por usuarios (`order`)
4. Límite de resultados (`num`)

**Mensajes informativos**:
- La ventana LIST muestra todos los filtros aplicados
- Se indica el rango de usuarios al inicio y al final de la lista
- Formato claro: "Rango de usuarios: 10-40" o "Usuarios mínimos: 10"

### Correcciones de Bugs

#### Fix: Autojoin Incorrecto en Comando `/list`

**Problema corregido**:
- Al ejecutar `/list`, se activaba incorrectamente el autojoin
- Se creaban ventanas duplicadas de canales ya unidos
- Las ventanas duplicadas aparecían vacías (sin mensajes ni usuarios)

**Causa**:
- La detección del mensaje RPL_WELCOME (001) era demasiado permisiva
- Buscaba la palabra "Welcome" en cualquier mensaje del servidor
- Mensajes de la lista de canales podían contener esa palabra

**Solución implementada** (`src/main.c:72`):
- Detección más estricta del mensaje 001
- Verifica el código numérico " 001 " (con espacios)
- Verifica que el mensaje comience con ':' (formato estándar IRC)
- Condición: `if (strstr(line, " 001 ") != NULL && line[0] == ':')`

**Resultado**:
- El autojoin ahora solo se ejecuta al conectar inicialmente
- `/list` ya no genera ventanas duplicadas
- Comportamiento más confiable y predecible

### Cambios Técnicos

#### Algoritmo de Word Wrap Mejorado

**Función `wrap_text()` reescrita (`src/terminal.c:10-109`)**:
- **Buffer de códigos activos**: Array de 512 bytes para almacenar códigos ANSI
- **Rastreo de formatos**: Detecta y guarda todos los códigos ANSI encontrados
- **Detección de reset**: Identifica `\033[0m` y limpia el buffer de códigos
- **Preservación entre líneas**:
  - Línea N termina con `\033[0m` (reset)
  - Línea N+1 comienza con códigos activos de línea N
- **Manejo de memoria**: Calcula tamaño dinámicamente considerando códigos preservados
- **Algoritmo**:
  1. Procesar texto carácter por carácter
  2. Saltar y guardar secuencias ANSI (no cuentan para ancho)
  3. Contar solo caracteres visibles UTF-8
  4. Al hacer break: añadir reset al final, códigos al inicio de siguiente línea

#### Comandos WHOIS/WHOWAS

**Nueva función `cmd_wii()` (`src/commands.c:803-834`)**:
- Envía dos comandos IRC secuenciales: WHOIS seguido de WHOWAS
- Usa `irc_send()` dos veces con el mismo nick
- Muestra confirmación de ambos comandos enviados
- Diferenciada de `cmd_whois()` que solo envía WHOIS

**Función `cmd_whois()` modificada (`src/commands.c:777-801`)**:
- Simplificada para solo enviar WHOIS
- Eliminada referencia a alias /wii en la ayuda
- Validación de argumentos y conexión

#### Estructura de Datos

**Nuevos campos en `Window` (`src/windows.h`)**:
- `int list_min_users` - Filtro mínimo de usuarios (0 = sin mínimo)
- `int list_max_users` - Filtro máximo de usuarios (0 = sin máximo)

**Inicialización**:
- Los campos se inicializan en 0 al crear ventanas
- Se resetean al limpiar la lista de canales

#### Parsing de Comandos

**Función `cmd_list()` actualizada (`src/commands.c`)**:
- Nuevo parsing para opción `users`:
  - Detecta formato de rango con guión: `10-40`
  - Detecta número específico: `50`
  - Valida y corrige rangos invertidos automáticamente
- Separación clara entre `num` (límite) y `users` (filtro)
- Soporte para combinación de todas las opciones

#### Filtrado de Canales

**Función `window_finalize_channel_list()` mejorada (`src/windows.c`)**:
- Nuevo filtrado por rango de usuarios antes de otros filtros
- Verifica mínimo y máximo de usuarios por canal
- Elimina canales que no cumplan el rango
- Actualiza el contador de canales correctamente
- Muestra información del rango aplicado en el resumen

### Comandos Nuevos y Actualizados

#### `/whois` y `/wii` - Información de Usuario

```
/whois <nick>        # Solo WHOIS
/wii <nick>          # WHOIS + WHOWAS
```

**Descripción**:
- `/whois` obtiene información solo de usuarios actualmente conectados
- `/wii` obtiene información completa: usuarios conectados Y histórico
- La respuesta del servidor incluye múltiples mensajes con información completa
- **Diferencia principal**: WHOWAS funciona con usuarios desconectados

**Implementación** (`src/commands.c`):
- Función `cmd_whois()` (líneas 777-801): construye y envía solo WHOIS
- Función `cmd_wii()` (líneas 803-834): envía WHOIS y WHOWAS secuencialmente
- Ambas validan conexión al servidor y argumentos
- Envío mediante `irc_send()`
- Confirmación visual en ventana de sistema
- `/wii` muestra dos confirmaciones (una por cada comando)

#### `/list` - Sintaxis Completa

```
/list [num <n>] [users <n>|<min>-<max>] [order] [search <patrón>]
```

**Parámetros**:
- `num <n>` - Limitar a n resultados (cuántos canales mostrar)
- `users <n>` - Filtrar por número exacto de usuarios
- `users <min>-<max>` - Filtrar por rango de usuarios
- `order` - Ordenar por número de usuarios (mayor a menor)
- `search <patrón>` - Filtrar por patrón (wildcards * y ?)

**Ejemplos de uso**:
```
/list                                           # Todos los canales
/list users 50                                  # Solo canales con 50 usuarios
/list users 10-40                               # Canales con 10-40 usuarios
/list users 20-100 order                        # Canales con 20-100 usuarios, ordenados
/list num 10 users 10-40 order search *linux*   # 10 canales, 10-40 usuarios, ordenados, con "linux"
```

### Documentación Actualizada

- **README.md**: Actualizado con nueva sintaxis de `/list`, ejemplos de `/whois`/`/wii`, y sección de word wrap
- **QUICKSTART.md**: Comandos esenciales actualizados
- **.ircchat.rc.example**: Documentación completa de comandos con ejemplos
- **CHANGELOG.md**: Esta sección con todos los cambios técnicos

### 🔄 Notas de Actualización

**Para usuarios existentes**:
- No se requiere migración de archivos de configuración
- El archivo `~/.ircchat.rc` sigue funcionando sin cambios
- Los archivos de log existentes se mantienen compatibles
- Todas las características anteriores siguen funcionando igual

**Cambios de comportamiento**:
- `/list` ahora soporta filtrado por usuarios con nueva opción `users`
- El autojoin ahora solo se ejecuta al mensaje 001 real del servidor
- `/wii` ahora envía WHOIS + WHOWAS en lugar de solo WHOIS

**Comandos nuevos**:
- `/whois <nick>` - Información de usuario (solo WHOIS)
- `/wii <nick>` - Información completa (WHOIS + WHOWAS)

### ✅ Compatibilidad

**Probado en**:
- Termux (Android)
- Linux con GCC 11+
- Servidores IRC: Libera.Chat, OFTC

**Requisitos**:
- GCC con soporte C11
- Terminal con soporte ANSI y UTF-8
- Sistema Unix/Linux

**Nota**: No se requiere recompilación de archivos de configuración. Solo ejecuta `make clean && make` para obtener la nueva versión.

---

## Versión Anterior

### Nuevas Funcionalidades

#### Soporte UTF-8 Completo en Entrada

**Caracteres especiales y multibyte**:
- La línea REPL ahora soporta caracteres UTF-8 multibyte
- Puedes escribir ñ, tildes (á, é, í, ó, ú), ç y cualquier carácter Unicode
- Detección automática de secuencias multibyte (2-4 bytes)
- Función `input_add_utf8()` para inserción de caracteres complejos
- Soporte completo para escritura de texto en español, catalán, francés, etc.

#### Word Wrap Automático

**Mensajes largos se dividen en múltiples líneas**:
- Los mensajes que exceden el ancho de pantalla se dividen automáticamente
- Función `wrap_text()` con conteo correcto de caracteres UTF-8
- Funciona en todos los tipos de ventana (sistema, canal, privado)
- Respeta el ancho disponible (considera lista de usuarios en canales)
- Máximo 10 líneas por mensaje individual
- Ya no se truncan mensajes largos

#### Sistema de Notificaciones de Actividad

**Indicadores visuales parpadeantes en el prompt**:
- **\* (rojo)** - Ventana privada nueva sin visitar
  - Aparece cuando recibes el primer mensaje de alguien nuevo
  - Indica que hay ventanas con flag `is_new = true`
- **+ (amarillo)** - Mensajes sin leer en ventanas existentes
  - Aparece cuando hay actividad en ventanas no activas
  - Indica que hay ventanas con flag `has_unread = true`
- Los indicadores usan secuencia ANSI `\033[5m` para efecto de parpadeo
- Se muestran en la esquina derecha de la línea de prompt

**Comportamiento de mensajes privados**:
- Los mensajes privados entrantes NO cambian de ventana automáticamente
- Se crea la ventana si no existe, pero permaneces en tu ventana actual
- La ventana se marca con `is_new = true` si es nueva
- La ventana se marca con `has_unread = true` si ya existía
- Los flags se limpian automáticamente al cambiar a la ventana

**Comando /wl mejorado**:
- Muestra indicador **+** en rojo junto a ventanas privadas con actividad
- Formato: `[n] Privado - nick +` cuando hay mensajes sin leer
- Facilita identificar qué ventanas requieren atención

#### Scroll Horizontal en Lista de Usuarios

**Truncamiento y navegación de nicks largos**:
- Los nicks más largos que el ancho de la lista de usuarios se truncan automáticamente
- **Ctrl+Shift+←** - Scroll horizontal hacia la izquierda (ver inicio del nick)
- **Ctrl+Shift+→** - Scroll horizontal hacia la derecha (ver final del nick)
- Indicadores visuales (←/→) en la cabecera "Usuarios:" muestran disponibilidad de scroll
- Soluciona el problema de nicks largos que se desbordaban en el área de mensajes
- El scroll horizontal se aplica a todos los usuarios visibles simultáneamente
- Se reinicia al cambiar de canal

#### Archivo de Configuración

**Soporte para archivo ~/.ircchat.rc**:
- Define configuraciones persistentes en `~/.ircchat.rc`
- `NICK=tunick` - Establece nickname por defecto
- `SERVER=servidor:puerto` - Define servidor por defecto
- `BUFFER=on/off` - Activa/desactiva buffer automáticamente
- El archivo se carga automáticamente al iniciar
- Con servidor configurado, `/connect` sin argumentos conecta al servidor por defecto

#### Atajos de Teclado Avanzados

**Cambio rápido de ventanas**:
- **Alt+0** a **Alt+9** - Cambia directamente a las ventanas 0-9
- **Alt+→** - Ventana siguiente (navegación cíclica)
- **Alt+←** - Ventana anterior (navegación cíclica)
- **Ctrl+.** - Limpia la pantalla (equivalente a /clear)

Estos atajos permiten navegación mucho más rápida sin necesidad de escribir comandos.

#### Mejoras de Usabilidad

**Cambio automático de ventana al unirse a canal**:
- Cuando usas `/join #canal`, el programa ahora cambia automáticamente a la ventana del canal
- Ya no es necesario usar `/w1` manualmente después de hacer join
- Mejora la fluidez del flujo de trabajo

**Comando `/wc` mejorado**:
- `/wc` sin parámetros ahora cierra la ventana actual
- `/wc <n>` sigue funcionando para cerrar una ventana específica
- Más intuitivo y rápido para cerrar la ventana en la que estás

#### Lista de Usuarios Mejorada

**Ordenamiento inteligente de usuarios**:
- Los usuarios se muestran ordenados por nivel de privilegio
- Orden de privilegios: Owner (~) → Admin (&) → Operator (@) → Half-op (%) → Voice (+) → Normal
- Dentro de cada grupo, los usuarios se ordenan alfabéticamente (sin distinción de mayúsculas/minúsculas)
- El ordenamiento se mantiene automáticamente cuando usuarios cambian de modo
- Facilita encontrar rápidamente a los operadores y usuarios específicos

**Parsing completo de usuarios del servidor**:
- El programa ahora parsea correctamente el mensaje NAMES (353) del servidor IRC
- Todos los usuarios del canal se cargan automáticamente al unirse
- Los usuarios se actualizan dinámicamente cuando entran/salen del canal

**Prefijos de modo**:
Los usuarios se muestran con su nivel de privilegio:
- `~nick` - Owner del canal (rojo)
- `&nick` - Admin del canal (magenta)
- `@nick` - Operador (verde)
- `%nick` - Half-op (azul)
- `+nick` - Voz (cyan)
- `nick` - Usuario normal (gris)

**Scroll en lista de usuarios**:
Cuando hay muchos usuarios en el canal y no caben en pantalla:
- **Ctrl+Shift+↑** - Scroll hacia arriba en la lista
- **Ctrl+Shift+↓** - Scroll hacia abajo en la lista
- Indicadores visuales (↑/↓) muestran si hay más usuarios fuera de vista

### Mejoras Técnicas

#### Algoritmo de Ordenamiento de Usuarios
- Implementado ordenamiento por inserción con función de comparación personalizada
- Función `get_mode_priority()` asigna prioridad numérica a cada nivel de privilegio
- Función `compare_users()` compara por privilegio primero, luego alfabéticamente
- Inserción ordenada en O(n) donde n es el número de usuarios
- Reordenamiento automático cuando un usuario cambia de modo
- Comparación alfabética case-insensitive usando `strcasecmp()`

#### Estructuras de Datos
- Añadido campo `mode` en `UserNode` para almacenar prefijo de privilegio
- Añadido campo `user_scroll_offset` en `Window` para posición del scroll
- Nueva función `window_add_user_with_mode()` para usuarios con prefijo

#### Parsing de IRC
- Implementado parsing completo de mensaje 353 (RPL_NAMREPLY)
- Soporte para múltiples prefijos de modo: @, +, %, ~, &
- Parseo robusto que maneja diferentes formatos de servidor

#### Renderizado
- Lista de usuarios soporta scroll vertical y horizontal con offset
- Truncamiento inteligente de nicks largos que respeta el ancho de columna
- Colores diferenciados según nivel de privilegio
- Indicadores visuales de scroll disponible (↑↓ para vertical, ←→ para horizontal)
- Cálculo dinámico del área visible para cada nick según offset horizontal

### Comandos de Teclado Actualizados

#### Navegación de Mensajes
- **↑** / **↓** - Historial de comandos
- **Ctrl+↑** / **Ctrl+↓** - Scroll por buffer de mensajes
- **Ctrl+B** - Ir al inicio del buffer
- **Ctrl+E** - Ir al final del buffer

#### Navegación de Usuarios (solo en canales)
- **Ctrl+Shift+↑** - Scroll vertical hacia arriba en lista de usuarios
- **Ctrl+Shift+↓** - Scroll vertical hacia abajo en lista de usuarios
- **Ctrl+Shift+←** - Scroll horizontal hacia la izquierda (nicks largos)
- **Ctrl+Shift+→** - Scroll horizontal hacia la derecha (nicks largos)

#### Otras Teclas
- **Ctrl+L** - Redibujar pantalla
- **Ctrl+C** - Salir del programa
- **Enter** - Enviar comando/mensaje

### Correcciones de Bugs

1. **Enter no funcionaba**: Corregido para aceptar tanto `\r` como `\n`
2. **Lectura bloqueante**: Eliminado bucle infinito en `input_read_key()`
3. **Includes faltantes**: Añadido `<unistd.h>` en `main.c` y `irc.c`

### Ejemplo de Uso

```
# Conectar a servidor
/connect irc.libera.chat 6667

# Establecer nick
/nick MiNick

# Unirse a canal
/join #linux

# Cambiar a ventana del canal
/w1

# Verás la lista de usuarios a la derecha con sus prefijos ordenados:
# ~owner          (Owner - máximo privilegio)
# @operator1      (Operadores ordenados alfabéticamente)
# @operator2
# +alice          (Con voz - ordenados alfabéticamente)
# +bob
# charlie         (Usuarios normales - ordenados alfabéticamente)
# david

# Si hay muchos usuarios, usa Ctrl+Shift+↑/↓ para navegar
```

### Estructura Visual del Canal

```
┌────────────────────────┬────────────┐
│ [#canal] (50 usuarios) │     ↑      │
│ <user1> Hola!         │ Usuarios:  │
│ <user2> Qué tal?      │ ~owner     │
│                       │ @operator1 │
│                       │ @operator2 │
│                       │ +voiced    │
│                       │ normaluser │
│                       │     ↓      │
├───────────────────────┴────────────┤
│ > mensaje_                          │
└─────────────────────────────────────┘
```

### Compatibilidad

- Compatible con servidores IRC estándar
- Probado con formato de mensajes RFC 1459
- Soporta variaciones comunes de prefijos de modo

### Próximas Mejoras Sugeridas

- [ ] Ordenar usuarios por nivel de privilegio
- [ ] Búsqueda de usuarios en canales grandes
- [ ] Autocompletado de nicks con Tab
- [ ] Notificaciones cuando te mencionan
- [ ] Colores personalizables para prefijos
