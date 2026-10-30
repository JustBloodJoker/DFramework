"""Dynamic-resolution ONNX export for the spatial-visibility model."""

from __future__ import annotations

from pathlib import Path

import torch

from .model import SpatialVisibilityModel


def export_onnx(model: SpatialVisibilityModel, output: str | Path, *, height: int = 32, width: int = 32) -> Path:
    target = Path(output)
    target.parent.mkdir(parents=True, exist_ok=True)

    inputs = (
        torch.zeros(1, 9, height, width, dtype=torch.float32),
        torch.zeros(1, 18, height, width, dtype=torch.float32),
        torch.ones(1, 1, height, width, dtype=torch.float32),
    )

    torch.onnx.export(
        model.eval(),
        inputs,
        str(target),
        opset_version=17,
        input_names=["base", "descriptor", "hard"],
        output_names=["visibility"],
        dynamic_axes={name: {2: "height", 3: "width"} for name in ("base", "descriptor", "hard", "visibility")},
    )

    return target
