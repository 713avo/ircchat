# Contribuir a IRCCHAT

¡Gracias por tu interés en contribuir! Este documento te guiará en el proceso.

## Cómo contribuir

### Reportar bugs

Si encuentras un bug, por favor abre un issue en GitHub con:

- Descripción clara del problema
- Pasos para reproducirlo
- Comportamiento esperado vs observado
- Versión del sistema operativo y terminal
- Output de `gcc --version`

### Sugerir mejoras

Las sugerencias son bienvenidas. Abre un issue con:

- Descripción detallada de la funcionalidad
- Caso de uso
- Ejemplos si es posible

### Pull Requests

1. **Fork** el repositorio
2. Crea una rama para tu feature: `git checkout -b feature/nueva-funcionalidad`
3. Haz commits con mensajes descriptivos
4. Asegúrate de que el código compila sin warnings: `make clean && make`
5. Prueba tu código extensivamente
6. Haz push a tu fork: `git push origin feature/nueva-funcionalidad`
7. Abre un Pull Request

### Estilo de código

- **Estándar**: C11
- **Indentación**: 4 espacios (no tabs)
- **Nombres**: snake_case para funciones y variables
- **Comentarios**: En español, claros y concisos
- **Límite de línea**: 100 caracteres cuando sea razonable

### Estructura de commits

Formato recomendado:

```
Tipo: Descripción breve (máx 72 caracteres)

Descripción detallada si es necesaria.
Puede ocupar múltiples líneas.

- Punto específico
- Otro punto

Relacionado con #123
```

**Tipos de commit:**
- `feat:` Nueva funcionalidad
- `fix:` Corrección de bug
- `docs:` Cambios en documentación
- `style:` Formato, punto y coma faltante, etc.
- `refactor:` Refactorización de código
- `perf:` Mejora de rendimiento
- `test:` Añadir tests
- `chore:` Cambios en build, herramientas, etc.

### Testing

Antes de enviar tu PR, prueba:

- Compilación limpia: `make clean && make`
- Conexión a servidor IRC real
- Comandos básicos: `/join`, `/part`, `/msg`, `/list`
- Notificaciones y alertas
- Scroll en buffers y lista de usuarios
- Logging y timestamps
- Diferentes tamaños de terminal

### Áreas que necesitan ayuda

- [ ] Soporte SSL/TLS
- [ ] Tests automatizados
- [ ] Mejoras en rendimiento
- [ ] Documentación y ejemplos
- [ ] Compatibilidad con más terminales
- [ ] Soporte para más plataformas

## Preguntas

Si tienes preguntas, abre un issue con la etiqueta `question`.

## Código de Conducta

- Sé respetuoso y constructivo
- Acepta críticas constructivas
- Enfócate en lo mejor para el proyecto
- Ayuda a otros contribuidores

---

¡Gracias por contribuir a IRCCHAT! 🚀
