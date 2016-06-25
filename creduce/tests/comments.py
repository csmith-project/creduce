import os
import tempfile
import unittest

from ..passes.delta import DeltaPass
from ..passes.comments import CommentsDeltaPass

class CommentsTest(unittest.TestCase):
    def test_block(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This /* contains *** /* two */ /*comments*/!\n")

        state = CommentsDeltaPass.new(tmp_file.name, "0")
        (_, state) = CommentsDeltaPass.transform(tmp_file.name, "0", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This  !\n")

    def test_line(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("This ///contains //two\n //comments\n!\n")

        state = CommentsDeltaPass.new(tmp_file.name, "0")
        (_, state) = CommentsDeltaPass.transform(tmp_file.name, "0", state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(variant, "This \n \n!\n")

    def test_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("/*This*/ ///contains //two\n //comments\n!\n")

        state = CommentsDeltaPass.new(tmp_file.name, "0")
        (result, state) = CommentsDeltaPass.transform(tmp_file.name, "0", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 4:
            state = CommentsDeltaPass.advance_on_success(tmp_file.name, "0", state)
            (result, state) = CommentsDeltaPass.transform(tmp_file.name, "0", state)
            iteration += 1

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
        self.assertEqual(variant, " \n \n!\n")

    def test_no_success(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write("/*This*/ ///contains //two\n //comments\n!\n")

        state = CommentsDeltaPass.new(tmp_file.name, "0")
        (result, state) = CommentsDeltaPass.transform(tmp_file.name, "0", state)

        iteration = 0

        while result == DeltaPass.Result.ok and iteration < 4:
            with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
                tmp_file.write("/*This*/ ///contains //two\n //comments\n!\n")

            state = CommentsDeltaPass.advance(tmp_file.name, "0", state)
            (result, state) = CommentsDeltaPass.transform(tmp_file.name, "0", state)
            iteration += 1

        os.unlink(tmp_file.name)

        self.assertEqual(iteration, 2)
