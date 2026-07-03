"""Command-line entry point for SimpleAIBLE.

Running ``simpleaible`` with no subcommand starts the MCP server (stdio
transport by default), so the bare command remains a valid MCP server
invocation for agent configurations. The ``install`` and ``uninstall``
subcommands manage the bundled agent skill for locally detected agents.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from dataclasses import dataclass
from importlib import resources
from pathlib import Path
from typing import List

SKILL_NAME = "simpleaible"


@dataclass(frozen=True)
class Agent:
    key: str
    display_name: str
    executable: str
    config_dir: Path

    @property
    def skill_dir(self) -> Path:
        return self.config_dir / "skills" / SKILL_NAME

    def detected(self) -> bool:
        return self.config_dir.is_dir() or shutil.which(self.executable) is not None


def _known_agents() -> List[Agent]:
    home = Path.home()
    return [
        Agent(key="claude", display_name="Claude Code", executable="claude", config_dir=home / ".claude"),
        Agent(key="codex", display_name="Codex", executable="codex", config_dir=home / ".codex"),
    ]


def _select_agents(requested: List[str]) -> List[Agent]:
    agents = _known_agents()
    if requested:
        # Explicitly requested agents are used even if not auto-detected.
        return [agent for agent in agents if agent.key in requested]
    return [agent for agent in agents if agent.detected()]


def _bundled_skill_path() -> Path:
    skill = resources.files(__package__) / "skills" / SKILL_NAME
    path = Path(str(skill))
    if not (path / "SKILL.md").is_file():
        raise FileNotFoundError(f"Bundled skill not found at {path}")
    return path


def install_skills(requested: List[str]) -> int:
    agents = _select_agents(requested)
    if not agents:
        print("No supported agents detected (looked for Claude Code and Codex).")
        print("Use --agent to target one explicitly, or install manually:")
        print("  npx skills add https://github.com/simpleble/simpleble --skill simpleaible")
        return 1

    source = _bundled_skill_path()
    for agent in agents:
        target = agent.skill_dir
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(source, target, dirs_exist_ok=True)
        print(f"Installed '{SKILL_NAME}' skill for {agent.display_name} at {target}")
    return 0


def uninstall_skills(requested: List[str]) -> int:
    agents = _select_agents(requested)
    removed = False
    for agent in agents:
        target = agent.skill_dir
        if not target.is_dir():
            continue
        if not (target / "SKILL.md").is_file():
            # Refuse to delete a directory that doesn't look like an installed skill.
            print(f"Skipping {target}: no SKILL.md found, not removing.")
            continue
        shutil.rmtree(target)
        removed = True
        print(f"Removed '{SKILL_NAME}' skill for {agent.display_name} from {target}")

    if not removed:
        print(f"No installed '{SKILL_NAME}' skill found.")
    return 0


def main() -> None:
    argv = sys.argv[1:]

    if argv and argv[0] in ("install", "uninstall"):
        parser = argparse.ArgumentParser(
            prog=f"simpleaible {argv[0]}",
            description=f"{argv[0].capitalize()} the SimpleAIBLE agent skill for detected agents.",
        )
        parser.add_argument(
            "--agent",
            action="append",
            choices=[agent.key for agent in _known_agents()],
            default=[],
            help="Target a specific agent (repeatable). Defaults to all detected agents.",
        )
        args = parser.parse_args(argv[1:])
        handler = install_skills if argv[0] == "install" else uninstall_skills
        sys.exit(handler(args.agent))

    # Default: run the MCP server. Imported lazily so skill management works
    # in environments where the BLE backend is unavailable.
    from . import mcp

    mcp.main()


if __name__ == "__main__":
    main()
