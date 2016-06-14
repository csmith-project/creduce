from .delta import DeltaPass
from ..utils.error import UnknownArgumentError
from ..utils import nestedmatcher

class TernaryDeltaPass(DeltaPass):
    varnum = r"(?:[-+]?[0-9a-zA-Z\_]+)"
    border = r"[*{([:,})\];]"
    border_or_space = r"(?:(?:" + border + r")|\s)"
    border_or_space_pattern = nestedmatcher.RegExPattern(border_or_space)
    varnum_pattern = nestedmatcher.RegExPattern(varnum)
    balanced_parens_pattern = nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.parens)
    varnumexp_pattern = nestedmatcher.OrPattern(varnum_pattern, balanced_parens_pattern)

    parts = [(border_or_space_pattern, "del1"),
             varnumexp_pattern,
             nestedmatcher.RegExPattern(r"\s*\?\s*"),
             (varnumexp_pattern, "b"),
             nestedmatcher.RegExPattern(r"\s*:\s*"),
             (varnumexp_pattern, "c"),
             (border_or_space_pattern, "del2")]

    @classmethod
    def check_prerequisites(cls):
        return True

    @classmethod
    def __get_next_match(cls, test_case, arg, pos):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        m = nestedmatcher.search(cls.parts, prog, pos=pos)

        return m

    @classmethod
    def new(cls, test_case, arg):
        return cls.__get_next_match(test_case, arg, pos=0)

    @classmethod
    def advance(cls, test_case, arg, state):
        return cls.__get_next_match(test_case, arg, pos=state["all"][0] + 1)

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return cls.__get_next_match(test_case, arg, pos=state["all"][0])

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            if state is None:
                return (DeltaPass.Result.stop, state)
            else:
                if arg == "b":
                    prog2 = prog2[0:state["del1"][1]] + prog2[state["b"][0]:state["b"][1]] + prog2[state["del2"][0]:]
                elif arg == "c":
                    prog2 = prog2[0:state["del1"][1]] + prog2[state["c"][0]:state["c"][1]] + prog2[state["del2"][0]:]
                else:
                    raise UnknownArgumentError()

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (DeltaPass.Result.ok, state)
                else:
                    print("Advance")
                    print(prog)
                    state = cls.advance(test_case, arg, state)
