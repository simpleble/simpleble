<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleAIBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleAIBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to AI
agents on Windows, macOS, and Linux. Your agent scans for Bluetooth Low Energy
devices, connects, explores their services, reads and writes GATT values, and
follows notifications, through an MCP server, an HTTP API, or a bundled agent
skill.

## Why SimpleAIBLE

- **Works with your agent:** Use it from Claude Code, Codex, Cursor, Windsurf, or any MCP client, or call the HTTP API from your own tools.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through SimpleBLE's maintained stack.
- **Built for real hardware work:** Let an agent discover, debug, and exercise the BLE devices on your desk.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [MCP server](https://docs.simpleble.org/simpleaible/mcp?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=support), [HTTP API](https://docs.simpleble.org/simpleaible/http?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=support), and [agent skills](https://docs.simpleble.org/simpleaible/skills?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=support) guides and the [tutorial](https://docs.simpleble.org/simpleaible/tutorial?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=support) cover every tool.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simpleaible).

## Get started

Use Python 3.10 or newer, and install SimpleAIBLE with [uv](https://docs.astral.sh/uv/):

```sh
uv tool install simpleaible
```

`pip install simpleaible` works too. Then register the MCP server with your
agent. For Claude Code:

```sh
claude mcp add simpleaible -- simpleaible
```

Other MCP clients take the same command, `simpleaible`, in their configuration.
Ask your agent to scan for five seconds and list what it found.

## MCP server

`simpleaible` starts the MCP server over stdio. Its tools cover the whole
client workflow: `get_adapters`, `scan_for`, `connect`, `services`, `read`,
`write_request`, `write_command`, `notify`, `indicate`, `get_notifications`,
`unsubscribe`, and `disconnect`. The
[MCP server guide](https://docs.simpleble.org/simpleaible/mcp?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=mcp)
has the configuration for each client.

## HTTP API

Run the HTTP server to control BLE devices from your own tools:

```sh
simpleaible-http --host 127.0.0.1 --port 8000
```

The API mirrors the MCP tools: `GET /adapters`, `POST /scan`,
`POST /connect/{address}`, and reads, writes, and subscriptions under
`/device/{address}`. The
[HTTP API reference](https://docs.simpleble.org/simpleaible/http?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=http)
lists every endpoint.

## Agent skills

Install the SimpleAIBLE skill so your agent knows the BLE workflows, platform
differences, and debugging patterns:

```sh
simpleaible install
```

The installer finds Claude Code and Codex and adds the skill to each, and
`simpleaible uninstall` removes it. For other agents, use the `skills` CLI:

```sh
npx skills add https://github.com/simpleble/simpleble --skill simpleaible
```

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 15.0 or newer | x64, arm64 | Bluetooth permission for the app that runs your agent |
| Windows 10 or newer | x64, x86, arm64 | |
| Linux | x64, x86, arm64 | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleAIBLE follows SimpleBLE's release versions. uv and pip install the latest
stable release, and development builds of the main branch are published as
pre-releases:

```sh
uv tool install --prerelease allow simpleaible
```

## License

SimpleAIBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=license)
to discuss your use case. Commercial licenses carry no royalties or revenue
sharing; see the [licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=license)
for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com?utm_source=github&utm_medium=referral&utm_campaign=simpleaible_readme&utm_content=footer).
