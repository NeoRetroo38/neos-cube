# Motor en el PC y consola en el Mac

La demo remota no convierte neos-cube en un servicio de red. El motor se ejecuta
en el PC, escucha únicamente en loopback y solo la API local de choisys puede
llamarlo.

```text
Mac ── Tailscale ──► choisys web/API en el PC
                              │
                              └──► 127.0.0.1:8765 neos-cube
```

## Límite de seguridad

- `8765` no se publica en la LAN, Tailscale, Funnel ni el router.
- `CHOISYS_LOCAL_API_TOKEN` no sale del PC y no se copia al Mac.
- El Mac no llama directamente al motor.
- No se registran cuerpos, selecciones, tokens ni estado interno.
- Una caída remota se diagnostica desde la API; la comprobación autenticada del
  motor se realiza localmente en el PC.

## Arranque en el PC

El proceso de choisys es responsable de proporcionar el token al motor y a la
API. Usa el lanzador documentado en `choisys/docs/DEMO-TAILSCALE.md`; no inicies
el motor con una dirección distinta de loopback para facilitar la demo.

Comprobaciones locales esperadas:

1. El proceso escucha en `127.0.0.1:8765`, no en todas las interfaces.
2. `/health` sin token es rechazado.
3. `/health` con el token local responde, sin imprimir el token.
4. La API de choisys responde en su dirección Tailscale autorizada.

## Recuperación

Si el Mac alcanza la API pero la evaluación falla:

1. revisa en el PC si el proceso del motor sigue vivo;
2. consulta registros mínimos, sin cuerpos ni credenciales;
3. reinicia el conjunto con el lanzador de choisys;
4. vuelve a probar a través de la API, no abriendo `8765` remotamente.

Coordinación: `NeoRetroo38/scenarys#1`, `NeoRetroo38/choisys#43` y
`#15`.
