# Inicio Rápido - Cliente IRC

> **Versión 1.1.0** - Ahora con word wrap que preserva colores, comando `/wii` (WHOIS+WHOWAS), y filtrado avanzado por usuarios en `/list`

## Compilar

```bash
make
```

## Ejecutar

```bash
./bin/ircchat
```

## Primeros Pasos

### 1. Establecer tu nickname

```
/nick TuNick
```

### 2. Conectar a un servidor IRC

```
/connect irc.libera.chat 6667
```

Servidores IRC públicos populares:
- **irc.libera.chat** (puerto 6667) - Red de software libre
- **irc.freenode.net** (puerto 6667) - Red general
- **irc.oftc.net** (puerto 6667) - Red de open source

### 3. Ver canales disponibles (opcional)

Puedes listar los canales del servidor antes de unirte:

```
/list                                         # Todos los canales
/list order                                   # Ordenar por número de usuarios
/list *linux*                                 # Buscar canales con "linux" en el nombre
/list num 10 order                            # Top 10 canales más poblados
/list users 10-40                             # Solo canales con 10-40 usuarios
/list num 10 users 10-40 order *linux*        # Top 10 canales de 10-40 usuarios con "linux", ordenados
```

### 4. Unirse a un canal

```
/join #linux
```

El programa cambiará automáticamente a la ventana del canal.

Canales populares en Libera.Chat:
- `#linux` - Soporte de Linux
- `#python` - Programación Python
- `#git` - Control de versiones Git
- `#c` - Programación C

### 5. Chatear

Simplemente escribe tu mensaje y presiona Enter. No uses `/` al principio si solo quieres enviar un mensaje.

```
Hola a todos!
```

### 6. Ver ventanas abiertas

```
/wl
```

### 7. Cambiar entre ventanas

```
/w0    # Ventana de sistema
/w1    # Primera ventana (normalmente primer canal)
/w2    # Segunda ventana
```

### 8. Enviar mensaje privado

```
/msg NickDelAmigo Hola, cómo estás?
```

### 9. Cerrar ventana actual

```
/wc
```

También puedes cerrar una ventana específica:
```
/wc 2    # Cierra la ventana 2
```

### 10. Salir

```
/exit
```

## Atajos de Teclado Útiles

- **↑** / **↓** - Navegar historial de comandos
- **Ctrl+↑** / **Ctrl+↓** - Scroll por mensajes antiguos
- **Ctrl+B** - Ir al principio del buffer
- **Ctrl+E** - Ir al final del buffer
- **Ctrl+C** - Salir del programa
- **Ctrl+L** - Redibujar pantalla

## Comandos Esenciales

```
/help                                           # Ver todos los comandos
/list [num <n>] [users <n>|<min>-<max>] [order] [search <patrón>]
                                                # Listar canales del servidor
/whois <nick>                                   # Info usuario (solo WHOIS)
/wii <nick>                                     # Info completa (WHOIS + WHOWAS)
/wl                                             # Listar ventanas
/w<n>                                           # Cambiar a ventana n
/wc [n]                                         # Cerrar ventana n (sin número cierra la actual)
/clear                                          # Limpiar pantalla
/buffer off                                     # Desactivar buffer (ahorrar memoria)
/buffer on                                      # Activar buffer
```

## Ejemplo de Sesión Completa

```
# Iniciar programa
./bin/ircchat

# Configurar nick
/nick MiNick

# Conectar
/connect irc.libera.chat 6667

# Esperar a que se conecte...
# Ver canales disponibles (opcional)
/list num 20 order

# Unirse a canal (cambia automáticamente a la ventana)
/join #python

# Chatear
Hola! Alguien sabe cómo usar decoradores?

# Ver quién está en el canal (mira la columna derecha)

# Ver información de un usuario
/whois AmigoPython
# Muestra: hostname, canales, tiempo idle, etc.

# Ver información completa (incluye historial)
/wii AmigoPython
# Ejecuta WHOIS y WHOWAS

# Enviar mensaje privado a alguien
/msg AmigoPython Gracias por la ayuda!

# Ver ventanas
/wl

# Cambiar a chat privado
/w2

# Volver a sistema
/w0

# Salir
/exit
```

## Solución Rápida de Problemas

### No se conecta al servidor
- Verifica tu conexión a Internet
- Prueba con otro servidor
- Algunos servidores requieren SSL (no soportado aún)

### Los caracteres se ven mal
```bash
# Configurar UTF-8
export LANG=es_ES.UTF-8
./bin/ircchat
```

### El terminal quedó en modo raw
```bash
reset
```

### Quiero debug
```bash
make debug
./bin/ircchat
```

## Notas

- Los mensajes en canales aparecen en el área izquierda
- Los usuarios del canal aparecen en el área derecha
- La ventana 0 (sistema) no se puede cerrar con `/wc 0`, usa `/exit`
- Los comandos que empiezan con `/` y no son reconocidos se envían al servidor
- Puedes usar comandos IRC estándar como `/whois`, `/mode`, `/topic`, etc.

## Ayuda

Para ver la ayuda completa:
```
/help
```

Para documentación detallada, ver `doc/README.md`
