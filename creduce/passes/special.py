import re

from .delta import DeltaPass
from ..utils.error import UnknownArgumentError

class SpecialDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def __get_config(cls, arg):
        config = {"search": None,
                  "replace_fn": None,
                 }

        def replace_printf(m):
            return r'printf("%d\n", (int){})'.format(m.group("list").split(",")[0])

        def replace_empty(m):
            return ""

        if arg == "a":
            config["search"] = r"transparent_crc\s*\((?P<list>[^)]*)\)"
            config["replace_fn"] = replace_printf
        elif arg == "b":
            config["search"] = r'extern "C"'
            config["replace_fn"] = replace_empty
        elif arg == "c":
            config["search"] = r'extern "C\+\+"'
            config["replace_fn"] = replace_empty
        else:
            raise UnknownArgumentError()

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
