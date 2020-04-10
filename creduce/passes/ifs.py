import filecmp
import os
import shutil
import subprocess
import tempfile
import re

from creduce.passes.abstract import AbstractPass, BinaryState
from creduce.utils import compat

class IfPass(AbstractPass):
    line_regex = re.compile('^\\s*#\\s*if')

    def check_prerequisites(self):
        return shutil.which(self.external_programs["unifdef"]) is not None

    @staticmethod
    def __macro_continues(line):
        return line.rstrip().endswith('\\')

    def __count_instances(self, test_case):
        count = 0
        in_multiline = False
        with open(test_case, "r") as in_file:
            for line in in_file.readlines():
                if in_multiline:
                    if self.__macro_continues(line):
                        continue
                    else:
                        in_multiline = False

                if self.line_regex.search(line):
                    count += 1
                    if self.__macro_continues(line):
                        in_multiline = True
        return count

    def new(self, test_case):
        bs = BinaryState.create(self.__count_instances(test_case))
        if bs:
            bs.value = 0
        return bs

    def advance(self, test_case, state):
        if state.value == 0:
            state.value = 1
        else:
            state.advance()
            state.value = 0
        return state

    def advance_on_success(self, test_case, state):
        return state.advance_on_success(self.__count_instances(test_case))

    def transform(self, test_case, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                i = 0
                in_multiline = False
                for line in in_file.readlines():
                    if in_multiline:
                        if self.__macro_continues(line):
                            continue
                        else:
                            in_multiline = False

                    if self.line_regex.search(line):
                        if state.index <= i and i < state.end():
                            tmp_file.write('#if {0}\n'.format(state.value))
                        i += 1
                        if self.__macro_continues(line):
                            in_multiline = True
                    else:
                        tmp_file.write(line)

        try:
            cmd = [self.external_programs["unifdef"], "-B", "-x", "2", "-k", "-o", test_case, tmp_file.name]
            proc = compat.subprocess_run(cmd, universal_newlines=True)
        except subprocess.SubprocessError:
            return (self.Result.error, state)
        return (self.Result.ok, state)
