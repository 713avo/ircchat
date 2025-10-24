# Guía de Debugging

## Si Enter no funciona

### 1. Verificar que el terminal está en UTF-8
```bash
echo $LANG
# Debería mostrar algo como: es_ES.UTF-8 o en_US.UTF-8
```

Si no es UTF-8:
```bash
export LANG=en_US.UTF-8
./bin/ircchat
```

### 2. Probar en diferentes terminales

En Termux, prueba:
```bash
# Terminal por defecto de Termux debería funcionar
./bin/ircchat
```

### 3. Verificar modo raw del terminal

Si el programa se bloquea o no responde:
```bash
# Presiona Ctrl+C para salir
# Luego restaura el terminal:
reset
```

### 4. Compilar en modo debug

```bash
make debug
./bin/ircchat
```

### 5. Probar manualmente la entrada

Crea un archivo de test:
```bash
cat > test_input.txt << 'EOF'
/help
/exit
EOF

./bin/ircchat < test_input.txt
```

### 6. Verificar que los caracteres se están capturando

Añadir temporalmente debug a `input_read_key()`:

En `src/input.c`, después de leer el carácter:
```c
nread = read(STDIN_FILENO, &c, 1);

// DEBUG: descomentar para ver qué se está leyendo
// fprintf(stderr, "DEBUG: leído carácter %d (0x%02x)\r\n", (int)c, (unsigned char)c);

if (nread != 1) return -1;
```

Luego recompilar:
```bash
make clean && make
./bin/ircchat 2> debug.log
```

Y en otra terminal:
```bash
tail -f debug.log
```

### 7. Problemas conocidos

#### Enter no hace nada
- **Causa**: El terminal no está enviando '\r' ni '\n'
- **Solución**: El código ya maneja ambos casos

#### El programa se congela
- **Causa**: Problema con select() o read()
- **Solución**: Presiona Ctrl+C y ejecuta `reset`

#### Los caracteres no se muestran
- **Causa**: Terminal no soporta ANSI
- **Solución**: Usa un terminal moderno

### 8. Teclas de emergencia

- **Ctrl+C**: Salir del programa (siempre funciona)
- **Ctrl+L**: Redibujar la pantalla
- **Ctrl+E**: Ir al final del buffer (si estás navegando)

### 9. Si todo falla

Restaurar el terminal:
```bash
reset
stty sane
```

## Información de Debug

Para reportar un bug, incluye:

1. Terminal usado: `echo $TERM`
2. Sistema operativo: `uname -a`
3. Locale: `locale`
4. Versión de GCC: `gcc --version`
5. Output de: `stty -a`

## Comandos útiles para testing

### Probar caracteres especiales
```bash
# Ver códigos de las teclas
od -c
# Presiona Enter y mira qué muestra (debería ser \r o \n)
# Presiona Ctrl+D para salir
```

### Verificar configuración del terminal
```bash
stty -a
# Busca: icanon, echo, icrnl
```

### Terminal limpio
```bash
stty sane
./bin/ircchat
```
