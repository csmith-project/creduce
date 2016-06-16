import os
import tempfile
import unittest

from ..passes.delta import DeltaPass
from ..passes.special import SpecialDeltaPass

class SpecialTest(unittest.TestCase):
    def test_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('// Useless comment\ntransparent_crc(g_376.f0, "g_376.f0", print_hash_value);\ntransparent_crc(g_1194[i].f0, "g_1194[i].f0", print_hash_value);\nint a = 9;')

        state = SpecialDeltaPass.new(tmp_file.name, "a")
        (_, state) = SpecialDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, '// Useless comment\nprintf("%d\\n", (int)g_376.f0);\ntransparent_crc(g_1194[i].f0, "g_1194[i].f0", print_hash_value);\nint a = 9;')

    def test_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('void foo(){} extern "C" {int a;}; a = 9;\n')

        state = SpecialDeltaPass.new(tmp_file.name, "b")
        (_, state) = SpecialDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "void foo(){}  {int a;}; a = 9;\n")

    def test_c(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('void foo(){} extern "C++" {int a;}; a = 9;\n')

        state = SpecialDeltaPass.new(tmp_file.name, "c")
        (_, state) = SpecialDeltaPass.transform(tmp_file.name, "c", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "void foo(){}  {int a;}; a = 9;\n")

    def test_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('// Useless comment\ntransparent_crc(g_376.f0, "g_376.f0", print_hash_value);\ntransparent_crc(g_1194[i].f0, "g_1194[i].f0", print_hash_value);\nint a = 9;')

        state = SpecialDeltaPass.new(tmp_file.name, "a")
        (result, state) = SpecialDeltaPass.transform(tmp_file.name, "a", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 4:
            state = SpecialDeltaPass.advance_on_success(tmp_file.name, "a", state)
            (result, state) = SpecialDeltaPass.transform(tmp_file.name, "a", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
        self.assertEqual(variant, '// Useless comment\nprintf("%d\\n", (int)g_376.f0);\nprintf("%d\\n", (int)g_1194[i].f0);\nint a = 9;')

    def test_no_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('// Useless comment\ntransparent_crc(g_376.f0, "g_376.f0", print_hash_value);\ntransparent_crc(g_1194[i].f0, "g_1194[i].f0", print_hash_value);\nint a = 9;')

        state = SpecialDeltaPass.new(tmp_file.name, "a")
        (result, state) = SpecialDeltaPass.transform(tmp_file.name, "a", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 4:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write('// Useless comment\ntransparent_crc(g_376.f0, "g_376.f0", print_hash_value);\ntransparent_crc(g_1194[i].f0, "g_1194[i].f0", print_hash_value);\nint a = 9;')

            state = SpecialDeltaPass.advance(tmp_file.name, "a", state)
            (result, state) = SpecialDeltaPass.transform(tmp_file.name, "a", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
