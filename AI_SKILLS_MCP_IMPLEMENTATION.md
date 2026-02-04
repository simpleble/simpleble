# SimpleBLE MCP Implementation Notes

This document records the implemented MCP server for SimplePyBLE, its tool surface, and how to run/use it.

## Scope (MVP)

The MCP server exposes a minimal, practical BLE tool set for scanning and basic GATT access:

- `get_adapters`
- `scan_for`
- `connect`
- `disconnect`
- `services`
- `read`
- `notify` (time-bounded sample collection)

No write/indicate/unsubscribe tools are included in this iteration.

## Source files

- MCP server: `simplepyble/src/simplepyble/mcp_server.py`
- Docs: `docs/content/docs/mcp/usage.mdx`
- SimplePyBLE REST server (reference): `simplepyble/src/simplepyble/server.py`

## Tool metadata (FastMCP)

Each tool includes:

- `description` for AI-facing clarity
- `tags` for grouping and filtering
- `annotations` (readOnlyHint, idempotentHint, destructiveHint, openWorldHint)
- `meta` with `version` and functional `role`

This follows the FastMCP guidance for tool descriptions and annotations.

## Outputs and encoding

- Binary values are returned as:
  - `data_hex` (hex string)
  - `data_utf8` (best-effort UTF-8 decode)
- Scan results include `manufacturer_data` as hex per company id.
- On macOS/iOS, device “address” is a UUID, not a MAC address.

## Recommended flow

1. `get_adapters`
2. `scan_for`
3. `connect`
4. `services`
5. `read` or `notify`
6. `disconnect`

## Local install (repo)

The MCP server lives in the repository, so for local dev/testing:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install .
pip install fastmcp
```

## Run the MCP server

STDIO (default, for Cursor MCP):

```bash
python -m simplepyble.mcp_server
```

HTTP (for manual testing):

```bash
python -m simplepyble.mcp_server --transport http --host 127.0.0.1 --port 8000
```

## Cursor MCP config

Example entry for `~/.cursor/mcp.json`:

```json
{
  "mcpServers": {
    "simplepyble": {
      "command": "/Users/alejorojas/work/caos/simpleble/.venv/bin/python",
      "args": ["-m", "simplepyble.mcp_server"],
      "cwd": "/Users/alejorojas/work/caos/simpleble"
    }
  }
}
```

## Manual smoke test

Using an MCP client:

- Call `get_adapters`
- Call `scan_for`
- Verify at least one device appears
- Connect to a device and call `services`

## Notes for AI tool consumers

- Prefer scanning immediately before connecting to ensure the target is in `scan_results`.
- Use `services` before attempting reads or notifications.
- Expect OS-specific visibility for advertisement fields and GATT metadata.
