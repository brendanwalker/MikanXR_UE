#!/usr/bin/env python3
"""Drive a running Unreal editor that hosts the MikanXR plugin, over the Remote Control HTTP API.

The editor's Remote Control plugin listens on http://127.0.0.1:30010. This client sends each
argument as one command, prints each reply, and exits nonzero on a connection failure or
timeout. The Mikan-specific commands call the plugin's UMikanDebugLibrary at a fixed object
path, so nothing has to be discovered before the first command. The Python commands need
bEnableRemotePythonExecution=True in the project's Config/DefaultRemoteControl.ini.

    python Tools/ue_automate.py ready info "mikan status"
    python Tools/ue_automate.py "mikan connect" "until:mikan status:connected true" "mikan systems"
    python Tools/ue_automate.py "pie start" "until:pie status:true" actors "screenshot" "pie stop"
    python Tools/ue_automate.py "log tail 20 MikanXRLog"

Commands:
    info                                     editor build kind and route count
    mikan status|connect|disconnect          connection state, or ask the plugin to (dis)connect
    mikan stage|push <stage>|pop             the Mikan editor's app stage
    mikan rc <commandType> [args...]         a Mikan remote control command
    mikan systems                            cached component systems and counts
    mikan components <system>                cached component ids and names of one system
    mikan component <system> <id>            every cached field of one component
    mikan dmx [universe]                     cached DMX universes, or one universe's channels
    mikan client                             object path of the MikanClient actor
    mikan actors                             spawned Mikan actors with their object paths
    mikan actor <transformId>                one spawned actor's attachment and transforms
    mikan refetch|clear                      the details panel's fetch and clear buttons
    mikan fetchstatus                        outcome of the last fetch, as the details panel shows it
    property get <objectPath> <name>         Remote Control property read, printed as JSON
    property set <objectPath> <name> <json>  Remote Control property write
    call <objectPath> <function> [json]      Remote Control function call, printed as JSON
    describe <objectPath>                    Remote Control object description
    py <expression>                          evaluate a Python expression in the editor
    pyexec <code>                            run Python statements in the editor
    pyfile <path> [args]                     run a Python file in the editor
    console <command>                        run an editor console command
    actors [classFilter]                     actors in the PIE world if running, else the editor world
    pie start|stop|status                    play in editor
    screenshot [path] [WxH]                  high resolution viewport screenshot
    log tail <n> [filter]                    last n editor log lines, optionally filtered

Client-side arguments:
    ready                 waits until the editor answers (the connect retry already covers a launch)
    sleep:<seconds>       pauses that long instead of the default delay
    until:<cmd>:<expect>  polls the command until a reply line starts with the expected text

Every reply is echoed after a "> command" line. A failure reply from the plugin is a single line
beginning "error:".
"""

import argparse
import json
import os
import shlex
import shutil
import time
import urllib.error
import urllib.request

DEFAULT_PORT = 30010
CONNECT_RETRY_SECONDS = 0.25
POLL_SECONDS = 0.25
DEBUG_LIBRARY = "/Script/MikanXR.Default__MikanDebugLibrary"
PYTHON_LIBRARY = "/Script/PythonScriptPlugin.Default__PythonScriptLibrary"


class RemoteControlClient:
    def __init__(self, port, wait_seconds, timeout_seconds):
        self.base_url = f"http://127.0.0.1:{port}"
        self.timeout = timeout_seconds
        self.unreal_imported = False
        deadline = time.monotonic() + wait_seconds
        while True:
            try:
                self.request("GET", "/remote/info")
                return
            except ConnectionError:
                if time.monotonic() >= deadline:
                    raise SystemExit(f"error: could not connect to {self.base_url}")
                time.sleep(CONNECT_RETRY_SECONDS)

    def request(self, verb, route, body=None):
        """Send one request and return (status, parsed JSON body or None)."""
        data = None
        headers = {}
        if body is not None:
            data = json.dumps(body).encode("utf-8")
            headers["Content-Type"] = "application/json"
        http_request = urllib.request.Request(self.base_url + route, data=data, headers=headers, method=verb)
        try:
            with urllib.request.urlopen(http_request, timeout=self.timeout) as response:
                return response.status, parse_json(response.read())
        except urllib.error.HTTPError as error:
            return error.code, parse_json(error.read())
        except urllib.error.URLError as error:
            raise ConnectionError(str(error.reason))

    def call(self, object_path, function_name, parameters=None):
        """Call a BlueprintCallable function and return the JSON of its return and out parameters."""
        body = {"objectPath": object_path, "functionName": function_name, "parameters": parameters or {}}
        status, reply = self.request("PUT", "/remote/object/call", body)
        if status != 200:
            message = error_message(reply, status)
            if object_path == PYTHON_LIBRARY:
                message += " (remote python execution is disabled: set bEnableRemotePythonExecution in Config/DefaultRemoteControl.ini)"
            raise SystemExit(f"error: {function_name}: {message}")
        return reply or {}

    def debug_lines(self, function_name, **parameters):
        """Call UMikanDebugLibrary and return its reply lines."""
        return list(self.call(DEBUG_LIBRARY, function_name, parameters).get("ReturnValue", []))

    def python(self, code, mode):
        """Run Python in the editor. Returns (succeeded, result text, captured log lines)."""
        reply = self.call(PYTHON_LIBRARY, "ExecutePythonCommandEx", {
            "PythonCommand": code,
            "ExecutionMode": mode,
            "FileExecutionScope": "Public",
        })
        log_lines = []
        for entry in reply.get("LogOutput", []):
            prefix = "" if entry.get("Type") == "Info" else f"{entry.get('Type', '').lower()}: "
            log_lines.append(prefix + entry.get("Output", ""))
        # The final print's newline arrives as an empty trailing entry
        while log_lines and not log_lines[-1]:
            log_lines.pop()
        return bool(reply.get("ReturnValue")), reply.get("CommandResult", ""), log_lines

    def python_eval(self, expression):
        # An expression cannot import, so make sure the shared scope already holds the unreal module.
        if not self.unreal_imported:
            self.python_run("import unreal\n")
            self.unreal_imported = True
        succeeded, result, log_lines = self.python(expression, "EvaluateStatement")
        if not succeeded:
            raise SystemExit("error: python: " + "\n".join(log_lines + [result]))
        return result

    def python_run(self, code):
        """Run statements and return what they printed, one line per entry."""
        succeeded, result, log_lines = self.python(code, "ExecuteFile")
        if not succeeded:
            raise SystemExit("error: python: " + "\n".join(log_lines + [result]))
        return log_lines


def parse_json(raw):
    if not raw:
        return None
    try:
        return json.loads(raw.decode("utf-8", errors="replace"))
    except ValueError:
        return {"errorMessage": raw.decode("utf-8", errors="replace")}


def error_message(reply, status):
    if isinstance(reply, dict) and reply.get("errorMessage"):
        return reply["errorMessage"]
    return f"http {status}"


def parse_size(text):
    width, _, height = text.lower().partition("x")
    return int(width), int(height)


def parse_json_argument(text, what):
    try:
        return json.loads(text)
    except ValueError as error:
        raise SystemExit(f"error: {what} is not valid JSON: {error}")


# -- Command handlers, each returning the reply lines -----

def command_info(client, rest):
    status, reply = client.request("GET", "/remote/info")
    if status != 200:
        raise SystemExit(f"error: info: {error_message(reply, status)}")
    return [
        f"packaged {'true' if reply.get('IsPackaged') else 'false'}",
        f"routes {len(reply.get('HttpRoutes', []))}",
    ]


def command_mikan(client, rest):
    tokens = shlex.split(rest)
    if not tokens:
        raise SystemExit("error: mikan: missing subcommand")
    sub, args = tokens[0], tokens[1:]
    if sub == "status":
        return client.debug_lines("GetConnectionStatus")
    if sub == "connect":
        return client.debug_lines("SetWantsToBeConnected", bConnect=True)
    if sub == "disconnect":
        return client.debug_lines("SetWantsToBeConnected", bConnect=False)
    if sub == "stage":
        return client.debug_lines("GetAppStage")
    if sub == "push":
        return client.debug_lines("PushAppStage", StageName=require(args, 1, "mikan push <stage>")[0])
    if sub == "pop":
        return client.debug_lines("PopAppStage")
    if sub == "rc":
        args = require(args, 1, "mikan rc <commandType> [args...]")
        return client.debug_lines("SendRemoteControlCommand", CommandType=args[0], CommandArgs=args[1:])
    if sub == "systems":
        return client.debug_lines("ListSystems")
    if sub == "components":
        return client.debug_lines("ListComponents", SystemName=require(args, 1, "mikan components <system>")[0])
    if sub == "component":
        args = require(args, 2, "mikan component <system> <id>")
        return client.debug_lines("DescribeComponent", SystemName=args[0], ComponentId=int(args[1]))
    if sub == "dmx":
        if args:
            return client.debug_lines("DescribeDMXUniverse", UniverseId=int(args[0]))
        return client.debug_lines("ListDMXUniverses")
    if sub == "client":
        return client.debug_lines("GetMikanClientPath")
    if sub == "actors":
        return client.debug_lines("ListSpawnedActors")
    if sub == "actor":
        return client.debug_lines("DescribeActor", TransformId=int(require(args, 1, "mikan actor <transformId>")[0]))
    if sub == "refetch":
        return client.debug_lines("RefetchFromMikan")
    if sub == "fetchstatus":
        return client.debug_lines("GetFetchStatus")
    if sub == "clear":
        return client.debug_lines("ClearMikanActors")
    raise SystemExit(f"error: mikan: unknown subcommand '{sub}'")


def command_property(client, rest):
    tokens = rest.split(None, 3)
    if len(tokens) < 3:
        raise SystemExit("error: usage: property get|set <objectPath> <name> [json]")
    action, object_path, name = tokens[0], tokens[1], tokens[2]
    body = {"objectPath": object_path, "propertyName": name}
    if action == "get":
        body["access"] = "READ_ACCESS"
    elif action == "set":
        if len(tokens) < 4:
            raise SystemExit("error: usage: property set <objectPath> <name> <json>")
        body["access"] = "WRITE_ACCESS"
        body["propertyValue"] = {name: parse_json_argument(tokens[3], "property value")}
    else:
        raise SystemExit(f"error: property: unknown action '{action}'")
    status, reply = client.request("PUT", "/remote/object/property", body)
    if status != 200:
        raise SystemExit(f"error: property {action} {name}: {error_message(reply, status)}")
    return json_lines(reply) if reply is not None else []


def command_call(client, rest):
    tokens = rest.split(None, 2)
    if len(tokens) < 2:
        raise SystemExit("error: usage: call <objectPath> <function> [json]")
    parameters = parse_json_argument(tokens[2], "call parameters") if len(tokens) > 2 else {}
    return json_lines(client.call(tokens[0], tokens[1], parameters))


def command_describe(client, rest):
    object_path = rest.strip()
    if not object_path:
        raise SystemExit("error: usage: describe <objectPath>")
    status, reply = client.request("PUT", "/remote/object/describe", {"objectPath": object_path})
    if status != 200:
        raise SystemExit(f"error: describe: {error_message(reply, status)}")
    return json_lines(reply)


def command_py(client, rest):
    return [client.python_eval(rest)]


def command_pyexec(client, rest):
    return client.python_run(rest)


def command_pyfile(client, rest):
    # ExecuteFile treats a command holding a .py path as a file to run, with the remainder as
    # arguments. Resolve the path so a relative or shell-style one still reaches the editor.
    if not rest:
        raise SystemExit("error: usage: pyfile <path> [args]")
    path, _, arguments = rest.partition(" ")
    resolved = os.path.abspath(path).replace("\\", "/")
    return client.python_run(f"{resolved} {arguments}".strip())


def command_console(client, rest):
    return client.python_run(
        "import unreal\n"
        f"unreal.SystemLibrary.execute_console_command(None, {rest!r})\n")


def command_actors(client, rest):
    class_filter = rest.strip()
    return client.python_run(
        "import unreal\n"
        "_mikan_editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)\n"
        "_mikan_world = _mikan_editor.get_game_world() or _mikan_editor.get_editor_world()\n"
        "for _mikan_actor in unreal.GameplayStatics.get_all_actors_of_class(_mikan_world, unreal.Actor):\n"
        f"    if {class_filter!r} in _mikan_actor.get_class().get_name():\n"
        "        print(_mikan_actor.get_path_name(), _mikan_actor.get_class().get_name(), _mikan_actor.get_actor_label())\n")


PIE_STATUS = "unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()"


def command_pie(client, rest):
    action = rest.strip()
    if action == "status":
        return [pie_status(client)]
    if action in ("start", "stop"):
        request = "editor_request_begin_play" if action == "start" else "editor_request_end_play"
        client.python_run(f"import unreal\nunreal.get_editor_subsystem(unreal.LevelEditorSubsystem).{request}()\n")
        expected = "true" if action == "start" else "false"
        wait_until(lambda: pie_status(client) == expected, client.timeout,
                   f"error: pie {action}: play in editor never reported {expected}")
        return [expected]
    raise SystemExit("error: usage: pie start|stop|status")


def pie_status(client):
    return client.python_eval(PIE_STATUS).lower()


def command_screenshot(client, rest, project_dir):
    # The editor's own HighResShot command, not AutomationLibrary.take_high_res_screenshot: the
    # latter returns a completed task but writes an empty black image outside an automation run.
    # HighResShot always writes into Saved/Screenshots, so pick up the new file and move it.
    tokens = rest.split()
    destination = os.path.abspath(tokens[0]) if tokens else None
    width, height = parse_size(tokens[1]) if len(tokens) > 1 else (1920, 1080)

    shot_dir = os.path.join(project_dir, "Saved", "Screenshots", "WindowsEditor")
    before = set(os.listdir(shot_dir)) if os.path.isdir(shot_dir) else set()

    client.python_run(
        "import unreal\n"
        f"unreal.SystemLibrary.execute_console_command(None, 'HighResShot {width}x{height}')\n")

    new_files = []

    def captured():
        if not os.path.isdir(shot_dir):
            return False
        new_files[:] = [name for name in os.listdir(shot_dir) if name not in before and name.endswith(".png")]
        return bool(new_files)

    wait_until(captured, client.timeout, "error: screenshot: no new file appeared in Saved/Screenshots")
    written = os.path.join(shot_dir, sorted(new_files)[-1])
    wait_until(lambda: file_size_settled(written), client.timeout,
               f"error: screenshot: {written} never finished writing")

    if destination is None:
        return [written]

    # shutil, not os.replace: the destination is often on another drive than the project.
    shutil.move(written, destination)
    return [destination]


def file_size_settled(path):
    """True once two reads a poll apart agree, so the capture has finished writing."""
    first = os.path.getsize(path)
    time.sleep(POLL_SECONDS)
    return first > 0 and os.path.getsize(path) == first


def command_log(client, rest, project_dir):
    tokens = rest.split(None, 2)
    if len(tokens) < 2 or tokens[0] != "tail":
        raise SystemExit("error: usage: log tail <n> [filter]")
    count = int(tokens[1])
    text_filter = tokens[2] if len(tokens) > 2 else ""
    log_path = find_log_path(project_dir)
    with open(log_path, encoding="utf-8", errors="replace") as log_file:
        lines = [line.rstrip("\r\n") for line in log_file if text_filter in line]
    return lines[-count:] if count > 0 else []


def find_log_path(project_dir):
    uprojects = [name for name in os.listdir(project_dir) if name.endswith(".uproject")]
    if len(uprojects) != 1:
        raise SystemExit(f"error: log: expected one .uproject in {project_dir}, found {len(uprojects)} (use --project)")
    log_path = os.path.join(project_dir, "Saved", "Logs", uprojects[0][:-len(".uproject")] + ".log")
    if not os.path.exists(log_path):
        raise SystemExit(f"error: log: {log_path} does not exist")
    return log_path


def require(args, count, usage):
    if len(args) < count:
        raise SystemExit(f"error: usage: {usage}")
    return args


def json_lines(value):
    return json.dumps(value, indent=2).splitlines()


def wait_until(predicate, timeout_seconds, failure_message):
    deadline = time.monotonic() + timeout_seconds
    while not predicate():
        if time.monotonic() >= deadline:
            raise SystemExit(failure_message)
        time.sleep(POLL_SECONDS)


def run_command(client, command, project_dir):
    verb, _, rest = command.strip().partition(" ")
    rest = rest.strip()
    handlers = {
        "info": command_info,
        "mikan": command_mikan,
        "property": command_property,
        "call": command_call,
        "describe": command_describe,
        "py": command_py,
        "pyexec": command_pyexec,
        "pyfile": command_pyfile,
        "console": command_console,
        "actors": command_actors,
        "pie": command_pie,
    }
    if verb == "log":
        return command_log(client, rest, project_dir)
    if verb == "screenshot":
        return command_screenshot(client, rest, project_dir)
    if verb not in handlers:
        raise SystemExit(f"error: unknown command '{verb}' (see the module docstring for the list)")
    return handlers[verb](client, rest)


def default_project_dir():
    # Tools/ue_automate.py sits inside <project>/Plugins/MikanXR_UE, three levels below the project.
    return os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    parser = argparse.ArgumentParser(description="Drive the MikanXR Unreal plugin over the Remote Control API.")
    parser.add_argument("commands", nargs="+", help="commands to send, in order")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Remote Control HTTP port")
    parser.add_argument("--wait", type=float, default=20.0, help="seconds to retry the initial connect")
    parser.add_argument("--timeout", type=float, default=60.0, help="seconds to wait for each reply and each poll")
    parser.add_argument("--delay", type=float, default=0.1, help="seconds to pause between commands")
    parser.add_argument("--project", default=default_project_dir(), help="project directory, for the log tail")
    args = parser.parse_args()

    client = RemoteControlClient(args.port, args.wait, args.timeout)

    for index, command in enumerate(args.commands):
        if index > 0:
            time.sleep(args.delay)

        if command.startswith("sleep:"):
            time.sleep(float(command.split(":", 1)[1]))
            continue

        if command.startswith("until:"):
            parts = command.split(":", 2)
            if len(parts) != 3:
                raise SystemExit("error: until needs the form until:<command>:<expected>")
            poll_command, expected = parts[1], parts[2]
            print(f"> until '{poll_command}' answers '{expected}'")
            deadline = time.monotonic() + args.timeout
            while True:
                lines = run_command(client, poll_command, args.project)
                if any(line.startswith(expected) for line in lines):
                    for line in lines:
                        print(line)
                    break
                if time.monotonic() >= deadline:
                    raise SystemExit(f"error: '{poll_command}' never answered '{expected}'")
                time.sleep(POLL_SECONDS)
            continue

        if command == "ready":
            command = "info"

        print(f"> {command}")
        for line in run_command(client, command, args.project):
            print(line)


if __name__ == "__main__":
    main()
