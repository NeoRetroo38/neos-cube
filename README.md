# Cubo de Neo — scenarys S.L.

Backend C++ público de choisys. Este repositorio se publica en
[`NeoRetroo38/neos-cube`](https://github.com/NeoRetroo38/neos-cube), pero el servicio se ejecuta
de forma local y solo escucha en loopback. La lógica del motor no se duplica en TypeScript,
frontend ni servicios cloud; las credenciales, logs y backups permanecen locales y privados.

## Componentes

- `src/core/cube.hpp`: copia exacta del motor fuente original, sin cambios de comportamiento.
- `src/api/server.cpp`: servicio HTTP independiente de la GUI (Winsock en Windows, sockets POSIX en macOS/Linux); cada sesión HTTP mantiene un `scenarys::Run`.
- `experimental/console.cpp`: consola local que consume el mismo core; no participa en el servicio.
- `experimental/gui.cpp`: GUI experimental que consume el mismo core; no participa en el servicio.
- `tests/core-tests.cpp`: pruebas deterministas del comportamiento `Run`/`Session` disponible.
- `build`: binarios locales. `tests/smoke.ps1`: verificación contra el servicio ya iniciado.

Fuente original preservada: `C:\Users\Admin\Documents\Codex\2026-10-04\h\outputs\neo-cube`.
Backup anterior a cambios: `C:\Users\Admin\Documents\Scenarys\backups\neo-cube\20261005-014449`.

La fuente disponible conserva selecciones y progresión de tres fases. No implementa probabilidades, inferencias o puntuaciones adicionales. La API solo comunica aceptación/progresión y finalización, nunca registros internos.

## Build y ejecución

MSYS2 g++ 16.2.0 en `C:\msys64\mingw64\bin\g++.exe`; C++17. Sin dependencias nuevas ni CMake/Ninja.

```powershell
& 'C:\Users\Admin\Documents\Scenarys\backend\neo-cube\build.ps1' -Target service
& 'C:\Users\Admin\Documents\Scenarys\backend\neo-cube\run.ps1'
```

Compilación efectiva del servicio:

```powershell
$env:PATH = 'C:\msys64\mingw64\bin;' + $env:PATH
& 'C:\msys64\mingw64\bin\g++.exe' -std=c++17 -Wall -Wextra -Wpedantic -static 'C:\Users\Admin\Documents\Scenarys\backend\neo-cube\src\api\server.cpp' -o 'C:\Users\Admin\Documents\Scenarys\backend\neo-cube\build\neo-cube-service.exe' -lws2_32
```

Otros targets: `console`, `gui`, `tests` y `all`. GUI añade `-mwindows -lgdiplus`; `tests` compila y ejecuta `neo-cube-tests.exe`. `all` construye servicio, consola, GUI y tests, y falla si las pruebas no pasan.

`run.ps1` lee `CHOISYS_LOCAL_API_TOKEN` del entorno o de `C:\Users\Admin\daemon.codex.env.local`, nunca lo imprime. El archivo local se generó con aleatoriedad criptográfica y ACL para el usuario actual y SYSTEM. No está dentro del repositorio. Se aceptan secretos de 32 a 256 caracteres ASCII imprimibles sin espacios; nunca añadir el valor a argumentos, documentación o bundle móvil. El servicio falla al arrancar si falta una credencial válida o no puede reservar el puerto.

### macOS y Linux

Solo hace falta un compilador C++17 (`c++`, `clang++` o `g++`) y `make`. Sin dependencias ni CMake.

```sh
make test        # compila y ejecuta los tests del núcleo
make service     # build/neo-cube-service
make smoke       # arranca un servicio con token y logs desechables y lo prueba de extremo a extremo (curl + openssl)
./run.sh         # arranca el servicio
```

`run.sh` lee `CHOISYS_LOCAL_API_TOKEN` del entorno o del archivo externo `~/.choisys.env.local` (otro
archivo con `CHOISYS_ENV_FILE`) y nunca lo imprime. El archivo debe crearse en cada equipo con un secreto
aleatorio de al menos 32 caracteres, por ejemplo `printf 'CHOISYS_LOCAL_API_TOKEN=%s\n' "$(openssl rand -hex 32)" > ~/.choisys.env.local && chmod 600 ~/.choisys.env.local`.
Los logs van a `~/.local/state/neo-cube` salvo que se fije `CHOISYS_LOCAL_LOG_DIR`. El comportamiento es el
mismo que en Windows: solo loopback, token Bearer, peticiones acotadas y deadlines.

Estado de verificación del port POSIX: la rama POSIX se comprobó solo con una verificación de sintaxis
(`-Wall -Wextra -Wpedantic`) sobre cabeceras simuladas, y `tests/smoke-posix.sh` se ejecutó contra el binario de
Windows. La primera compilación y ejecución reales en macOS/Linux se hacen con `make test smoke` en el propio
Mac. Hay un workflow de CI (ubuntu y macos) preparado en la rama `claude/ci-posix`; para subirlo, el token de
`gh` necesita el permiso `workflow` (`gh auth refresh -h github.com -s workflow`).

`CHOISYS_LOCAL_LOG_DIR` permite configurar exclusivamente el directorio de logs; por defecto `%USERPROFILE%\Documents\Scenarys\logs\neo-cube`. El launcher prepara directorio/archivo antes de iniciar el proceso. `service.log` registra fecha UTC, endpoint permitido (`/health`, `/evaluate` u `other`), estado HTTP, milisegundos y código seguro. No registra cuerpos, IDs de sesión, selecciones, tokens ni datos del motor. Rotación de logs pendiente para uso prolongado.

## Contrato mínimo

Escucha exclusivamente `127.0.0.1:8765`, puerto fijo y sin opción LAN. Todos los endpoints requieren `Authorization: Bearer <credencial-local>`. `apps/api` conserva la credencial y es el único consumidor de producto. Cabeceras `Origin` se rechazan y no se emiten cabeceras CORS.

- `GET /health`: `{"ok":true,"service":"neo-cube","version":"0.1.0"}`.
- `POST /evaluate`: JSON con exactamente `scenarioId`, `sessionId`, `phase`, `decisions`.
- `scenarioId`: `choice-grid`; `sessionId`: UUID en formato de 36 caracteres; `phase`: 1, 2 o 3.
- `decisions`: entre 1 y 9 objetos con exactamente `position`, `selected`, `value`. Posiciones 1 a 9 sin duplicados. Exactamente uno debe tener `selected: true` y `value: 1`; los demás son opcionales, `false` y `0`.
- Éxito: `{"ok":true,"result":{"sessionId":"<UUID>","phase":1,"status":"phase-complete","nextPhase":2}}`. En la fase final: `status: completed`, `nextPhase: null`.
- Error: `{"ok":false,"error":{"code":"<código>"}}`. Códigos: `INVALID_REQUEST` (400), `UNAUTHORIZED` (401), `SESSION_NOT_FOUND`/`NOT_FOUND` (404), `REQUEST_TIMEOUT` (408), `SESSION_CONFLICT` (409), `SERVICE_BUSY` (503), `INTERNAL_ERROR` (500).

Sesiones solo en memoria, TTL inactivo 30 minutos y máximo 256. Una nueva sesión empieza en fase 1. Repetir una fase con la misma selección devuelve su respuesta original sin avanzar otra vez; alterar una selección aceptada o saltar una fase devuelve conflicto. Los botones false omitidos no cambian la identidad semántica del retry. Reiniciar el proceso pierde las sesiones; se inicia un nuevo UUID. No existe identidad de usuario dentro del motor.

HTTP limita cabeceras/cuerpo a 8192 bytes cada uno, usa una conexión por solicitud y plazo absoluto de lectura de 2 segundos, escritura de 1 segundo. Rechaza longitudes duplicadas, cabeceras repetidas, transfer-encoding, expect y formatos fuera del contrato. El lector JSON es deliberadamente limitado a ASCII y tipos usados por este contrato. La cola TCP es 16 y el procesamiento local es secuencial; no se ha diseñado como servidor de internet ni de alta concurrencia.

## Verificación

Con el servicio iniciado:

```powershell
& 'C:\Users\Admin\Documents\Scenarys\backend\neo-cube\build.ps1' -Target tests
& 'C:\Users\Admin\Documents\Scenarys\backend\neo-cube\tests\smoke.ps1'
```

La inferencia de nivel `Session` y `Run` no está implementada porque la fuente y la documentación disponibles no definen su cálculo. Solo fijan los nombres previstos y el dominio no negativo. No se añadirá una fórmula supuesta.

Comprueba health, autenticación, request inválido, tres fases, idempotencia/conflictos y binding. El token se lee localmente sin imprimirlo.

## Integraciones

Flujo autorizado: mobile → API de producto Express → servicio local C++ → API → mobile. El móvil nunca llama al loopback C++ ni recibe su token. La exposición LAN de la API para Expo requiere configuración explícita y controles del producto; no cambia el binding de este servicio.

Para futuros servicios de scenarys S.L. se deben definir productor, consumidor, protocolo, autenticación, payload permitido, frecuencia, permisos, sensibilidad, persistencia, logging, timeout y fallback antes de implementar. Integración Wibaruim pendiente de especificación.
