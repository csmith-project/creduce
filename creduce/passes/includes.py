import os
import re
import shutil
import tempfile

from .delta import DeltaPass

class IncludesDeltaPass(DeltaPass):
    def check_prerequisites(self):
        return True

    def new(self, test_case):
        return 1

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
        success = self.__transform(test_case, state)
        return (DeltaPass.Result.ok if success else DeltaPass.Result.stop, state)

    def __transform(self, test_case, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                includes = 0
                matched = False

                for line in in_file:
                    include_match = re.match(r"\s*#\s*include", line)

                    if include_match is not None:
                        includes += 1

                        if includes == state:
                            matched = True
                            continue

                    tmp_file.write(line)

        if matched:
            shutil.move(tmp_file.name, test_case)
        else:
            os.unlink(tmp_file.name)

        return matched
