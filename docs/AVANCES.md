# Avances del motor (8 oct 2026)

Resumen para el equipo. La presentación completa del backend está en `choisys/docs/avances/` (HTML, PDF y PowerPoint/Keynote).

## Hecho

- **Núcleo `Run`/`Session`** (`src/core/cube.hpp`): tres fases, cuadrícula 3×3 por fase, intentos y entradas vacías, inválidas o fuera, salidas anticipadas.
- **Servicio local** (`src/api/server.cpp`): solo `127.0.0.1:8765`, token Bearer, peticiones y plazos acotados, logs sin datos. Portable: Winsock en Windows, sockets POSIX en macOS/Linux.
- **Pruebas:** `tests/core-tests.cpp` (núcleo) y humo extremo a extremo (`smoke.ps1`, `smoke-posix.sh`).
- **Ejemplo:** `examples/simulate_runs.cpp` genera recorridos **sintéticos** con la librería real (semilla fija, salida idéntica). Alimenta el informe ejecutivo de choisys.
- **Amenazas:** `docs/THREAT_MODEL.md`.

## Cómo lo usa el producto

La API de choisys envía las decisiones de cada fase y recibe la confirmación; al completar la fase 3 recibe `measurements` (fase, fila, columna) y los guarda en el historial de la persona. TypeScript no calcula nada del Cubo.

## Pendiente

- Primera compilación y ejecución reales en macOS (`make test smoke`).
- Inferencia: no existe y no se escribe sin una especificación matemática aprobada (`docs/INFERENCE_ARCHITECTURE.md`).
