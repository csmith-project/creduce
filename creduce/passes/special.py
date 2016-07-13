import re

from . import AbstractPass
from ..utils.error import UnknownArgumentError

class SpecialPass(AbstractPass):
    def check_prerequisites(self):
        return True

    def __get_config(self):
        config = {"search": None,
                  "replace_fn": None,
                 }

        def replace_printf(m):
            return r'printf("%d\n", (int){})'.format(m.group("list").split(",")[0])

        def replace_empty(m):
            return ""

        if self.arg == "a":
            config["search"] = r"transparent_crc\s*\((?P<list>[^)]*)\)"
            config["replace_fn"] = replace_printf
        elif self.arg == "b":
            config["search"] = r'extern "C"'
            config["replace_fn"] = replace_empty
        elif self.arg == "c":
            config["search"] = r'extern "C\+\+"'
            config["replace_fn"] = replace_empty
        else:
            raise UnknownArgumentError()

        return config

    def __get_next_match(self, test_case, pos):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        config = self.__get_config()
        regex = re.compile(config["search"], flags=re.DOTALL)
        m = regex.search(prog, pos=pos)

        return m

    def new(self, test_case):
        return self.__get_next_match(test_case, pos=0)

    def advance(self, test_case, state):
        return self.__get_next_match(test_case, pos=state.start() + 1)

    def advance_on_success(self, test_case, state):
        return self.__get_next_match(test_case, pos=state.start())

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        config = self.__get_config()

        while True:
            if state is None:
                return (self.Result.stop, state)
            else:
                prog2 = prog2[:state.start()] + config["replace_fn"](state) + prog2[state.end():]

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (self.Result.ok, state)
                else:
                    state = self.advance(test_case, state)
