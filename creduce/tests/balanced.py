import os
import tempfile
import unittest

from ..passes import BalancedPass

class BalancedParensTest(unittest.TestCase):
    def test_parens_no_match(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This is a simple test!\n")

        state = BalancedPass.new(tmp_file.name, "parens")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a simple test!\n")

    def test_parens_simple(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This is a (simple) test!\n")

        state = BalancedPass.new(tmp_file.name, "parens")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a  test!\n")

    def test_parens_nested_outer(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This !\n")

    def test_parens_nested_inner(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens")
        # Transform failed
        state = BalancedPass.advance(tmp_file.name, "parens", state)
        (_, state) = BalancedPass.transform(tmp_file.name, "parens", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This (is a  test)!\n")

class BalancedParensOnlyTest(unittest.TestCase):
    def test_parens_no_match(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This is a simple test!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a simple test!\n")

    def test_parens_simple(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This is a (simple) test!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a simple test!\n")

    def test_parens_nested_outer(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a (simple) test!\n")

    def test_parens_nested_inner(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        # Transform failed
        state = BalancedPass.advance(tmp_file.name, "parens-only", state)
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This (is a simple test)!\n")

    def test_parens_nested_both(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)
        state = BalancedPass.advance_on_success(tmp_file.name, "parens-only", state)
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a simple test!\n")

    def test_parens_nested_all(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("(This) (is a (((more)) complex) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        (result, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        iteration = 0

        while result == BalancedPass.Result.ok and iteration < 7:
            state = BalancedPass.advance_on_success(tmp_file.name, "parens-only", state)
            (result, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 5)
        self.assertEqual(variant, "This is a more complex test!\n")

    def test_parens_nested_no_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("(This) (is a (((more)) complex) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-only")
        (result, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)

        iteration = 0

        while result == BalancedPass.Result.ok and iteration < 7:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("(This) (is a (((more)) complex) test)!\n")

            state = BalancedPass.advance(tmp_file.name, "parens-only", state)
            (result, state) = BalancedPass.transform(tmp_file.name, "parens-only", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 5)

class BalancedParensInsideTest(unittest.TestCase):
    def test_parens_no_match(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This is a simple test!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a simple test!\n")

    def test_parens_simple(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This is a (simple) test!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This is a () test!\n")

    def test_parens_nested_outer(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This ()!\n")

    def test_parens_nested_inner(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        # Transform failed
        state = BalancedPass.advance(tmp_file.name, "parens-inside", state)
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This (is a () test)!\n")

    def test_parens_nested_both(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This (is a (simple) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)
        state = BalancedPass.advance_on_success(tmp_file.name, "parens-inside", state)
        (_, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This ()!\n")

    def test_parens_nested_all(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("(This) (is a (((more)) complex) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        (result, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        iteration = 0

        while result == BalancedPass.Result.ok and iteration < 4:
            state = BalancedPass.advance_on_success(tmp_file.name, "parens-inside", state)
            (result, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
        self.assertEqual(variant, "() ()!\n")

    def test_parens_nested_no_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("(This) (is a (((more)) complex) test)!\n")

        state = BalancedPass.new(tmp_file.name, "parens-inside")
        (result, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)

        iteration = 0

        while result == BalancedPass.Result.ok and iteration < 7:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("(This) (is a (((more)) complex) test)!\n")

            state = BalancedPass.advance(tmp_file.name, "parens-inside", state)
            (result, state) = BalancedPass.transform(tmp_file.name, "parens-inside", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 5)
