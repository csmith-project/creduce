import os
import re
import shutil
import tempfile

from .delta import DeltaPass

class IncludesDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def new(cls, test_case, arg):
        return 1

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        success = cls.__transform(test_case, state)
        return (DeltaPass.Result.ok if success else DeltaPass.Result.stop, state)

    @classmethod
    def __transform(cls, test_case, state):
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
