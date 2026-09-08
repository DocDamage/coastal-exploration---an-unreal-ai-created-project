"""Geometry handedness regression for the terrain exported to Unreal."""
import importlib.util
from pathlib import Path
import tempfile
import unittest


class ObjWindingTests(unittest.TestCase):
    def test_reflection_preserves_surface_orientation(self):
        source = Path(__file__).resolve().parents[1] / "tools/unreal/m2_geometry.py"
        spec = importlib.util.spec_from_file_location("coastal_geometry", source)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        mesh = module.Obj()
        for point in [(0, 0, 0), (100, 0, 0), (100, 100, 0), (0, 100, 0)]:
            mesh.vertex(point)
        mesh.face([1, 2, 3, 4], "Ground")
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "quad.obj"
            mesh.write(output)
            lines = output.read_text().splitlines()
        vertices = [tuple(map(float, line.split()[1:])) for line in lines if line.startswith("v ")]
        faces = [[int(value.split('/')[0])-1 for value in line.split()[1:]]
                 for line in lines if line.startswith("f ")]
        self.assertEqual(len(faces), 2)
        for a, b, c in faces:
            ab = tuple(vertices[b][i] - vertices[a][i] for i in range(3))
            ac = tuple(vertices[c][i] - vertices[a][i] for i in range(3))
            # Positive oriented area survives the reflected coordinate export.
            self.assertGreater(ab[0]*ac[1] - ab[1]*ac[0], 0)


if __name__ == "__main__":
    unittest.main()
