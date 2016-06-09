#!/usr/bin/env python3

from .delta import DeltaPass
from ..utils.error import UnknownArgumentError

class TernaryDeltaPass(DeltaPass):
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
        raise NotImplementedError("Balanced parenthesis cannot be matched in Python regex")
        #with open(test_case, "r") as in_file:
        #    prog = in_file.read()
        #    prog2 = prog

        #prog2 = cls._replace_nth_match(r"(?P<del1>$borderorspc)(?P<a>$varnumexp)\s*\?\s*(?P<b>$varnumexp)\s*:\s*(?P<c>$varnumexp)(?<del2>$borderorspc)", prog2, state, replace_fn)

        #if arg == "b":
        #    replace_fn = lambda m: m.group("del1") + m.group("b") + m.group("del2")
        #elif arg == "c":
        #    replace_fn = lambda m: m.group("del1") + m.group("c") + m.group("del2")
        #else:
        #    raise UnknownArgumentError()

        #if prog != prog2:
        #    with open(test_case, "w") as out_file:
        #        out_file.write(prog2)

        #    return (DeltaPass.Result.stop, state)
        #else:
        #    return (DeltaPass.Result.stop, state)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run ternary pass!")
