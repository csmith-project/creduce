#!/usr/bin/env python3

from .delta import DeltaPass
from utils.error import UnknownArgumentError

class SpecialDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def new(cls, test_case, arg):
        return 0

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

        if arg == "a":
            replace_fn = lambda m: 'printf("%d\\n", (int){})'.format(m.group("list").split(",")[0])
            prog2 = cls._replace_nth_match(r"transparent_crc\s*\((?P<list>.*?)\)", prog2, state, replace_fn)
        elif arg == "b":
            prog2 = cls._replace_nth_match('extern "C"', prog2, state, lambda m: "")
        elif arg == "c":
            prog2 = cls._replace_nth_match('extern "C\+\+"', prog2, state, lambda m: "")
        else:
            raise UnknownArgumentError()

        if prog2 is not None and prog != prog2:
            with open(test_case, "w") as out_file:
                out_file.write(prog2)

            return (DeltaPass.Result.ok, state)
        else:
            return (DeltaPass.Result.stop, state)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run special pass!")
