# Makefile para Cliente IRC en C

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -O2
LDFLAGS =
INCLUDES = -Isrc

# Directorios
SRCDIR = src
BINDIR = bin
DOCDIR = doc

# Archivos fuente y objetos
SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/terminal.c \
          $(SRCDIR)/windows.c \
          $(SRCDIR)/buffer.c \
          $(SRCDIR)/irc.c \
          $(SRCDIR)/commands.c \
          $(SRCDIR)/input.c \
          $(SRCDIR)/config.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BINDIR)/%.o)

# Ejecutable
TARGET = $(BINDIR)/ircchat

# Regla por defecto
all: $(TARGET)

# Crear ejecutable
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Compilación completada: $(TARGET)"

# Compilar archivos objeto
$(BINDIR)/%.o: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Crear directorios necesarios
$(BINDIR):
	mkdir -p $(BINDIR)

# Limpiar archivos compilados
clean:
	rm -rf $(BINDIR)/*.o $(TARGET)
	@echo "Limpieza completada"

# Limpiar todo incluyendo binarios
distclean: clean
	rm -rf $(BINDIR)
	@echo "Limpieza profunda completada"

# Ejecutar el programa
run: $(TARGET)
	$(TARGET)

# Compilar con símbolos de depuración
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Compilación en modo debug completada"

# Instalar (opcional)
install: $(TARGET)
	@echo "Instalando en /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/ircchat
	@echo "Instalación completada"

# Desinstalar
uninstall:
	@echo "Desinstalando..."
	@sudo rm -f /usr/local/bin/ircchat
	@echo "Desinstalación completada"

# Ayuda
help:
	@echo "Makefile para Cliente IRC"
	@echo ""
	@echo "Objetivos disponibles:"
	@echo "  all        - Compilar el proyecto (por defecto)"
	@echo "  clean      - Eliminar archivos objeto y ejecutable"
	@echo "  distclean  - Limpieza profunda (eliminar bin/)"
	@echo "  run        - Compilar y ejecutar el programa"
	@echo "  debug      - Compilar en modo debug"
	@echo "  install    - Instalar en /usr/local/bin (requiere sudo)"
	@echo "  uninstall  - Desinstalar del sistema"
	@echo "  help       - Mostrar esta ayuda"

# Declarar objetivos que no son archivos
.PHONY: all clean distclean run debug install uninstall help
