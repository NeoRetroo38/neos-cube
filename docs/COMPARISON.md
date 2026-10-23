# Comparación de Runs (cubos de referencia)

Estado: implementada en `src/core/cube.hpp` (`scenarys::compare`) y expuesta en `POST /compare`.
Pedida por el dueño el 23 oct 2026 para los cubos de referencia de choisys. No es inferencia: no
interpreta a nadie; mide la proximidad de dos recorridos dentro de la forma que define ese cubo.

## Espacio

Un cubo tiene `n` fases (1–10). La fase `p` es una cuadrícula de `R_p` filas × `C_p` columnas (1–10),
de celdas unitarias. Un Run completo es una selección por fase: `(p, r_p, c_p)`, base 1. Es exactamente
el DTO público `measurements` que ya devuelve `/evaluate`.

## Fórmula

Para una referencia `A` y un candidato `B` del mismo cubo y la misma forma:

```text
D_p   = √((R_p − 1)² + (C_p − 1)²)                    diagonal de la fase p
d_p   = √((rA_p − rB_p)² + (cA_p − cB_p)²) / D_p       si D_p > 0
d_p   = 0                                              si la fase es 1×1
s_p   = 100 · (1 − d_p)                                similitud de la fase p
S     = 100 · (1 − (1/n) · Σ d_p)                      similitud del Run
```

- `d_p ∈ [0, 1]`: 0 es la misma celda; 1, esquinas opuestas de la cuadrícula.
- `S = 100` solo si todas las selecciones coinciden; `S = 0` solo si en todas las fases están en
  esquinas opuestas.
- **Pesos:** todas las fases pesan `1/n`. El modelo de cubo actual (fases con etiquetas de filas y
  columnas) no define pesos; si algún día se definen, entrarán aquí como `w_p` normalizados.
- Simétrica (`S(A,B) = S(B,A)`), determinista y sin estado: no usa reloj, red ni azar.
- Redondeo: a la décima más cercana (`std::lround`, mitades lejos de cero) sobre el valor sin redondear.
  `S` se calcula con las `d_p` sin redondear; no es la media de las `s_p` redondeadas.

## Ejemplos (cubo 3 × 3 × 3)

| Referencia | Candidato | d por fase | S |
|---|---|---|---|
| (1,1) (1,1) (1,1) | igual | 0, 0, 0 | 100.0 |
| (1,1) (1,1) (1,1) | (3,3) (3,3) (3,3) | 1, 1, 1 | 0.0 |
| (1,1) (1,1) (1,1) | (2,2) (2,2) (2,2) | 0.5, 0.5, 0.5 | 50.0 |
| (1,1) (1,1) (1,1) | (1,2) (1,1) (1,1) | 0.3536, 0, 0 | 88.2 |

Forma mixta `[[3,3],[1,1],[1,5]]`: `(1,1)(1,1)(1,1)` frente a `(3,3)(1,1)(1,3)` → fases 0.0, 100.0, 50.0; S = 50.0.

## Contrato

```text
POST /compare   (Bearer, solo 127.0.0.1)
{"shape":[[R,C],…],"reference":[{"phase":1,"row":r,"column":c},…],"candidate":[…]}
→ 200 {"ok":true,"result":{"similarity":88.2,"phases":[{"phase":1,"similarity":64.6},…]}}
→ 400 INVALID_REQUEST   forma inválida, run incompleto, fase desordenada o celda fuera de rango
```

## Qué no afirma

La similitud mide proximidad geométrica dentro del cubo configurado. No mide personalidad, capacidad,
aptitud ni compatibilidad psicológica, y no sustituye una decisión humana.
