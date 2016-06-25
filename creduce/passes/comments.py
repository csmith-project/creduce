import re

from .delta import DeltaPass

class CommentsDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def new(cls, test_case, arg):
        return -2

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            if state > -1:
                return (DeltaPass.Result.stop, state)
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

                return (DeltaPass.Result.ok, state)
            else:
                state = cls.advance(test_case, arg, state)
