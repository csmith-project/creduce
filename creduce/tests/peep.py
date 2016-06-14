import os
import tempfile
import unittest

from ..passes.delta import DeltaPass
from ..passes.peep import PeepDeltaPass

class PeepTest(unittest.TestCase):
    def test_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {} test;\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " test;\n")

    def test_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {} test;\n")

        state = PeepDeltaPass.new(tmp_file.name, "b")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "struct 0 {} test;\n")
