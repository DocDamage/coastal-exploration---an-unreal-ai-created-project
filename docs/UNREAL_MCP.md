# Local Unreal MCP connection

The existing Codex server `unreal-mcp-bridge` runs the local Python stdio wrapper
at `C:/Users/Doc/.codex/external/ue5-mcp-bridge-mcp`. Its TCP client expects
newline-delimited `call_function` requests on `127.0.0.1:30020`.

This project supplies the matching editor endpoint in
`tools/unreal/coastal_mcp_listener.py`. The local host loads it from
`F:/coastline/LocalHost/CoastalExploration/Content/Python/init_unreal.py`.
PythonScriptPlugin is already enabled. No additional package or credentials are
required. The startup file finds the original source tools relative to the host;
keep the `LocalHost` and `CoastalExploration` sibling layout on F:.

The editor must restart once after installing the startup file. Then use
`unreal_mcp_ping`, `unreal_mcp_tools_list`, and `unreal_mcp_call` with one of:

| Method | Arguments / behavior |
|---|---|
| `editor.ping` | Project path, engine version and process identity |
| `tools.list` | Registered method descriptions |
| `editor.get_state` | Current map, play world, actor count and dirty packages |
| `editor.list_actors` | Optional `filter` substring against actor labels |
| `editor.execute_python` | `code`; captured output and optional `result` variable |
| `editor.console` | `command`; submission is separate from completion |
| `editor.save` | Save dirty maps/content; explicit success result |

All engine calls execute on the editor's Slate tick thread, using Epic's
[Python tick callback API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/module/unreal?application_version=5.7).
The listener binds only to loopback, uses an exclusive Windows socket, bounds
request size and concurrent clients, and expires incomplete requests. It is a
trusted local code-execution endpoint: local programs can use it to operate this
editor. Do not forward its port or expose it on the network. Long Python calls
block the editor; schedule asynchronous engine work and inspect completion in a
later call. The editor endpoint is not a runtime gameplay service.

For diagnostics, inspect `COASTAL_MCP_READY` in the host log. A refused connection
means the listener has not started. A bind error means another editor owns the
port; do not silently connect to a different project. Always check `editor.ping`
before mutations. Headless `-NullRHI` and `-run=` jobs skip listener startup.

`python -m unittest discover -s tests -p test_mcp_listener.py` checks the protocol
and error boundaries. Live connection and editor operations require a running
editor and are recorded separately in the M3 implementation evidence.
