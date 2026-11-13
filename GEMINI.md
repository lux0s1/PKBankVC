# Reglas de Proyecto - PKBankVC

1. **No reescribir archivos completos:**
   - Usar siempre ediciones parciales (`edit_file`).
   - Evitar reescrituras completas (`write_file` con `Overwrite: true`) a menos que el archivo sea totalmente nuevo o cambie más del 80% de su contenido.

2. **Validar antes de declarar hecho:**
   - Compilar con el toolchain de 3DS / devkitARM (`make -C "E:\IA workspace\PKBankVC"`), verificar salida limpia y ausencia de errores antes de declarar una tarea como finalizada.

3. **Cero charla aduladora:**
   - Ir directo al grano y al trabajo técnico.
   - Eliminar cortesías redundantes, introducciones de relleno o elogios.

4. **Paralelizar tool calls:**
   - Agrupar llamadas a herramientas independientes en una sola ronda siempre que sea posible.

5. **No duplicar código en la respuesta:**
   - No volcar código fuente extenso ni diffs redundantes en el chat.
   - Proveer enlaces de archivo concisos (`file://`).

6. **No usar Agent cuando Grep/Read basta:**
   - Realizar búsquedas y lecturas directas en el árbol de directorios sin subagentes innecesarios.

# Política de Enrutamiento de Modelos

- **Tareas triviales y repetitivas** (formateo, generación de pruebas unitarias simples, documentación, migración de tipos o búsqueda en archivos): Delega siempre a un subagente configurado con `gemini-3.5-flash-lite` o `gemini-3.8-flash` (`Model: "flash_lite"` o `"flash"`).
- **Refactorización multi-archivo y debugging complejo**: Usa el modelo principal (`gemini-4-argon` o `gemini-3.1-pro-preview`) únicamente para diagnosticar la causa raíz y generar el plan. Una vez aprobado el plan, cambia la ejecución a `gemini-3.8-flash`.
- **Consultas de terminal / comandos Git**: Ejecuta en modo **Fast** sin activar el modo *Planning*.
