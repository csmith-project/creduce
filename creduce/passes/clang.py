import os
import subprocess
import shutil
import tempfile

from . import AbstractPass
from ..utils import compat

class ClangPass(AbstractPass):
    def check_prerequisites(self):
        return shutil.which("clang_delta") is not None

    def new(self, test_case):
        return 1

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            try:
                proc = compat.subprocess_run(["clang_delta", "--transformation={}".format(self.arg), "--counter={}".format(state), test_case], universal_newlines=True, stdout=tmp_file)
            except subprocess.SubprocessError:
                return (self.Result.error, state)

        if proc.returncode == 0:
            shutil.move(tmp_file.name, test_case)
            return (self.Result.ok, state)
        else:
            os.unlink(tmp_file.name)

            if proc.returncode == 255 or proc.returncode == 1:
                return (self.Result.stop, state)
            else:
                return (self.Result.error, state)
