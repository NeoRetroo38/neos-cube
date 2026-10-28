# Casos simulados (landing de choisys)

`examples/simulate_cases.cpp` genera las poblaciones que muestra la sección de choisys en la landing.
Es una **simulación**: el comportamiento es sintético y no describe a personas reales. Lo que sí es real es
la librería: cada recorrido pasa por `scenarys::Run`, y los recuentos, el embudo y la similitud
(`scenarys::compare`, ver `COMPARISON.md`) se calculan aquí, en C++. La landing solo dibuja el JSON.

```sh
make cases            # build/cases.json; lo genera dos veces y comprueba que es idéntico
```

Semilla por defecto `20261028`. `std::mt19937` está definido por el estándar; las distribuciones de
`<random>` no, así que el sorteo usa una función propia (`pick`) y el resultado es el mismo byte a byte en
Linux, macOS y Windows.

## Caso 1 · Selección de producto (5 000 personas)

Tres fases de 3 × 3 que terminan en un producto concreto:

| Fase | Filas | Columnas |
|---|---|---|
| 1 | presupuesto: bajo · medio · alto | uso: casa · transporte · deporte |
| 2 | prioridad: sonido · batería · comodidad | formato: in-ear · diadema · abierto |
| 3 | gama: básica · media · premium | tipo: in-ear · diadema · deportivo → 9 productos |

Cuatro segmentos con preferencias declaradas (pesos por fila y columna en las fases 1 y 2) y una
probabilidad de abandonar en cada fase:

| Segmento | Peso | Abandono por fase |
|---|---|---|
| estudiante | 34 % | 7 % |
| deportista | 22 % | 5 % |
| viajero | 24 % | 5 % |
| audiófilo | 20 % | 4 % |

En la fase 3, la gama se inclina hacia la fila de presupuesto elegida (peso 1 · 0,30 · 0,09) y el tipo
hacia el formato (1 · 0,25 · 0,0625); si el uso es «deporte», el 60 % va directo a «deportivo».

Salida: `funnel` (empiezan, tras fase 1, tras fase 2, completan), `counts` (elecciones por celda y fase),
`after[f1 * 9 + f2]` (cuántos eligieron cada producto tras cada combinación de las fases 1 y 2) y `sample`
(60 recorridos completos para dibujar).

## Caso 2 · Selección de personal (400 invitaciones)

La referencia es el recorrido grabado para el puesto: fase 1 (3,1), fase 2 (2,2), fase 3 (1,3), en base 1.
Abre el enlace el 72 % y abandona un 6 % en cada fase. Los candidatos salen de cuatro arquetipos, cada uno
centrado en sus celdas con un peso de vecindad (más alto = más disperso):

| Arquetipo | Peso | Centro (fila, columna) por fase | Vecindad |
|---|---|---|---|
| afín | 25 % | igual que la referencia | 0,60 |
| próximo | 30 % | (2,1) (2,3) (1,2) | 0,55 |
| distinto | 30 % | (1,3) (1,1) (3,1) | 0,40 |
| disperso | 15 % | centro en todas | 0,95 |

Salida: `funnel` (invitados, abren, completan), `histogram` (similitud en tramos de 10 puntos),
`phaseMean` (similitud media por fase), y `ranking` (los 12 primeros, desempate por orden de invitación).

La similitud mide proximidad dentro de la cuadrícula de este cubo. No mide a la persona ni sustituye la
decisión humana: el arquetipo de origen es parte del modelo, no del candidato.
