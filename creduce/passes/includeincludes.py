import os
import re
import shutil
import tempfile

from .delta import DeltaPass

class IncludeIncludesDeltaPass(DeltaPass):
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
        with open(test_case, "r") as in_file:
            with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
                includes = 0
                matched = False

                for line in in_file:
                    include_match = re.match(r'\s*#\s*include\s*"(.*?)"', line)

                    if include_match is not None:
                        includes += 1

                        if includes == state:
                            try:
                                with open(include_match.group(1), "r") as inc_file:
                                    matched = True
                                    tmp_file.write(inc_file.read())
                                    continue
                            #TODO: Lookup correct error to catch!
                            except FileNotFoundError:
                                pass

                    tmp_file.write(line)

        if matched:
            shutil.move(tmp_file.name, test_case)
        else:
            os.unlink(tmp_file.name)

        return matched
