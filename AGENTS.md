# neos-cube — guía para agentes (Codex, Claude y cualquier otro)

Motor privado **Cubo de Neo** del producto choisys (scenarys S.L.). Repositorio **privado**; el código
del motor no debe copiarse a ningún repositorio público, frontend, API pública, nube ni documentación.
El cliente de producto vive en el repo público `choisys`.

## Qué es

- `src/core/cube.hpp`: `Run` (recorrido de 3 fases) y `Session` (observaciones de una fase), cubo 3×3×3.
- `src/api/server.cpp`: servicio HTTP local en `127.0.0.1:8765`, autenticado, secuencial, sin dependencias.
- `tests/`: pruebas del núcleo y smoke del servicio. `experimental/`: banco de pruebas local (consola, GUI Win32).

## Reglas cerradas

1. **No inventes inferencia.** `docs/INFERENCE_ARCHITECTURE.md` manda: sin especificación matemática aprobada
   (campos, algoritmo, regla de combinación, ejemplos) no se escribe ninguna fórmula.
2. **Dominio no negativo.** Ausencia, fase no visitada y selección existente son estados distintos; ningún
   valor negativo como centinela.
3. **Lo que sale del servicio es un DTO público aprobado.** Hoy: confirmación de fase y, al completar el
   Run, `measurements: [{ phase, row, column }]` (base 1). No serialices la estructura privada completa
   ni expongas matrices, pesos, fórmulas, estados intermedios o trazas.
4. **Resultado determinista** para la misma entrada y versión de escenario; sin red, reloj ni base de datos
   dentro del cálculo.
5. **Solo IPv4 loopback**, token Bearer de 32–256 caracteres, peticiones y respuestas acotadas, con deadlines.
6. **Logs mínimos:** timestamp, endpoint permitido, estado HTTP, latencia y código de error. Nunca cuerpos,
   IDs de sesión, selecciones, credenciales ni estados internos.

## Seguridad

- El token (`CHOISYS_LOCAL_API_TOKEN`) vive fuera del repo, en `C:\Users\Admin\daemon.codex.env.local`.
  No lo imprimas ni lo guardes en archivos, argumentos, logs o commits.
- `.gitignore` excluye `build/`, binarios, logs y archivos `.env`. No fuerces la inclusión de ninguno.

## Git (importante)

- **Autoría:** los commits son solo del dueño. **No añadas** `Co-Authored-By`, firmas de IA ni enlaces de
  herramientas en mensajes de commit.
- **Ramas:** trabaja en tu rama (`codex/<tema>` o `claude/<tema>`); `main` cambia solo con aprobación
  explícita. Otro agente puede estar trabajando a la vez: commits pequeños.
- **Nunca** `push --force`, reescribir historia ni `push` a `main` sin permiso.

## Compilar y probar (Windows, MSYS2 g++)

```powershell
.\build.ps1 -Target tests     # compila y ejecuta los tests del núcleo
.\build.ps1 -Target service   # build\neo-cube-service.exe
.\run.ps1                     # arranca el servicio (lee el token del entorno o del archivo externo)
```

Flags: `-std=c++17 -Wall -Wextra -Wpedantic`. `cube.hpp` y `tests/core-tests.cpp` son C++17 estándar y
compilan en cualquier plataforma. **`server.cpp` usa Winsock y `gmtime_s` (solo Windows)**; el port a
POSIX (sockets, `gmtime_r`, `Makefile`/`build.sh`) está pendiente, para macOS y Linux/servidor.

## Definición de terminado

Tests del núcleo en verde (`ALL NEO CUBE TESTS PASSED`), el servicio compila sin warnings, sin cambios de
comportamiento matemático, sin datos privados nuevos en respuestas ni logs. Informa de lo que no pudiste
verificar.
