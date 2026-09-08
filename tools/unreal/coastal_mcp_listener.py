"""Local editor endpoint for the existing Codex Unreal MCP stdio client.

Loaded by the host's Content/Python/init_unreal.py. All Unreal calls run on
the editor thread. This trusted-local automation endpoint is never packaged.
"""
import atexit
import contextlib
import io
import json
import os
import socket
import time

import unreal

HOST, PORT = "127.0.0.1", 30020
MAX_REQUEST = 262144
METHODS = {
    "editor.ping": "Return project identity and connection health.",
    "tools.list": "List this editor's methods.",
    "editor.get_state": "Return map, play world, actor count and dirty packages.",
    "editor.list_actors": "List actor names, labels, classes and transforms.",
    "editor.execute_python": "Execute trusted local Python; args.code; returns stdout and result.",
    "editor.console": "Run args.command in the game world or editor world.",
    "editor.save": "Save dirty maps/content and report success.",
}
_listener = None
_clients = {}
_tick_handle = None
_processing = False


def _worlds():
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if subsystem is None:
        raise RuntimeError("Editor state methods require the interactive Unreal Editor")
    game = subsystem.get_game_world()
    return (None if game else subsystem.get_editor_world()), game


def dispatch(method, args):
    if method not in METHODS:
        raise ValueError("Unknown method: " + str(method))
    if method == "editor.ping":
        return {"project": unreal.Paths.get_project_file_path(),
                "engine": unreal.SystemLibrary.get_engine_version(), "pid": os.getpid()}
    if method == "tools.list":
        return {"tools": [{"name": name, "description": desc} for name, desc in METHODS.items()]}
    if method == "editor.get_state":
        editor, game = _worlds()
        actors = (unreal.GameplayStatics.get_all_actors_of_class(game, unreal.Actor) if game else
                  unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
        return {"project": unreal.Paths.get_project_file_path(),
                "map": editor.get_path_name() if editor else None,
                "play_world": game.get_path_name() if game else None,
                "actor_count": len(actors),
                "dirty_maps": [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
                "dirty_content": [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]}
    if method == "editor.list_actors":
        if _worlds()[1]:
            raise RuntimeError("Stop Play before listing editor actors")
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
        contains = args.get("filter", "")
        if not isinstance(contains, str):
            raise ValueError("filter must be text")
        return {"actors": [{"name": a.get_name(), "label": a.get_actor_label(),
                            "class": a.get_class().get_path_name(),
                            "location": str(a.get_actor_location())}
                           for a in actors if contains.lower() in a.get_actor_label().lower()]}
    if method == "editor.execute_python":
        code = args.get("code")
        if not isinstance(code, str) or not code.strip():
            raise ValueError("code must be nonempty text")
        namespace = {"unreal": unreal, "__name__": "__coastal_mcp__"}
        output = io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            exec(compile(code, "<coastal-mcp>", "exec"), namespace)
        return {"stdout": output.getvalue()[-65536:], "result": namespace.get("result")}
    if method == "editor.console":
        command = args.get("command")
        if not isinstance(command, str) or not command.strip():
            raise ValueError("command must be nonempty text")
        editor, game = _worlds()
        unreal.SystemLibrary.execute_console_command(game or editor, command)
        return {"submitted": True}
    if method == "editor.save":
        return {"saved": unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)}


def handle_request(raw):
    request_id = None
    try:
        request = json.loads(raw.decode("utf-8"))
        if not isinstance(request, dict):
            raise ValueError("Request must be an object")
        request_id = request.get("id")
        if not isinstance(request_id, (str, int)) or isinstance(request_id, bool):
            raise ValueError("id must be text or an integer")
        if request.get("kind") != "call_function":
            raise ValueError("kind must be call_function")
        method, args = request.get("method"), request.get("args", {})
        if not isinstance(method, str) or not isinstance(args, dict):
            raise ValueError("method must be text and args must be an object")
        return {"id": request_id, "ok": True, "result": dispatch(method, args)}
    except Exception as error:
        return {"id": request_id, "ok": False,
                "error": {"message": str(error), "type": type(error).__name__}}


def _close_client(client):
    _clients.pop(client, None)
    client.close()


def encode_response(response):
    try:
        return (json.dumps(response, default=str) + "\n").encode("utf-8")
    except (ValueError, TypeError, RecursionError) as error:
        return (json.dumps({"id": response.get("id"), "ok": False,
                            "error": {"message": "Cannot serialize result: " + str(error)}}) + "\n").encode("utf-8")


def _tick(delta):
    global _processing
    # Import/save may pump nested Slate ticks. Never expire or redispatch the
    # in-flight socket while its engine operation is still executing.
    if _processing:
        return
    _processing = True
    try:
        _pump(delta)
    finally:
        _processing = False


def _pump(_delta):
    if _listener is None:
        return
    # Bound work per frame and drop stalled or oversized clients.
    for _ in range(8 - len(_clients)):
        try:
            client, _address = _listener.accept()
        except BlockingIOError:
            break
        client.setblocking(False)
        _clients[client] = {"input": bytearray(), "output": None, "deadline": time.monotonic() + 10}
    for client, state in list(_clients.items()):
        try:
            if time.monotonic() > state["deadline"]:
                _close_client(client)
                continue
            if state["output"] is None:
                chunk = client.recv(65536)
                if not chunk:
                    _close_client(client)
                    continue
                state["input"].extend(chunk)
                if len(state["input"]) > MAX_REQUEST:
                    _close_client(client)
                    continue
                if b"\n" not in state["input"]:
                    continue
                response = handle_request(bytes(state["input"]).split(b"\n", 1)[0])
                state["output"] = encode_response(response)
                state["deadline"] = time.monotonic() + 10
            sent = client.send(state["output"])
            state["output"] = state["output"][sent:]
            if not state["output"]:
                _close_client(client)
        except BlockingIOError:
            continue
        except OSError:
            _close_client(client)


def stop():
    global _listener, _tick_handle
    if _tick_handle is not None:
        unreal.unregister_slate_post_tick_callback(_tick_handle)
        _tick_handle = None
    for client in list(_clients):
        _close_client(client)
    if _listener is not None:
        _listener.close()
        _listener = None


def start():
    global _listener, _tick_handle
    if _listener is not None:
        return
    command_line = unreal.SystemLibrary.get_command_line().lower()
    if "-run=" in command_line or "-nullrhi" in command_line:
        return
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # Never expose arbitrary editor Python to the LAN or share a bound port.
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        listener.bind((HOST, PORT))
        listener.listen(8)
        listener.setblocking(False)
        _listener = listener
        _tick_handle = unreal.register_slate_post_tick_callback(_tick)
    except Exception:
        listener.close()
        _listener = None
        raise
    atexit.register(stop)
    unreal.log("COASTAL_MCP_READY {}:{}".format(HOST, PORT))
