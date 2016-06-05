#!/usr/bin/env python3

import os
import subprocess
import shutil
import tempfile

from .delta import DeltaPass

class ClangDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return shutil.which("clang_delta") is not None

    @classmethod
    def new(cls, test_case, arg):
        return 1

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            try:
                proc = subprocess.run(["clang_delta", "--transformation={}".format(arg), "--counter={}".format(state), test_case], universal_newlines=True, stdout=tmp_file)
            except subprocess.SubprocessError:
                return (DeltaPass.Result.error, state)

        if proc.returncode == 0:
            shutil.move(tmp_file.name, test_case)
            return (DeltaPass.Result.ok, state)
        else:
            os.unlink(tmp_file.name)

            if proc.returncode == 255 or proc.returncode == 1:
                return (DeltaPass.Result.stop, state)
            else:
                return (DeltaPass.Result.error, state)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run clang pass!")
