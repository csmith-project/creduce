#!/usr/bin/env python3

from .delta import DeltaPass
from ..utils.nestedmatcher import NestedMatcher
from ..utils.error import UnknownArgumentError

class BalancedDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        config = cls.__get_config(arg)
        m = NestedMatcher.find(prog, config["search"], start=state + 1, prefix=config["prefix"])

        return m[0] if m is not None else state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @staticmethod
    def __get_config(arg):
        config = {"search": None,
                  "replace_fn": None,
                  "prefix": "",
                 }

        if arg == "parens":
            config["search"] = ("(", ")")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[1] + 1:]
        elif arg == "curly":
            config["search"] = ("{", "}")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[1] + 1:]
        elif arg == "curly2":
            config["search"] = ("{", "}")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + ";" + content[match[1] + 1:]
        elif arg == "curly3":
            config["search"] = ("{", "}")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[1] + 1:]
            config["prefix"] = "=\s*"
        elif arg == "angles":
            config["search"] = ("<", ">")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[1] + 1:]
        elif arg == "parens-only":
            config["search"] = ("(", ")")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[0] + 1:match[1]] + content[match[1] + 1:]
        elif arg == "curly-only":
            config["search"] = ("{", "}")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[0] + 1:match[1]] + content[match[1] + 1:]
        elif arg == "angles-only":
            config["search"] = ("<", ">")
            config["replace_fn"] = lambda content, match: content[0:match[0]] + content[match[0] + 1:match[1]] + content[match[1] + 1:]
        else:
            raise UnknownArgumentError()

        return config

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            config = cls.__get_config(arg)
            m = NestedMatcher.find(prog2, config["search"], start=state, prefix=config["prefix"])

            if m is None:
                return (DeltaPass.Result.stop, state)
            else:
                prog2 = config["replace_fn"](prog2, m)

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (DeltaPass.Result.ok, state)
                else:
                    state += 1

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run balanced pass!")
