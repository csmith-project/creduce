import os
import tempfile
import unittest

from ..passes import CommentsPass

class CommentsTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = CommentsPass("0")

    def test_block(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This /* contains *** /* two */ /*comments*/!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This  !\n")

    def test_line(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This ///contains //two\n //comments\n!\n")

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This \n \n!\n")

    def test_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("/*This*/ ///contains //two\n //comments\n!\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 4:
            state = self.pass_.advance_on_success(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
        self.assertEqual(variant, " \n \n!\n")

    def test_no_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("/*This*/ ///contains //two\n //comments\n!\n")

        state = self.pass_.new(tmp_file.name)
        (result, state) = self.pass_.transform(tmp_file.name, state)

        iteration = 0

        while result == self.pass_.Result.ok and iteration < 4:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("/*This*/ ///contains //two\n //comments\n!\n")

            state = self.pass_.advance(tmp_file.name, state)
            (result, state) = self.pass_.transform(tmp_file.name, state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
