#!/usr/bin/env python3
"""Erase and flash a specifically identified nRF52840 DK using a packaged SDK."""

import argparse
import hashlib
import json
from pathlib import Path

import pylink


def attach(probe=None, board=None):
    link = pylink.JLink()
    try:
        if probe is None:
            probes = link.connected_emulators()
            if not probes:
                raise RuntimeError("No USB J-Link probe found")
            if len(probes) > 1:
                serials = ", ".join(str(p.SerialNumber) for p in probes)
                raise RuntimeError(
                    f"Multiple J-Link probes found ({serials}); select one with --probe"
                )
            probe = probes[0].SerialNumber
        link.open(serial_no=probe)
        link.set_tif(pylink.enums.JLinkInterfaces.SWD)
        link.connect("NRF52840_XXAA", speed=4000)
        part = link.memory_read32(0x10000100, 1)[0]
        low, high = link.memory_read32(0x10000060, 2)
        actual = f"{high:08x}{low:08x}"
        if part != 0x52840 or (board is not None and actual != board.lower()):
            raise RuntimeError(f"Wrong target: part={part:x}, board={actual}")
        if link.halted():
            raise RuntimeError(
                "Target is halted; explicitly reset/run it before flashing"
            )
        return link, link.serial_number, actual
    except BaseException:
        link.close()
        raise


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--probe", type=int, help="J-Link serial; auto-detected when omitted"
    )
    parser.add_argument("--board", help="Require this FICR board ID")
    parser.add_argument("--build", type=Path, default=Path("hitl/_build/firmware"))
    parser.add_argument("--sdk-cache", type=Path, default=root / ".sdk")
    args = parser.parse_args()
    manifest = json.loads((args.build / "sdk-manifest.json").read_text())
    sdk = args.sdk_cache / manifest["root"]
    archive = args.sdk_cache / manifest["archive"]
    if hashlib.sha256(archive.read_bytes()).hexdigest() != manifest["sha256"]:
        raise RuntimeError("SDK archive checksum mismatch")
    images = [
        sdk
        / "src/simpleembed/nrf52/softdevice/s140/hex/s140_nrf52_7.2.0_softdevice.hex",
        args.build / "simpleble_hitl.hex",
    ]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in images}
    link, probe, board = attach(args.probe, args.board)
    try:
        link.halt()
        link.erase()
        for image in images:
            link.flash_file(str(image.resolve()), 0)
        link.reset(halt=False)
    finally:
        link.close()
    print(json.dumps({"board": board, "probe": probe, "sha256": hashes}, indent=2))


if __name__ == "__main__":
    main()
