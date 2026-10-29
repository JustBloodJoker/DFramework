"""Portable ONNX Runtime evaluation and PyTorch parity helpers."""

from __future__ import annotations

from pathlib import Path

import numpy as np
import onnxruntime as runtime
import torch

from .model import SpatialVisibilityModel, load_onnx_weights


def parity(path: str | Path, *, height: int = 32, width: int = 32, seed: int = 7) -> dict[str, float]:
    generator = np.random.default_rng(seed)

    base = generator.normal(size=(1, 9, height, width)).astype(np.float32)
    descriptor = generator.uniform(0.0, 1.0, size=(1, 18, height, width)).astype(np.float32)
    hard = generator.uniform(0.0, 1.0, size=(1, 1, height, width)).astype(np.float32)

    session = runtime.InferenceSession(str(path), providers=["CPUExecutionProvider"])
    ort_output = session.run(["visibility"], {"base": base, "descriptor": descriptor, "hard": hard})[0]

    model = load_onnx_weights(SpatialVisibilityModel(), path).eval()
    with torch.no_grad():
        torch_output = model(torch.from_numpy(base), torch.from_numpy(descriptor), torch.from_numpy(hard)).numpy()

    error = np.abs(ort_output - torch_output)

    return {
        "max_abs": float(error.max()),
        "mean_abs": float(error.mean()),
        "p99_abs": float(np.quantile(error, 0.99)),
    }
