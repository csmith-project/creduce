import shutil
import subprocess

from . import AbstractPass
from ..utils import compat
from ..utils.error import UnknownArgumentError

class IndentPass(AbstractPass):
    def check_prerequisites(self):
        return shutil.which(self.external_programs["clang-format"]) is not None

    def new(self, test_case):
        return 0

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state + 1

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            old = in_file.read()

        if state != 0:
            return (self.Result.stop, state)

        cmd = [self.external_programs["clang-format"], "-i"]

        if self.arg == "regular":
            cmd.extend(["-style", "{SpacesInAngles: true}", test_case])
        elif self.arg == "final":
            cmd.append(test_case)
        else:
            raise UnknownArgumentError()

        try:
            compat.subprocess_run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.SubprocessError:
            return (self.Result.error, state)

        with open(test_case, "r") as in_file:
            new = in_file.read()

        if old == new:
            return (self.Result.stop, state)
        else:
            return (self.Result.ok, state)
