# MikanXR_UE

Unreal Engine plugin for MikanXR. It connects the engine to a running Mikan editor over the MikanXR client API, mirrors the Mikan scene (anchors, stencils, shapes, cameras, lights, stages, scenes) as actors under an `AMikanClient`, publishes render targets back to the compositor, and relays DMX light data.

## Scenes

A Mikan project can hold several scenes, including more than one on the same stage, and only one of them is current at a time. The plugin spawns actors for all of them and follows the editor's choice: the scene system's `current_scene_id` sets `AMikanClient`'s active scene, and every other scene's actor subtree is hidden. A hidden actor casts no shadow, which is the point, since an inactive scene's depth proxy mesh would otherwise shadow the live shot.

A scene with the editor's Force Render flag set stays visible while another scene is active. That is how two stages get aligned against each other with both scenes on screen.

`AMikanClient::SetActiveMikanScene` is still Blueprint callable and still fires the `OnSceneActivated` / `OnSceneDeactivated` events, so a Blueprint can override the choice. The next `current_scene_id` change from the editor takes it back.

## Automation

The plugin can be driven and inspected from outside the process through Unreal's Remote Control HTTP API, which the editor starts on `http://127.0.0.1:30010` when the Remote Control plugin is enabled. `Tools/ue_automate.py` is the checked-in client. It mirrors the shape of the MikanXR editor's `tools/automate.py`: each argument is one command, each reply is echoed after a `> command` line, and a failure exits nonzero.

```
python Plugins/MikanXR_UE/Tools/ue_automate.py ready info "mikan status"
python Plugins/MikanXR_UE/Tools/ue_automate.py "mikan connect" "until:mikan status:connected true" "mikan systems" "mikan actors"
python Plugins/MikanXR_UE/Tools/ue_automate.py "pie start" "until:pie status:true" actors screenshot "pie stop"
```

### Setup

- Enable the Remote Control and Python Editor Script plugins in the project.
- Add `Config/DefaultRemoteControl.ini` with `bEnableRemotePythonExecution=True` under `[/Script/RemoteControlCommon.RemoteControlSettings]`. Without it the property, call, and `mikan` commands still work but the Python commands are refused. The settings panel greys this flag out behind Restrict Server Access, but the ini value is what the runtime check reads.
- The HTTP listener binds loopback only. Nothing here is reachable from another machine.

### Commands

The `mikan` commands call `UMikanDebugLibrary`, a static function library in the plugin at the fixed object path `/Script/MikanXR.Default__MikanDebugLibrary`, so a drive needs no discovery step to start. Replies are one record or one `<field> <value>` per line. A failure is a single line beginning `error:`.

- `mikan status` replies the connection state, the pending async request count, and the client info the plugin registered with Mikan
- `mikan connect` / `mikan disconnect` set the subsystem's wants-to-be-connected flag, the same switch as the details panel buttons
- `mikan stage`, `mikan push <stage>`, `mikan pop` read or change the Mikan editor's app stage
- `mikan rc <commandType> [args...]` sends a Mikan remote control command and replies its result lines
- `mikan systems` lists the cached component systems as `<system> <componentClass> <count>`
- `mikan components <system>` lists `<id> <name>` per cached component
- `mikan component <system> <id>` dumps every cached field of one component, keyed by the Mikan property name. An id of `-1` names the system itself and dumps its system-level values, the same convention the Mikan editor's automation channel uses
- `mikan dmx [universe]` lists the cached DMX universes with their non-zero channel counts, or one universe's non-zero channels as `<channel> <value>`
- `mikan client` replies the object path of the `AMikanClient` actor
- `mikan actors` lists spawned actors as `<system> <transformId> <parentTransformId> <actorPath>`
- `mikan actor <transformId>` dumps one spawned actor's class, path, attachment, and relative and world transforms
- `mikan refetch` / `mikan clear` are the details panel's Fetch From Mikan and Clear Editor Actors buttons (editor only)

Actor queries run against the PIE world while a session is running, otherwise the editor world, so the object paths they reply are the ones the raw Remote Control commands need at that moment.

The raw Remote Control routes, printed as JSON:

- `property get <objectPath> <name>` / `property set <objectPath> <name> <json>`
- `call <objectPath> <function> [json]`
- `describe <objectPath>`

Editor Python, run in the shared console scope so globals persist between commands:

- `py <expression>` evaluates and replies the result
- `pyexec <code>` runs statements and replies what they printed
- `pyfile <path> [args]` runs a script file
- `console <command>` runs an editor console command
- `actors [classFilter]` lists `<path> <class> <label>` for the PIE world if running, else the editor world
- `pie start|stop|status` requests play in editor and waits for the state to flip
- `screenshot [path] [WxH]` captures the level viewport through the editor's own `HighResShot` and replies the path (1920x1080 by default). The editor always writes into `Saved/Screenshots/WindowsEditor`, so the client moves the new file when a path is given and otherwise replies where the editor put it.

Client side:

- `log tail <n> [filter]` reads the last n lines of `Saved/Logs/<Project>.log`, optionally containing the filter text (`MikanXRLog` is the plugin's log category). The project directory is derived from the script's location and overridden with `--project`.
- `ready`, `sleep:<seconds>`, and `until:<command>:<expected>` behave as in the MikanXR client: `until` polls a command until a reply line starts with the expected text.

`--port`, `--wait` (connect retry, default 20 s), `--timeout` (per reply and per poll, default 60 s), and `--delay` (between commands, default 0.1 s) match the MikanXR client.

### Limits

- The Python commands need the editor. The Python plugin is not part of packaged builds, so in a `-game` or packaged process only `info`, `property`, `call`, `describe`, and the `mikan` commands answer, and only when the process is launched with `-RCWebControlEnable`.
- Remote Control resolves object paths verbatim. A PIE actor lives under a `UEDPIE_0_` package, which is why `mikan actors` and `actors` reply full paths rather than names.
- `mikan stage`, `push`, `pop`, and `rc` block on the Mikan editor's reply and answer an error when not connected.
- Requests carry a fifteen second deadline. If one goes unanswered the fetch fails instead of hanging, the log names the likely cause, and the details panel shows `Fetch failed`. `mikan fetchstatus` reports the same state the panel shows. Reconnecting with `mikan disconnect` then `mikan connect` rebuilds the store. This is a safety net: the stall it was written for came from the Mikan editor's websocket server and is fixed, so a timeout now means something genuinely went wrong.

## Publishing

`PushToMikanXR_UE.bat` mirrors `Source`, `ThirdParty`, `Tools`, and this README into the standalone plugin repository checkout. `BindMikan.bat` and `FetchMikan.bat` pull the MikanXR client headers, libraries, and DLLs from a MikanXR distribution.
