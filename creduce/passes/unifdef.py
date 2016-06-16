import filecmp
import os
import shutil
import subprocess
import tempfile

from .delta import DeltaPass

class UnIfDefDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return shutil.which("unifdef") is not None

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
        try:
            proc = subprocess.run(["unifdef", "-s", test_case], universal_newlines=True, stdout=subprocess.PIPE)
        except subprocess.SubprocessError:
            return (DeltaPass.Result.error, state)

        defs = {}

        for l in proc.stdout.splitlines():
            defs[l] = 1

        deflist = list(sorted(defs.keys()))

        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            while True:
                du = "-D" if state % 2 == 0 else "-U"
                n_index = state / 2

                if n_index >= len(deflist):
                    #FIXME: Changed: No unlink in Perl script
                    os.unlink(tmp_file.name)
                    return (DeltaPass.Result.stop, state)

                def_ = deflist[n_index]

                try:
                    proc = subprocess.run(["unifdef", "-B", "-x", "2", "{}{}".format(du, def_), "-o", tmp_file.name, test_case], universal_newlines=True)
                except subprocess.SubprocessError:
                    return (DeltaPass.Result.error, state)

                if filecmp.cmp(test_case, tmp_file.name, shallow=False):
                    state += 1
                    continue

                shutil.move(tmp_file.name, test_case)
                return (DeltaPass.Result.ok, state)
