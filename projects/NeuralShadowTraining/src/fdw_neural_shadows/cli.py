"""Command-line entry points for model and dataset validation."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import onnx

from .dataset import load_sample
from .evaluate import parity


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="fdw-neural-shadows")
    commands = parser.add_subparsers(dest="command", required=True)

    inspect_model = commands.add_parser("inspect-model")
    inspect_model.add_argument("path", type=Path)

    check_parity = commands.add_parser("parity")
    check_parity.add_argument("path", type=Path)

    inspect_sample = commands.add_parser("inspect-sample")
    inspect_sample.add_argument("path", type=Path)

    args = parser.parse_args(argv)
    if args.command == "inspect-model":
        model = onnx.load(str(args.path))
        onnx.checker.check_model(model)
        result = {
            "sha256": hashlib.sha256(args.path.read_bytes()).hexdigest(),
            "inputs": [value.name for value in model.graph.input],
            "outputs": [value.name for value in model.graph.output],
        }
    elif args.command == "parity":
        result = parity(args.path)
    else:
        sample = load_sample(args.path)
        result = {
            "sample_id": sample.sample_id,
            "generation_id": sample.generation_id,
            "blocks": sorted(sample.blocks),
        }

    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
