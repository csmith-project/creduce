import re

from . import AbstractPass
from ..utils.error import UnknownArgumentError

class IntsPass(AbstractPass):
    border_or_space = r"(?:(?:[*,:;{}[\]()])|\s)"

    def check_prerequisites(self):
        return True

    def __get_config(self):
        config = {"search": None,
                  "replace_fn": None,
                 }

        if self.arg == "a":
            # Delete first digit
            def replace_fn(m):
                return m.group("pref") + m.group("numpart") + m.group("suf")

            config["search"] = r"(?P<pref>" + self.border_or_space + r"[+-]?(?:0|(?:0[xX]))?)[0-9a-fA-F](?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*" + self.border_or_space + r")"
        elif self.arg == "b":
            # Delete prefix
            def replace_fn(m):
                return m.group("del") + m.group("numpart") + m.group("suf")

            config["search"] = r"(?P<del>" + self.border_or_space + r")(?P<pref>[+-]?(?:0|(?:0[xX])))(?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*" + self.border_or_space + r")"
        elif self.arg == "c":
            # Delete suffix
            def replace_fn(m):
                return m.group("pref") + m.group("numpart") + m.group("del")

            config["search"] = r"(?P<pref>" + self.border_or_space + r"[+-]?(?:0|(?:0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>" + self.border_or_space + r")"
        elif self.arg == "d":
            # Hex to dec
            def replace_fn(m):
                return m.group("pref") + str(int(m.group("numpart"), 16)) + m.group("suf")

            config["search"] = r"(?P<pref>" + self.border_or_space + r")(?P<numpart>0[Xx][0-9a-fA-F]+)(?P<suf>[ULul]*" + self.border_or_space + r")"
        elif self.arg == "e":
            #FIXME: Same as c?!
            def replace_fn(m):
                return m.group("pref") + m.group("numpart") + m.group("del")

            config["search"] = r"(?P<pref>" + self.border_or_space + r"[+-]?(?:0|(?:0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>" + self.border_or_space + r")"
        else:
            raise UnknownArgumentError()

        config["replace_fn"] = replace_fn

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
