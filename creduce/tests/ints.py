import os
import tempfile
import unittest

from ..passes import IntsPass

class IntsATestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = IntsPass("a")

    def test_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 23L + 0x456 + 0789!\n")

    def test_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 6:
            state = self.pass_.advance_on_success(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 4)
        self.assertEqual(variant, "Compute 3L + 0x6 + 0789!\n")

    def test_no_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 4:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("Compute 123L + 0x456 + 0789!\n")

            state = self.pass_.advance(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)

class IntsBTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = IntsPass("b")

    def test_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123L + 456 + 0789!\n")

class IntsCTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = IntsPass("c")

    def test_c(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123 + 0x456 + 0789!\n")

class IntsDTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = IntsPass("d")

    def test_d(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123L + 1110 + 0789!\n")

class IntsETestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = IntsPass("e")

    def test_e(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123 + 0x456 + 0789!\n")
