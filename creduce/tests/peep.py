import os
import tempfile
import unittest

from ..passes import PeepPass

class PeepATestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = PeepPass("a")

    def test_a_1(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("<That's a small test> whether the transformation works!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " whether the transformation works!\n")

    def test_a_2(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("{That's a small test} whether the transformation works!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " whether the transformation works!\n")

    def test_a_3(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("namespace creduce {Some more content} which is not interesting!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " which is not interesting!\n")

    def test_a_3(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("namespace {Some more content} which is not interesting!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " which is not interesting!\n")

    def test_a_4(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {} test;\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, " test;\n")

    def test_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 5:
            state = self.pass_.advance_on_success(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 3)
        self.assertEqual(variant, " foo \n")

    def test_no_success_a(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 8:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("struct test_t {int a;} foo = {1};\n")

            state = self.pass_.advance(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 6)

class PeepBTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = PeepPass("b")

    def test_b_1(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {} test;\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "struct 0 {} test;\n")

    def test_success_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 11:
            state = self.pass_.advance_on_success(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 9)
        self.assertEqual(variant, "struct  { ;}  = {};\n")

    def test_no_success_b(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("struct test_t {int a;} foo = {1};\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 38:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("struct test_t {int a;} foo = {1};\n")

            state = self.pass_.advance(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 36)

    def test_infinite_loop(self):
        #FIXME: Why is this test failing?
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write(",0,")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, ",,")

class PeepCTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = PeepPass("c")

    def test_c_1(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("while   (a == b)\n{\n    int a = 4;\n    short b = 5;\n    break;\n}\n\nulong c = 18;\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "{\n    int a = 4;\n    short b = 5;\n    \n}\n\nulong c = 18;\n")

