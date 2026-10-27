# DMC5 Split-Screen - v6

## Resultado de la prueba v5 (y que anade v6)
| Evidencia (logs v5) | Conclusion |
|---|---|
| `PlayerManager cambio: 0 -> ...63220`; `state` vio 2 jugadores, P1=VergilPL, 2 grabaciones con su `request` | Correccion de v5 **verificada**. |
| `p2spawn` (id=500) x2: `create -> ok`, `isFinished -> TRUE` en ~20 frames, el juego NO se rompio, pero `players` siguio en 2 y no hubo `addPlayerCore/addPlayer` | **Repetir `create` con el delegado del juego NO crea un Player.** Solo se instancian 2 GameObjects (cuerpo + cara) colgados del padre grabado (posible basura en escena). |
| En el original: `create` en f5630, y `addPlayerCore(PlayerV)` ~2000 frames despues (f7587), `addPlayer` en f8012 | El registro NO ocurre dentro de `create`/del delegado inmediato: lo dispara otra cosa mas tarde (inicio de mision / `findPlayer` / inicializacion del componente). |
| `isFinished` y el delegado: `describe_delegate` da `target=null` | No se puede leer el manejador asi. |
| Fin de mision: `unload(RequestData)` -> las grabaciones se descartan solas | Ciclo de vida OK. |

**v6 solo observa** (no cambia el comportamiento): traza `FinishCreationEvent.Invoke`, `PlayerManager.findPlayer/updateMissionInitCheck/initCostumeInfo`; registra los GameObjects que devuelve `create(prefab,padre,true)` y, con `state` y 90 frames despues de `p2spawn`, imprime nombre, padre y si llevan componente `Player`/`Character`; `state` y las lineas `player[i]` ahora incluyen el nombre del GameObject y de su padre. El volcado `recon` incluye `via.GameObject`, `via.Transform` y metodos de arranque de `app.Player`.

## Lo que demostraron los logs de la prueba v4 (y que corrige v5)
| Evidencia | Conclusion |
|---|---|
| `state` dio `getPlayerNum=-1` y `p2spawn` dijo "no hay P1" **durante una mision** (frames 14512-18191, la 2a mision) | **Bug mio**: el plugin cacheaba el `PlayerManager` de la 1a mision (`...F6CF0`). El trazado muestra que el juego crea uno NUEVO por mision (`...83220`). v5 lo resuelve de nuevo cada 30 frames y antes de cada orden. |
| `P1 desaparecio ... se descartan grabaciones` a los 2 frames de grabar la 2a mision | **Bug mio**: al detectar el fin de la 1a mision borraba tambien las grabaciones recien hechas. v5 las ata al `RequestData` que devuelve `create` y solo las olvida cuando el juego llama a `CharacterFactory.unload(request)`. |
| `p2spawn`/`create` propio se grababa a si mismo | v5 lo evita. |
| En la mision, V (`PlayerV`) y Vergil (`PlayerVergilPL`, P1) comparten el MISMO `padInput` (`...17CB36C0`) | Dato clave para la fase de entrada: el `PadInput` no es por personaje, sino por pad/usuario. |
| `get_FOV=0` otra vez | Mi "arreglo" de v4 no era la causa: probablemente el float nativo vuelve por xmm0 y no en el buffer. v5 imprime los bytes crudos; no es critico. |
| XInput directo: mando 0 conectado, `XInputGetState` del juego = 0 llamadas; `via.hid.GamePad` cuenta 1 dispositivo | El juego NO usa XInputGetState: lee los mandos por `via.hid.GamePad`. El aislamiento por MinHook de XInput no sirve; hay que actuar sobre `PadInput`/`GamePad`. |

## Lo que sigue de v4

| Evidencia | Conclusion |
|---|---|
| Banner con fecha de compilacion, `ping`->`pong`, 24 hooks de traza instalados, `players: 0->1->2`, P1 = `app.PlayerVergilPL` | El diagnostico YA es fiable. Cargas la DLL correcta. |
| El juego crea sus jugadores con `CharacterFactory.create(id=500/600, costume=null, parent=<GameObject>, mode=InGame(3), finishEvent=<delegado>)` y luego `create(prefab, parent, true)` por cada prefab; el registro llega solo (`addPlayerCore`, luego `addPlayer`) | **Ese es el flujo real de creacion en mision.** |
| `AdditionalPlayerLoadManager` no se llamo NI UNA VEZ en toda la mision | Esa via es secundaria (la mantengo como experimento `p2load`). |
| `set_manualPlayer` aparecia con `this=app.PlayerChain` / `ActivateMotionSynchronizer` | Es un setter trivial cuyo codigo nativo comparte el compilador con otros tipos: ruido. Quitado de la traza. |
| `app.Player` no declara `doUpdate` pero la cadena Character/GameModel/... si; hooks instalados sin problema | OK |
| Cero llamadas a `XInputGetState(0)` | O no habia mando conectado, o el juego no usa esa funcion. **Conecta un mando y usa la orden `pad`.** |
| `get_FOV=0`, `Aspect=1e-19` de la camara | Error mio: el objeto es nativo y lo invocaba por el tipo equivocado. Corregido. |
| `app.PadInput` tiene `update()`, `setAxisRawL(x,y)`, `set_buttonOnFlags(uint)`, y cada `Player` tiene el suyo | Punto de enganche para el mando del P2 (siguiente fase). |
| `via.render.layer.Scene` tiene `get/set_Camera`, `get_Size`, `get_Region`, `OutputRenderTarget`; `via.SceneView.set_Size` | Hay palanca para la pantalla dividida, en la capa de render (fase posterior). |

## Que hace (v4 + v5)
- **Graba** la llamada real del juego a `CharacterFactory.create` (padre + delegado) y **describe el delegado** (a que objeto y metodo apunta) en `dmc5_ss_trace.log`.
- **`p2spawn [id]`**: repite esa llamada con otro id, usando el padre y el delegado reales. Detecta al nuevo `Player` en `PlayerManager.players` y le engancha el aislamiento de entrada.
- **`pad`**: sonda de mandos (XInput directo + `via.hid.GamePad` por usuario).
- Sonda de camara corregida. `recon` ahora vuelca tambien `FactoryID`, `CostumeData`, `CharacterFactoryData`, `via.UserIndex` y `PlayerControlType`.

## Instalacion
Sobrescribe los archivos del proyecto con los del zip (**hay cambios en**: `main.cpp`, `tracer.*`, `player_manager.*`, `input_isolation.*`, `camera_manager.cpp`, `diagnostics.hpp`, `reflect_util.hpp`). Compila en Release.

## Prueba (en este orden)
1. **Con el plugin ya cargado**, entra en una mision (idealmente conecta un mando). Es imprescindible: `p2spawn` necesita haber visto el `create` de arranque de la mision. Si ya estabas en la mision, reiniciala. (Ahora tambien sirve la 2a, 3a... mision.)
2. Mira `dmc5_ss.log`: debe haber lineas `GRABADO CharacterFactory.create(id=..., mode=3)`.
3. Ordenes (archivo `dmc5_ss_cmd.txt`): `recon`, `state`, `pad`.
4. **`p2spawn`** (sin argumento usa el primer id grabado). Si el juego no se rompe, mira `dmc5_ss.log`: `isFinished`, `PlayerManager.players: 2 -> 3`, `P2 DETECTADO`. Prueba despues `p2spawn 600` (o el otro id grabado).
5. Envia `dmc5_ss.log`, `dmc5_ss_trace.log` y `dmc5_ss_dump.log`.

> Reutilizar el delegado del juego es un experimento: su manejador puede estar pensado para un solo jugador (podria reemplazar a P1, o fallar). El log dira que paso. Hazlo offline; puede cerrar el juego.

## Ordenes
`ping` F5 - `state` F6 - `dump` F7 - `recon` F8 - **`p2spawn [id]` F9** - `p2load` F10 (experimento APL) - `pad` F11 - `p2activate`, `p2finish`, `trace on|off|sum`.
