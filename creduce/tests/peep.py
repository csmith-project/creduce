import os
import tempfile
import unittest

from ..passes.delta import DeltaPass
from ..passes.peep import PeepDeltaPass

class PeepTest(unittest.TestCase):
    def test_a_1(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("<That's a small test> whether the transformation works!\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " whether the transformation works!\n")

    def test_a_2(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("{That's a small test} whether the transformation works!\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " whether the transformation works!\n")

    def test_a_3(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("namespace creduce {Some more content} which is not interesting!\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " which is not interesting!\n")

    def test_a_3(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("namespace {Some more content} which is not interesting!\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " which is not interesting!\n")

    def test_a_4(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {} test;\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " test;\n")

    def test_b_1(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {} test;\n")

        state = PeepDeltaPass.new(tmp_file.name, "b")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "struct 0 {} test;\n")

    def test_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (result, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 5:
            state = PeepDeltaPass.advance_on_success(tmp_file.name, "a", state)
            (result, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 3)
        self.assertEqual(variant, " foo \n")

    def test_no_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = PeepDeltaPass.new(tmp_file.name, "a")
        (result, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 8:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("struct test_t {int a;} foo = {1};\n")

            state = PeepDeltaPass.advance(tmp_file.name, "a", state)
            (result, state) = PeepDeltaPass.transform(tmp_file.name, "a", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 6)

    def test_success_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = PeepDeltaPass.new(tmp_file.name, "b")
        (result, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 9:
            state = PeepDeltaPass.advance_on_success(tmp_file.name, "b", state)
            (result, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 7)
        self.assertEqual(variant, "struct  { ;}  = {};\n")

    def test_no_success_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = PeepDeltaPass.new(tmp_file.name, "b")
        (result, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 38:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("struct test_t {int a;} foo = {1};\n")

            state = PeepDeltaPass.advance(tmp_file.name, "b", state)
            (result, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 36)

    def test_infinite_loop(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write(",0,")

        state = PeepDeltaPass.new(tmp_file.name, "b")
        (_, state) = PeepDeltaPass.transform(tmp_file.name, "b", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, ",,")
