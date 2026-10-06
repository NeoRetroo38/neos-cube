# Arquitectura futura de Inference

## Estado

La inferencia todavía no está implementada. La documentación disponible fija dos
niveles, `SessionInference` para una fase y `RunInference` para el conjunto de tres
fases, pero no define el cálculo. No se añadirá una fórmula supuesta.

## Ubicación prevista

```text
src/core/
  cube.hpp
  inference.hpp      interfaz privada del core
  inference.cpp      implementación propietaria
tests/
  core-tests.cpp
  inference-tests.cpp
```

Estos archivos permanecerán exclusivamente en el backend privado local. No se
copiarán a `packages/core`, `packages/shared`, `apps/api` ni `apps/mobile`.

## Separación de responsabilidades

- `Session` conserva observaciones de una fase.
- `Run` conserva las tres sesiones, el recorrido y el estado agregado.
- `SessionInference` será el resultado privado calculado desde una `Session`.
- `RunInference` será el resultado privado calculado desde un `Run` y, si la
  especificación lo exige, desde los tres resultados de sesión.
- CLI y GUI solo consumirán funciones del core; no calcularán inferencia.
- El servicio C++ transformará el resultado privado a un DTO público previamente
  aprobado. No serializará la estructura privada completa.

## Invariantes

- Dominio no negativo.
- Ausencia, fase no visitada y selección existente son estados distintos.
- No usar valores negativos como centinelas.
- Resultado determinista para la misma entrada y versión de escenario.
- Sin dependencias de red, reloj, base de datos o UI dentro del cálculo.
- Ninguna fórmula, matriz, peso, estado intermedio o traza inferencial en logs.
- Ninguna implementación paralela en TypeScript o JavaScript.

## Secuencia de implementación

1. Aprobar la especificación matemática y versionarla de forma privada.
2. Definir entradas, salidas, rangos y estados ausentes.
3. Escribir casos deterministas con resultados esperados.
4. Implementar `SessionInference` y ejecutar sus tests.
5. Implementar `RunInference` y ejecutar sus tests.
6. Integrar CLI y GUI como consumidores del core.
7. Definir qué parte mínima puede abandonar el servicio C++.
8. Añadir el mapeo seguro al servicio y repetir tests HTTP y E2E.

## Especificación necesaria antes de programar

- Campos exactos de `SessionInference` y `RunInference`.
- Fórmula o algoritmo canónico de cada nivel.
- Regla de combinación de las tres fases.
- Tratamiento de fase no visitada, salida anticipada e input inválido.
- Rango, unidades, precisión y redondeo.
- Versión de escenario o modelo que gobierna el cálculo.
- Al menos dos ejemplos completos por nivel con resultado esperado.
- Lista explícita de campos públicos permitidos, si los hubiera.
