import re

from .delta import DeltaPass
from ..utils.error import UnknownArgumentError

class IntsDeltaPass(DeltaPass):
    border_or_space = r"(?:(?:[*,:;{}[\]()])|\s)"

    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def __get_config(cls, arg):
        config = {"search": None,
                  "replace_fn": None,
                 }

        if arg == "a":
            # Delete first digit
            def replace_fn(m):
                return m.group("pref") + m.group("numpart") + m.group("suf")

            config["search"] = r"(?P<pref>" + cls.border_or_space + r"[+-]?(?:0|(?:0[xX]))?)[0-9a-fA-F](?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*" + cls.border_or_space + r")"
        elif arg == "b":
            # Delete prefix
            # FIXME: Made 0x mandatory
            def replace_fn(m):
                return m.group("del") + m.group("numpart") + m.group("suf")

            config["search"] = r"(?P<del>" + cls.border_or_space + r")(?P<pref>[+-]?(?:0|(?:0[xX])))(?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*" + cls.border_or_space + r")"
        elif arg == "c":
            # Delete suffix
            #FIXME: Changed star to plus for suffix
            def replace_fn(m):
                return m.group("pref") + m.group("numpart") + m.group("del")

            config["search"] = r"(?P<pref>" + cls.border_or_space + r"[+-]?(?:0|(?:0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>" + cls.border_or_space + r")"
        elif arg == "d":
            # Hex to dec
            def replace_fn(m):
                return m.group("pref") + str(int(m.group("numpart"), 16)) + m.group("suf")

            config["search"] = r"(?P<pref>" + cls.border_or_space + r")(?P<numpart>0[Xx][0-9a-fA-F]+)(?P<suf>[ULul]*" + cls.border_or_space + r")"
        elif arg == "e":
            #FIXME: Same as c?!
            def replace_fn(m):
                return m.group("pref") + m.group("numpart") + m.group("del")

            config["search"] = r"(?P<pref>" + cls.border_or_space + r"[+-]?(?:0|(?:0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>" + cls.border_or_space + r")"
        else:
            raise UnknownArgumentError()

        config["replace_fn"] = replace_fn

        return config

    @classmethod
    def __get_next_match(cls, test_case, arg, pos):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        config = cls.__get_config(arg)
        regex = re.compile(config["search"], flags=re.DOTALL)
        m = regex.search(prog, pos=pos)

        return m

    @classmethod
    def new(cls, test_case, arg):
        return cls.__get_next_match(test_case, arg, pos=0)

    @classmethod
    def advance(cls, test_case, arg, state):
        return cls.__get_next_match(test_case, arg, pos=state.start() + 1)

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return cls.__get_next_match(test_case, arg, pos=state.start())

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        config = cls.__get_config(arg)

        #FIXME: Changed: Only stop if no match is found. Not if no change is made
        #FIXME: Could potentially match variable names
        while True:
            if state is None:
                return (DeltaPass.Result.stop, state)
            else:
                prog2 = prog2[:state.start()] + config["replace_fn"](state) + prog2[state.end():]

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (DeltaPass.Result.ok, state)
                else:
                    state = cls.advance(test_case, arg, state)
