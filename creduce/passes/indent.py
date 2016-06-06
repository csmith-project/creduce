#!/usr/bin/env python3

import shutil
import subprocess

from .delta import DeltaPass

class IndentDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        if shutil.which("clang-format") is None:
            return False

        if shutil.which("indent") is None:
            return False

        if shutil.which("astyle") is None:
            return False

        return True

    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            old = in_file.read()

        while True:
            if arg == "regular":
                if state != 0:
                    return (DeltaPass.Result.stop, state)
                else:
                    cmd = ["clang-format", "-i", test_case]
            elif arg == "final":
                if state == 0:
                    cmd = ["indent", "-nbad", "-nbap", "-nbbb", "-cs", "-pcs", "-prs", "-saf", "-sai", "-saw", "-sob", "-ss", test_case]
                elif state == 1:
                    cmd = ["astyle", test_case]
                elif state == 2:
                    cmd = ["clang-format", "-i", test_case]
                else:
                    return (DeltaPass.Result.stop, state)

            try:
                subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except subprocess.SubprocessError:
                return (DeltaPass.Result.error, state)

            with open(test_case, "r") as in_file:
                new = in_file.read()

            if old == new:
                state += 1
            else:
                break

        return (DeltaPass.Result.ok, state)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run indent pass!")
