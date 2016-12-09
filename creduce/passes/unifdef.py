import filecmp
import os
import shutil
import subprocess
import tempfile

from . import AbstractPass
from ..utils import compat

class UnIfDefPass(AbstractPass):
    def check_prerequisites(self):
        return shutil.which("unifdef") is not None

    def new(self, test_case):
        return 0

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
        try:
            proc = compat.subprocess_run(["unifdef", "-s", test_case], universal_newlines=True, stdout=subprocess.PIPE)
        except subprocess.SubprocessError:
            return (self.Result.error, state)

        defs = {}

        for l in proc.stdout.splitlines():
            defs[l] = 1

        deflist = list(sorted(defs.keys()))

        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            while True:
                du = "-D" if state % 2 == 0 else "-U"
                n_index = state / 2

                if n_index >= len(deflist):
                    os.unlink(tmp_file.name)
                    return (self.Result.stop, state)

                def_ = deflist[n_index]

                try:
                    proc = compat.subprocess_run(["unifdef", "-B", "-x", "2", "{}{}".format(du, def_), "-o", tmp_file.name, test_case], universal_newlines=True)
                except subprocess.SubprocessError:
                    return (self.Result.error, state)

                if filecmp.cmp(test_case, tmp_file.name, shallow=False):
                    state += 1
                    continue

                shutil.move(tmp_file.name, test_case)
                return (self.Result.ok, state)
