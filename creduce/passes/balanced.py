from . import AbstractPass
from ..utils import nestedmatcher
from ..utils.error import UnknownArgumentError

class BalancedPass(AbstractPass):
    def check_prerequisites(self):
        return True

    def __get_next_match(self, test_case, pos):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        config = self.__get_config()
        m = nestedmatcher.find(config["search"], prog, pos=pos, prefix=config["prefix"])

        return m

    def new(self, test_case):
        return self.__get_next_match(test_case, pos=0)

    def advance(self, test_case, state):
        return self.__get_next_match(test_case, pos=state[0] + 1)

    def advance_on_success(self, test_case, state):
        return self.__get_next_match(test_case, pos=state[0])

    def __get_config(self):
        config = {"search": None,
                  "replace_fn": None,
                  "prefix": "",
                 }

        def replace_all(string, match):
            return string[0:match[0]] + string[match[1]:]

        def replace_only(string, match):
            return string[0:match[0]] + string[(match[0] + 1):(match[1] - 1)] + string[match[1]:]

        def replace_inside(string, match):
            return string[0:(match[0] + 1)] + string[(match[1] - 1):]

        if self.arg == "square-inside":
            config["search"] = nestedmatcher.BalancedExpr.squares
            config["replace_fn"] = replace_inside
        elif self.arg == "angles-inside":
            config["search"] = nestedmatcher.BalancedExpr.angles
            config["replace_fn"] = replace_inside
        elif self.arg == "parens-inside":
            config["search"] = nestedmatcher.BalancedExpr.parens
            config["replace_fn"] = replace_inside
        elif self.arg == "curly-inside":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_inside
        elif self.arg == "square":
            config["search"] = nestedmatcher.BalancedExpr.squares
            config["replace_fn"] = replace_all
        elif self.arg == "angles":
            config["search"] = nestedmatcher.BalancedExpr.angles
            config["replace_fn"] = replace_all
        elif self.arg == "parens-to-zero":
            config["search"] = nestedmatcher.BalancedExpr.parens
            config["replace_fn"] = lambda string, match: string[0:match[0]] + "0" + string[match[1]:]
        elif self.arg == "parens":
            config["search"] = nestedmatcher.BalancedExpr.parens
            config["replace_fn"] = replace_all
        elif self.arg == "curly":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_all
        elif self.arg == "curly2":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = lambda string, match: string[0:match[0]] + ";" + string[match[1]:]
        elif self.arg == "curly3":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_all
            config["prefix"] = "=\s*"
        elif self.arg == "parens-only":
            config["search"] = nestedmatcher.BalancedExpr.parens
            config["replace_fn"] = replace_only
        elif self.arg == "curly-only":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_only
        elif self.arg == "angles-only":
            config["search"] = nestedmatcher.BalancedExpr.angles
            config["replace_fn"] = replace_only
        elif self.arg == "square-only":
            config["search"] = nestedmatcher.BalancedExpr.squares
            config["replace_fn"] = replace_only
        else:
            raise UnknownArgumentError()

        return config

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        config = self.__get_config()

        while True:
            if state is None:
                return (Result.stop, state)
            else:
                prog2 = config["replace_fn"](prog2, state)

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (Result.ok, state)
                else:
                    state = self.advance(test_case, state)
