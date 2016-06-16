import os
import shutil
import subprocess
import tempfile

from .delta import DeltaPass

class ClexDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return shutil.which("clex") is not None

    @classmethod
    def new(cls, test_case, arg):
        return 0

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
                proc = subprocess.run(["clex", str(arg), str(state), test_case], universal_newlines=True, stdout=tmp_file)
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
