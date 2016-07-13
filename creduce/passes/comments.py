import re

from . import AbstractPass

class CommentsPass(AbstractPass):
    def check_prerequisites(self):
        return True

    def new(self, test_case):
        return -2

    def advance(self, test_case, state):
        return state + 1

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            if state > -1:
                return (self.Result.stop, state)
            elif state == -2:
                # Remove all multiline comments
                # Replace /* any number of * if not followed by / or anything but * */
                # FIXME: What about the rest of the original regex?
                prog2 = re.sub(r"/\*(?:\*(?!/)|[^*])*\*/", "", prog2, flags=re.DOTALL)
            elif state == -1:
                # Remove all single line comments
                prog2 = re.sub(r"//.*$", "", prog2, flags=re.MULTILINE)

            if prog != prog2:
                with open(test_case, "w") as out_file:
                    out_file.write(prog2)

                return (self.Result.ok, state)
            else:
                state = self.advance(test_case, state)
