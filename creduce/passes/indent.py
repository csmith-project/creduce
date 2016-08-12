import shutil
import subprocess

from . import AbstractPass
from ..utils import compat

class IndentPass(AbstractPass):
    def check_prerequisites(self):
        if shutil.which("clang-format") is None:
            return False

        if shutil.which("indent") is None:
            return False

        if shutil.which("astyle") is None:
            return False

        return True

    def new(self, test_case):
        return 0

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state + 1

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            old = in_file.read()

        while True:
            if self.arg == "regular":
                if state != 0:
                    return (self.Result.stop, state)
                else:
                    cmd = ["clang-format", "-i", test_case]
            elif self.arg == "final":
                if state == 0:
                    cmd = ["indent", "-nbad", "-nbap", "-nbbb", "-cs", "-pcs", "-prs", "-saf", "-sai", "-saw", "-sob", "-ss", test_case]
                elif state == 1:
                    cmd = ["astyle", test_case]
                elif state == 2:
                    cmd = ["clang-format", "-i", test_case]
                else:
                    return (self.Result.stop, state)

            try:
                compat.subprocess_run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except subprocess.SubprocessError:
                return (self.Result.error, state)

            with open(test_case, "r") as in_file:
                new = in_file.read()

            if old == new:
                state += 1
            else:
                break

        return (self.Result.ok, state)
