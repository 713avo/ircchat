# Guía para Subir IRCCHAT a GitHub

Esta guía te llevará paso a paso para publicar el proyecto en GitHub.

## Paso 1: Preparar el repositorio local

Ejecuta el script de inicialización:

```bash
cd ~/woark/src/IRCCHAT
chmod +x init-git.sh
./init-git.sh
```

Este script:
- Limpia archivos compilados
- Inicializa el repositorio git
- Configura tu usuario git (si es necesario)
- Añade todos los archivos
- Crea el commit inicial

## Paso 2: Crear repositorio en GitHub

1. Ve a [github.com/new](https://github.com/new)
2. Configura el repositorio:
   - **Nombre**: `ircchat` (o el que prefieras)
   - **Descripción**: `Cliente IRC completo en C con interfaz de terminal`
   - **Visibilidad**: Público (recomendado) o Privado
   - **⚠️ IMPORTANTE**: NO marques ninguna de estas opciones:
     - ❌ Add a README file
     - ❌ Add .gitignore
     - ❌ Choose a license

     (Ya tenemos estos archivos localmente)

3. Haz clic en **"Create repository"**

## Paso 3: Conectar y subir el código

Copia los comandos que GitHub te muestra (algo como esto):

```bash
cd ~/woark/src/IRCCHAT

# Añadir el repositorio remoto
git remote add origin https://github.com/TU-USUARIO/ircchat.git

# Renombrar rama principal a 'main'
git branch -M main

# Subir el código
git push -u origin main
```

**Nota**: Reemplaza `TU-USUARIO` con tu nombre de usuario de GitHub.

## Paso 4: Configurar el repositorio en GitHub

Una vez subido el código, configura tu repositorio:

### Añadir Topics

En la página principal del repositorio, haz clic en el ⚙️ engranaje junto a "About" y añade:

```
irc, irc-client, terminal, c, chat, cli, unix, linux, termux, ansi
```

### Descripción y Website

En la misma sección "About":
- **Description**: `Cliente IRC completo en C con interfaz de terminal - Soporte para múltiples ventanas, colores mIRC, logging, notificaciones y más`
- **Website**: (opcional) Si tienes documentación adicional

### Crear sección About

El archivo README.md se mostrará automáticamente como página principal.

## Paso 5: Configurar GitHub (Opcional)

### Proteger rama main

Settings → Branches → Add branch protection rule:
- Branch name pattern: `main`
- ☑️ Require pull request before merging
- ☑️ Require status checks to pass

### Issues y Discussions

Settings → General:
- ☑️ Issues (para reportar bugs)
- ☑️ Discussions (opcional, para comunidad)

### GitHub Pages (Opcional)

Si quieres una página web del proyecto:
- Settings → Pages
- Source: Deploy from a branch
- Branch: main / docs

## Paso 6: Compartir el proyecto

Tu proyecto ahora está en:
```
https://github.com/TU-USUARIO/ircchat
```

### Clonar el repositorio

Otros usuarios pueden clonarlo con:

```bash
git clone https://github.com/TU-USUARIO/ircchat.git
cd ircchat
make
./bin/ircchat
```

### Crear Releases

Cuando tengas una versión estable:

1. Ve a la pestaña "Releases"
2. Clic en "Create a new release"
3. Tag version: `v1.0.0`
4. Release title: `v1.0.0 - Primera versión estable`
5. Describe los cambios
6. Clic en "Publish release"

## Commits futuros

Para actualizar el repositorio:

```bash
# Hacer cambios en el código
vim src/main.c

# Compilar y probar
make clean && make
./bin/ircchat

# Añadir cambios
git add .

# Crear commit
git commit -m "feat: Añadir soporte para SSL/TLS"

# Subir cambios
git push
```

## Solución de problemas

### Error: "Authentication failed"

Usa un Personal Access Token en lugar de contraseña:

1. GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Generate new token (classic)
3. Selecciona scopes: `repo`
4. Usa el token como contraseña al hacer push

O configura SSH:

```bash
# Generar clave SSH
ssh-keygen -t ed25519 -C "tu@email.com"

# Añadir a GitHub
cat ~/.ssh/id_ed25519.pub
# Copia el contenido y pégalo en GitHub → Settings → SSH keys

# Cambiar remote a SSH
git remote set-url origin git@github.com:TU-USUARIO/ircchat.git
```

### Error: "Updates were rejected"

Alguien hizo cambios en GitHub que no tienes localmente:

```bash
# Descargar y fusionar cambios
git pull origin main

# Resolver conflictos si los hay
# Luego hacer push
git push
```

### Ver el estado del repositorio

```bash
# Estado de archivos
git status

# Historial de commits
git log --oneline

# Diferencias con el repositorio remoto
git diff origin/main
```

## Recursos adicionales

- [Documentación de Git](https://git-scm.com/doc)
- [GitHub Docs](https://docs.github.com)
- [Markdown Guide](https://www.markdownguide.org/)
- [Semantic Versioning](https://semver.org/)

---

¡Tu proyecto está listo para el mundo! 🚀
