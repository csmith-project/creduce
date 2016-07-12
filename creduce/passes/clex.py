import os
import shutil
import subprocess
import tempfile

from .delta import DeltaPass

class ClexDeltaPass(DeltaPass):
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
                proc = subprocess.run(["clex", str(self.arg), str(state), test_case], universal_newlines=True, stdout=tmp_file)
            except subprocess.SubprocessError:
                return (DeltaPass.Result.error, state)

        if proc.returncode == 51:
            shutil.move(tmp_file.name, test_case)
            return (DeltaPass.Result.ok, state)
        else:
            os.unlink(tmp_file.name)

            if proc.returncode == 71:
                return (DeltaPass.Result.stop, state)
            else:
                return (DeltaPass.Result.error, state)
