from .delta import DeltaPass
from ..utils import nestedmatcher
from ..utils.error import UnknownArgumentError

class BalancedDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def __get_next_match(cls, test_case, arg, pos):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        config = cls.__get_config(arg)
        m = nestedmatcher.find(config["search"], prog, pos=pos, prefix=config["prefix"])

        return m

    @classmethod
    def new(cls, test_case, arg):
        return cls.__get_next_match(test_case, arg, pos=0)

    @classmethod
    def advance(cls, test_case, arg, state):
        return cls.__get_next_match(test_case, arg, pos=state[0] + 1)

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return cls.__get_next_match(test_case, arg, pos=state[0])

    @staticmethod
    def __get_config(arg):
        config = {"search": None,
                  "replace_fn": None,
                  "prefix": "",
                 }

        def replace_all(string, match):
            return string[0:match[0]] + string[match[1]:]

        def replace_only(string, match):
            return string[0:match[0]] + string[(match[0] + 1):(match[1] - 1)] + string[match[1]:]

        if arg == "parens":
            config["search"] = nestedmatcher.BalancedExpr.parens
            config["replace_fn"] = replace_all
        elif arg == "curly":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_all
        elif arg == "curly2":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = lambda string, match: string[0:match[0]] + ";" + string[match[1]:]
        elif arg == "curly3":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_all
            config["prefix"] = "=\s*"
        elif arg == "angles":
            config["search"] = nestedmatcher.BalancedExpr.angles
            config["replace_fn"] = replace_all
        elif arg == "parens-only":
            config["search"] = nestedmatcher.BalancedExpr.parens
            config["replace_fn"] = replace_only
        elif arg == "curly-only":
            config["search"] = nestedmatcher.BalancedExpr.curlies
            config["replace_fn"] = replace_only
        elif arg == "angles-only":
            config["search"] = nestedmatcher.BalancedExpr.angles
            config["replace_fn"] = replace_only
        else:
            raise UnknownArgumentError()

        return config

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
                prog2 = config["replace_fn"](prog2, state)

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (DeltaPass.Result.ok, state)
                else:
                    state = cls.advance(test_case, arg, state)
