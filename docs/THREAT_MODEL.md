# Modelo de amenazas de choisys y del Cubo de Neo

Documento **privado** (vive en este repo a propósito: lista debilidades abiertas). Estado a 7 oct. 2026.
Se basa en el código real de `choisys` (público) y de este motor. Todo lo marcado como **hueco** es una
tarea pendiente, no un incidente.

## Qué protegemos, en orden de valor
1. **El Cubo de Neo** (código, fórmulas futuras, especificación): propiedad intelectual de la sociedad.
2. **Los datos de las personas**: decisiones, cuentas, correos.
3. **La disponibilidad** del servicio durante el lanzamiento.
4. **El control de `main`** y de las cuentas con poder (GitHub, SUPERDEV).

## Quién puede atacar (hipótesis, sin datos todavía)
Curioso con la app abierta · bot de fuerza bruta contra el login · competidor que busca el motor ·
atacante de la cadena de suministro (dependencias npm, GitHub Actions) · agente de IA con demasiado
permiso · robo o pérdida de un dispositivo o de una cuenta.

## Superficies, estado actual y huecos

| Superficie | Qué hay hoy | Hueco |
|---|---|---|
| **Login** | scrypt N=32768 con sal propia; respuesta genérica `INVALID_CREDENTIALS`; límite en memoria: 40 intentos/IP y 10/email cada 15 min; máximo 8 hashes a la vez | El contador no sobrevive a un reinicio ni se comparte entre instancias. Detrás de un proxy, `req.ip` sería siempre el del proxy (`trust proxy` está en `false`). El registro responde `ACCOUNT_EXISTS` (409): permite saber si un correo está registrado |
| **Sesión** | Token aleatorio, solo se guarda su hash; caduca a los **30 días con renovación deslizante**; se puede revocar | 30 días es mucho para un token robado. No hay cierre de todas las sesiones, ni cambio de contraseña, ni segundo factor |
| **Contraseñas** | 8 a 128 caracteres | Sin comprobación de contraseñas filtradas |
| **API** | Orígenes CORS exactos (nunca `*`); cuerpo máximo 8 KB; sin `x-powered-by`; `nosniff` y `no-store`; la API solo escucha en loopback o en IP privada | **Sin TLS**: en una red real el token viaja en claro. `/evaluate`, `/sessions` y `/auth/me` no tienen límite de peticiones. Sin HSTS |
| **Motor C++ (puerto 8765)** | Solo `127.0.0.1`, token Bearer comparado en tiempo constante, cabeceras y cuerpos acotados, deadlines, logs sin datos | Atiende **una conexión cada vez**: un cliente lento puede retrasar a los demás. Sin rotación de logs. Token en un archivo local |
| **App móvil** | Token en el almacén seguro del sistema; nunca se llama al motor; el token del motor nunca llega al móvil | En web, el token vive en `sessionStorage` (expuesto a XSS): una PWA necesita CSP estricta. Sin *certificate pinning* |
| **Datos (Postgres)** | Migración con CHECKs; `role_changes` solo de inserción; borrado de cuenta conserva la auditoría | Sin instancia, sin copias de seguridad, sin cifrado en reposo decidido. El borrado y la exportación de datos del usuario aún no existen |
| **Roles y poder** | Jerarquía de cinco roles; el primer SUPERDEV solo se asigna con un procedimiento local de un solo uso | Cuenta SUPERDEV comprometida = control total: hace falta segundo factor para ella. Hoy no hay |
| **Dependencias** | `package-lock.json` fijo; 29 avisos de `npm audit` (10 moderados, 19 altos) | Todos están en las **herramientas de Expo y React Native** (Metro, `@expo/cli`, `braces`, `node-forge`…), es decir, en el entorno de desarrollo y construcción. **Ninguno en `express` ni `prisma`**, que corren en el servidor. `npm audit fix --force` propone saltos de versión mayores: no aplicarlo a ciegas |
| **CI y repo** | `check:public` revisa secretos y lógica del motor; AGENTS.md prohíbe coautores, `push --force` y fusionar sin permiso | `main` **no está protegido** (cualquiera con escritura puede empujar directo). Las Actions de la CI se fijarán por etiqueta, no por *hash*. El token de `gh` aún no tiene permiso `workflow` |
| **Propiedad intelectual** | Motor en repo privado; el público solo ve contratos | El historial público de `choisys` conserva una implementación matemática provisional de `packages/core` (decisión del dueño pendiente). Los agentes de IA con acceso al repo privado ven el motor |
| **Agentes de IA** | Reglas cerradas en `AGENTS.md`, ramas y PRs con revisión cruzada | Un agente con credenciales puede hacer daño si se salta las reglas: por eso `main` debe estar protegido y las fusiones son solo del dueño |

## Prioridades antes de abrir al público
1. **TLS y proxy inverso** delante de la API (con HSTS y configuración de IP de confianza). Issue choisys #9.
2. **Proteger `main`** (exigir PR) y **segundo factor en la cuenta de GitHub**. Lo hace el dueño en Settings.
3. **Límite de peticiones** en todo lo que no sea login y límite compartido (no en memoria). Issue choisys #12.
4. **Acortar la vida de la sesión** (por ejemplo 7 días sin renovación infinita) y añadir "cerrar todas las sesiones".
5. **Motor concurrente y logs rotados** antes de tráfico real. Issue neos-cube #2.
6. **Decidir el historial público de `packages/core`** antes de dar visibilidad al repo. Issue neos-cube #4.
7. **Borrado y exportación de datos** (RGPD). Issue choisys #4.
8. **Evitar enumeración de correos** en el registro (respuesta uniforme o verificación por correo).

## Lo que NO sabemos todavía
Qué hará de verdad un atacante: no hay tráfico ni telemetría. Cuando exista el primer despliegue,
hay que mirar los logs del proxy (intentos de login, 429, rutas inexistentes) y revisar este documento.
Este modelo se actualiza cada vez que se añada una superficie (endpoints de USER y SUPERDEV, PWA, pagos).
