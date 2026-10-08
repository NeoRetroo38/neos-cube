# Nota operativa para Daemon, Neo y Claude

Estado validado en el PC `desktop-dgjsrgv` el 8 de octubre de 2026.

## Límite del motor en la demo Safari

Safari nunca accede directamente a este servicio. El recorrido aprobado es:

```text
Safari -> scenarys -> choisys web -> choisys API -> 127.0.0.1:8765 neo-cube
                                      |
                                      -> Neon PostgreSQL
```

El proceso `neo-cube-service` debe escuchar solo en IPv4
`127.0.0.1:8765`. No publicar 8765 mediante Tailscale Serve, LAN, Funnel,
firewall o router. El token local permanece en el PC y solo lo comparten el
launcher del motor y la API de choisys.

## Responsables

- **Daemon:** comprobar listener, health autenticado y logs mínimos en el PC.
- **Neo:** probar desde Safari a través de choisys; no necesita credenciales ni
  acceso directo a este repositorio.
- **Claude:** preservar loopback, autenticación y DTO público al modificar el
  servicio. Probar `make test smoke` en macOS cuando haya un Mac disponible.

Un error de registro de usuario pertenece a la API/Neon de choisys, no al motor.
Un fallo del Cubo aparece después, durante la evaluación de las tres fases.
