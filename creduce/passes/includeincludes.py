import os
import re
import shutil
import tempfile

from . import AbstractPass

class IncludeIncludesPass(AbstractPass):
    def check_prerequisites(self):
        return True

    def new(self, test_case):
        return 1

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
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
                                    # Go to next include
                                    # Don't write original line back to file
                                    continue
                            except FileNotFoundError:
                                # Do nothing. The original line will be written back
                                pass

                    tmp_file.write(line)

        if matched:
            shutil.move(tmp_file.name, test_case)
            return (self.Result.ok, state)
        else:
            os.unlink(tmp_file.name)
            return (self.Result.stop, state)
