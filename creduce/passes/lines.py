import logging
import shutil
import subprocess
import tempfile

from creduce.passes.abstract import AbstractPass, BinaryState
from ..utils import compat

class LinesPass(AbstractPass):
    def check_prerequisites(self):
        return shutil.which(self.external_programs["topformflat"]) is not None

    def __format(self, test_case):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                try:
                    cmd = [self.external_programs["topformflat"], self.arg]
                    proc = compat.subprocess_run(cmd, stdin=in_file, stdout=subprocess.PIPE, universal_newlines=True)
                except subprocess.SubprocessError:
                    return (self.Result.error, new_state)

            for l in proc.stdout.splitlines(keepends=True):
                if not l.isspace():
                    tmp_file.write(l)

        shutil.move(tmp_file.name, test_case)

    def __count_instances(self, test_case):
        with open(test_case, "r") as in_file:
            lines = in_file.readlines()
            return len(lines)

    def new(self, test_case):
        self.__format(test_case)
        instances = self.__count_instances(test_case)
        return BinaryState.create(instances)

    def advance(self, test_case, state):
        return state.advance()

    def advance_on_success(self, test_case, state):
        return state.advance_on_success(self.__count_instances(test_case))

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            data = in_file.readlines()

        old_len = len(data)
        data = data[0:state.index] + data[state.end():]
        assert len(data) < old_len

        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            tmp_file.writelines(data)

        shutil.move(tmp_file.name, test_case)

        return (self.Result.ok, state)
