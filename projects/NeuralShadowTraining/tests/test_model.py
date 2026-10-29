from pathlib import Path
import unittest

import torch

from fdw_neural_shadows.evaluate import parity
from fdw_neural_shadows.model import SpatialVisibilityModel, parameter_count, physical_visibility


MODEL = Path(__file__).parents[2] / "FDW" / "Content" / "ShadowRendering" / "NeuralModel" / "neural_shadow_model.onnx"


class ModelTests(unittest.TestCase):
    def test_frozen_parameter_count(self) -> None:
        self.assertEqual(parameter_count(SpatialVisibilityModel()), 59_661)

    def test_physical_correction(self) -> None:
        hard = torch.tensor([[[[0.25]]]])
        hits = torch.tensor([[[[0.50]]]])
        head = torch.tensor([[[[0.0]]]])
        self.assertTrue(torch.equal(physical_visibility(hard, hits, head), hard))

    def test_frozen_onnx_parity(self) -> None:
        result = parity(MODEL, height=16, width=16)
        self.assertLessEqual(result["max_abs"], 2.0e-6)


if __name__ == "__main__":
    unittest.main()
