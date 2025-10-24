#!/bin/bash

# Script para inicializar el repositorio Git y preparar para GitHub

echo "=================================================="
echo "Inicializando repositorio Git para IRCCHAT"
echo "=================================================="
echo ""

# Ir al directorio del proyecto
cd "$(dirname "$0")" || exit 1

# Verificar si git está instalado
if ! command -v git &> /dev/null; then
    echo "❌ Error: git no está instalado"
    echo "Instala git con: pkg install git"
    exit 1
fi

# Limpiar archivos compilados antes de inicializar
echo "🧹 Limpiando archivos compilados..."
make clean 2>/dev/null || true

# Inicializar repositorio git si no existe
if [ ! -d .git ]; then
    echo "📦 Inicializando repositorio git..."
    git init
    echo "✅ Repositorio inicializado"
else
    echo "ℹ️  Repositorio git ya existe"
fi

# Configurar usuario git si no está configurado
if [ -z "$(git config user.name)" ]; then
    echo ""
    echo "⚙️  Configuración de Git"
    read -p "Introduce tu nombre: " git_name
    read -p "Introduce tu email: " git_email
    git config user.name "$git_name"
    git config user.email "$git_email"
    echo "✅ Usuario configurado"
fi

# Añadir todos los archivos
echo ""
echo "📝 Añadiendo archivos al staging area..."
git add .

# Mostrar estado
echo ""
echo "📊 Estado del repositorio:"
git status

# Crear primer commit
echo ""
read -p "¿Crear commit inicial? (s/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Ss]$ ]]; then
    git commit -m "Initial commit: Cliente IRC en C

- Interfaz de terminal con múltiples ventanas
- Soporte para canales y mensajes privados
- Sistema de notificaciones múltiples (C M * +)
- Logging automático con timestamps
- Autocompletado de nicks con TAB
- Colores mIRC y UTF-8
- Modo silencioso configurable
- Comandos /list con filtros y ordenamiento
- Procesamiento completo de eventos IRC (JOIN, PART, QUIT)
- Buffer con scroll y word wrap inteligente

🤖 Generated with Claude Code"
    echo "✅ Commit inicial creado"
fi

echo ""
echo "=================================================="
echo "✅ Repositorio preparado para GitHub"
echo "=================================================="
echo ""
echo "📋 Próximos pasos:"
echo ""
echo "1. Crea un nuevo repositorio en GitHub:"
echo "   https://github.com/new"
echo ""
echo "2. NO marques 'Initialize with README' (ya tenemos uno)"
echo ""
echo "3. Conecta tu repositorio local con GitHub:"
echo "   git remote add origin https://github.com/TU-USUARIO/ircchat.git"
echo ""
echo "4. Sube el código:"
echo "   git branch -M main"
echo "   git push -u origin main"
echo ""
echo "5. (Opcional) Añade topics en GitHub:"
echo "   irc, irc-client, terminal, c, chat, unix, linux"
echo ""
