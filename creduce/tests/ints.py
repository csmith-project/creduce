import os
import tempfile
import unittest

from ..passes import IntsPass

class IntsTest(unittest.TestCase):
    def test_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "a")
        (_, state) = IntsPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 23L + 0x456 + 0789!\n")

    def test_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "b")
        (_, state) = IntsPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123L + 456 + 0789!\n")

    def test_c(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "c")
        (_, state) = IntsPass.transform(tmp_file.name, "c", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123 + 0x456 + 0789!\n")

    def test_d(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "d")
        (_, state) = IntsPass.transform(tmp_file.name, "d", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123L + 1110 + 0789!\n")

    def test_e(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "e")
        (_, state) = IntsPass.transform(tmp_file.name, "e", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "Compute 123 + 0x456 + 0789!\n")

    def test_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "a")
        (result, state) = IntsPass.transform(tmp_file.name, "a", state)

        iteration = 0

        while result == IntsPass.Result.ok and iteration < 6:
            state = IntsPass.advance_on_success(tmp_file.name, "a", state)
            (result, state) = IntsPass.transform(tmp_file.name, "a", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 4)
        self.assertEqual(variant, "Compute 3L + 0x6 + 0789!\n")

    def test_no_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("Compute 123L + 0x456 + 0789!\n")

        state = IntsPass.new(tmp_file.name, "a")
        (result, state) = IntsPass.transform(tmp_file.name, "a", state)

        iteration = 0

        while result == IntsPass.Result.ok and iteration < 4:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("Compute 123L + 0x456 + 0789!\n")

            state = IntsPass.advance(tmp_file.name, "a", state)
            (result, state) = IntsPass.transform(tmp_file.name, "a", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
