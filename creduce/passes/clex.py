import os
import shutil
import subprocess
import tempfile

from . import AbstractPass
from ..utils import compat

class ClexPass(AbstractPass):
    def check_prerequisites(self):
        return shutil.which("clex") is not None

    def new(self, test_case):
        return 0

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            try:
                proc = compat.subprocess_run(["clex", str(self.arg), str(state), test_case], universal_newlines=True, stdout=tmp_file)
            except subprocess.SubprocessError:
                return (self.Result.error, state)

        if proc.returncode == 51:
            shutil.move(tmp_file.name, test_case)
            return (self.Result.ok, state)
        else:
            os.unlink(tmp_file.name)

            if proc.returncode == 71:
                return (self.Result.stop, state)
            else:
                return (self.Result.error, state)
