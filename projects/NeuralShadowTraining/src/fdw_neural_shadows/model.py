"""Frozen spatial-visibility model and ONNX weight importer."""

from __future__ import annotations

from pathlib import Path

import numpy as np
import onnx
from onnx import numpy_helper
import torch
from torch import Tensor, nn
import torch.nn.functional as functional


def physical_visibility(hard: Tensor, hit_probability: Tensor, head: Tensor) -> Tensor:
    """Apply the frozen physical correction used by the runtime graph."""

    weight = torch.maximum(1.0 - hard, hit_probability)
    return torch.clamp(hard + weight * torch.tanh(head), 0.0, 1.0)


class _Residual(nn.Module):
    def __init__(self, channels: int, *, groups: int = 1, dilation: int = 1) -> None:
        super().__init__()
        self.a = nn.Conv2d(channels, channels, 3, padding=dilation, dilation=dilation, groups=groups)
        self.b = nn.Conv2d(channels, channels, 3, padding=dilation, dilation=dilation, groups=groups)

    def forward(self, value: Tensor) -> Tensor:
        return functional.silu(value + self.b(functional.silu(self.a(value))))


class SpatialVisibilityModel(nn.Module):
    """59,661-parameter full-frame spatial-visibility model."""

    def __init__(self) -> None:
        super().__init__()
        self.projections = nn.ModuleDict(
            {
                "H": nn.Conv2d(1, 15, 1),
                "B": nn.Conv2d(18, 15, 1),
                "G": nn.Conv2d(3, 15, 1),
                "M": nn.Conv2d(2, 15, 1),
            }
        )
        self.grouped_a = _Residual(60, groups=4)
        self.grouped_b = _Residual(60, groups=4)
        self.group_norm_scale = nn.Parameter(torch.ones(4))
        self.branch_scale = nn.Parameter(torch.ones(4))
        self.correction = nn.Conv2d(45, 60, 1)
        self.mixer_down = nn.Conv2d(60, 12, 1)
        self.mixer_up = nn.Conv2d(12, 60, 1)
        self.to_middle = nn.Conv2d(60, 24, 1)
        self.middle = _Residual(24, dilation=2)
        self.late = _Residual(24, dilation=4)
        self.head = nn.Conv2d(24, 1, 1)

    def forward(self, base: Tensor, descriptor: Tensor, hard: Tensor) -> Tensor:
        normal = base[:, 1:4]
        direction = base[:, 4:7]

        normal_length = torch.linalg.vector_norm(normal, dim=1, keepdim=True)
        direction_length = torch.linalg.vector_norm(direction, dim=1, keepdim=True)

        valid = ((normal_length > 0.5) & (direction_length > 0.5)).to(base.dtype)

        cosine = torch.clamp(
            torch.sum(
                normal / normal_length.clamp_min(1.0e-6) * (direction / direction_length.clamp_min(1.0e-6)),
                dim=1,
                keepdim=True,
            ),
            -1.0,
            1.0,
        ) * valid
        sine = torch.sqrt(torch.clamp(1.0 - cosine * cosine, min=0.0)) * valid

        compact = torch.cat(
            (base[:, 0:1], sine, torch.zeros_like(sine), cosine, base[:, 7:8], base[:, 8:9]),
            dim=1,
        )
        projected = torch.cat(
            (
                functional.silu(self.projections["H"](compact[:, 0:1])),
                functional.silu(self.projections["B"](descriptor)),
                functional.silu(self.projections["G"](compact[:, 1:4])),
                functional.silu(self.projections["M"](compact[:, 4:6])),
            ),
            dim=1,
        )

        grouped = self.grouped_b(self.grouped_a(projected))
        chunks = torch.chunk(grouped, 4, dim=1)
        normalized = tuple(
            chunk / torch.sqrt(torch.mean(chunk * chunk, dim=(1, 2, 3), keepdim=True) + 1.0e-6)
            * self.group_norm_scale[index]
            for index, chunk in enumerate(chunks)
        )

        replicated = torch.cat(tuple(normalized[0] * scale for scale in self.branch_scale), dim=1)
        mixed = functional.silu(replicated + self.correction(torch.cat(normalized[1:], dim=1)))
        mixed = functional.silu(mixed + self.mixer_up(functional.silu(self.mixer_down(mixed))))

        value = functional.silu(self.to_middle(mixed))
        value = self.late(self.middle(value))

        return physical_visibility(hard, descriptor[:, :9].mean(dim=1, keepdim=True), self.head(value))


def load_onnx_weights(model: SpatialVisibilityModel, path: str | Path) -> SpatialVisibilityModel:
    """Load the frozen ONNX initializers without changing their values."""

    graph = onnx.load(str(path))
    arrays = {item.name: numpy_helper.to_array(item).copy() for item in graph.graph.initializer}
    state = model.state_dict()

    for name in list(state):
        if name in ("group_norm_scale", "branch_scale"):
            continue

        source_name = name
        if source_name not in arrays:
            raise ValueError(f"missing ONNX initializer: {source_name}")

        state[name] = torch.from_numpy(arrays[source_name]).to(dtype=state[name].dtype)

    state["group_norm_scale"] = torch.tensor(
        [arrays[f"onnx::Mul_{index}"].item() for index in range(328, 332)],
        dtype=torch.float32,
    )

    state["branch_scale"] = torch.tensor(
        [arrays[f"onnx::Mul_{index}"].item() for index in range(332, 336)],
        dtype=torch.float32,
    )

    model.load_state_dict(state, strict=True)
    return model


def parameter_count(model: nn.Module) -> int:
    return sum(parameter.numel() for parameter in model.parameters())
