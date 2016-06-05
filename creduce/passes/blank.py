#!/usr/bin/env python3

import os
import re
import shutil
import tempfile

from .delta import DeltaPass

class BlankDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @staticmethod
    def __transform(test_case, pattern):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                matched = False

                for l in in_file:
                    if re.match(pattern, l) is not None:
                        matched = True
                    else:
                        tmp_file.write(l)

        if matched:
            shutil.move(tmp_file.name, test_case)
        else:
            os.unlink(tmp_file.name)

        return matched

    @classmethod
    def transform(cls, test_case, arg, state):
        patterns = [r"^\s*$", r"^#"]

        if state >= len(patterns):
            return (DeltaPass.Result.stop, state)
        else:
            success = False

            while not success and state < len(patterns):
                success = cls.__transform(test_case, patterns[state])
                state += 1

            return (DeltaPass.Result.ok if success else DeltaPass.Result.stop, state)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run blank pass!")
