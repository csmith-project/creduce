#!/usr/bin/env python3

from .delta import DeltaPass
from utils.error import UnknownArgumentError

class IntsDeltaPass(DeltaPass):
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

        re_border_or_space = r"(?:(?:[*,:;{}[\]()])|\s)"

        #FIXME: Changed: Only stop if no match is found. Not if no change is made
        #FIXME: Could potentially match variable names

        while True:
            if arg == "a":
                # Delete first digit
                replace_fn = lambda m: m.group("pref") + m.group("numpart") + m.group("suf")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0}[+-]?(?:0|(0[xX]))?)[0-9a-fA-F](?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "b":
                # Delete prefix
                # FIXME: Made 0x mandatory

                replace_fn = lambda m: m.group("del") + m.group("numpart") + m.group("suf")
                prog2 = cls._replace_nth_match(r"(?P<del>{0})(?P<pref>[+-]?(?:0|(0[xX])))(?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "c":
                # Delete suffix
                #FIXME: Changed star to plus for suffix
                replace_fn = lambda m: m.group("pref") + m.group("numpart") + m.group("del")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0}[+-]?(?:0|(0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "d":
                # Hex to dec
                replace_fn = lambda m: m.group("pref") + str(int(m.group("numpart"), 16)) + m.group("suf")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0})(?P<numpart>0[Xx][0-9a-fA-F]+)(?P<suf>[ULul]*{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "e":
                #FIXME: Same as c?!
                replace_fn = lambda m: m.group("pref") + m.group("numpart") + m.group("del")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0}[+-]?(?:0|(0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>{0})".format(re_border_or_space), prog2, state, replace_fn)
            else:
                raise UnknownArgumentError()

            if prog2 is None:
                return (DeltaPass.Result.stop, state)
            else:
                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (DeltaPass.Result.ok, state)
                else:
                    state += 1

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run ints pass!")
