#!/usr/bin/env python3

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
            if state == -2:
                replace_fn = lambda m: m.group(2) if m is not None and m.group(2) is not None else ""
                prog2 = re.sub(r"/\*[^*]*\*+([^/*][^*]*\*+)*/|(\"(\.|[^\"\\])*\"|'(\.|[^'\\])*'|.[^/\"'\\]*)", replace_fn, prog2, flags=re.DOTALL)
            elif state == -1:
                prog2 = re.sub(r"//.*$", "", prog2, flags=re.MULTILINE)
            else:
                #TODO: Is this correct?
                pass

            if prog == prog2 and state == -2:
                state = -1
                continue

            if prog != prog2:
                with open(test_case, "w") as out_file:
                    out_file.write(prog2)

                return (DeltaPass.Result.ok, state)
            else:
                return (DeltaPass.Result.stop, state)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run comments pass!")
