import os
import tempfile
import unittest

from ..passes.delta import DeltaPass
from ..passes.ternary import TernaryDeltaPass

class TernaryTest(unittest.TestCase):
    def test_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("int res = a ? b : c;\n")

        state = TernaryDeltaPass.new(tmp_file.name, "b")
        (_, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "int res = b;\n")

    def test_c(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("int res = a ? b : c;\n")

        state = TernaryDeltaPass.new(tmp_file.name, "c")
        (_, state) = TernaryDeltaPass.transform(tmp_file.name, "c", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "int res = c;\n")

    def test_parens(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("int res = (a != 0) ? (b + 5) : c;\n")

        state = TernaryDeltaPass.new(tmp_file.name, "b")
        (_, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "int res = (b + 5);\n")

    def test_all_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("// no ? match :\nint res = a ? (ba ? bb : bc) : c\nint sec = t ? u : v\n")

        state = TernaryDeltaPass.new(tmp_file.name, "b")
        (result, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        while result == DeltaPass.Result.ok:
            state = TernaryDeltaPass.advance_on_success(tmp_file.name, "b", state)
            (result, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "// match res = (bb)\nint sec = u\n")

    def test_all_b_2(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
           tmp_file.write("// no ? match :!\nint res = a ? (ba ? bb : bc) : c\nint sec = t ? u : v\n")

        state = TernaryDeltaPass.new(tmp_file.name, "b")
        (result, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        while result == DeltaPass.Result.ok:
            state = TernaryDeltaPass.advance_on_success(tmp_file.name, "b", state)
            (result, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "// no ? match :!\nint res = (bb)\nint sec = u\n")

    def test_no_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("// no ? match :\nint res = a ? (ba ? bb : bc) : c\nint sec = t ? u : v\n")

        state = TernaryDeltaPass.new(tmp_file.name, "b")
        (result, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 6:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("// no ? match :\nint res = a ? (ba ? bb : bc) : c\nint sec = t ? u : v\n")

            state = TernaryDeltaPass.advance(tmp_file.name, "b", state)
            (result, state) = TernaryDeltaPass.transform(tmp_file.name, "b", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 4)
