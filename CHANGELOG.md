# Changelog - Cliente IRC

## Versión Actual

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
