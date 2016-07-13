from . import AbstractPass
from ..utils.error import UnknownArgumentError
from ..utils import nestedmatcher

class TernaryPass(AbstractPass):
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

    def check_prerequisites(self):
        return True

    def __get_next_match(self, test_case, pos):
        with open(test_case, "r") as in_file:
            prog = in_file.read()

        m = nestedmatcher.search(self.parts, prog, pos=pos)

        return m

    def new(self, test_case):
        return self.__get_next_match(test_case, pos=0)

    def advance(self, test_case, state):
        return self.__get_next_match(test_case, pos=state["all"][0] + 1)

    def advance_on_success(self, test_case, state):
        return self.__get_next_match(test_case, pos=state["all"][0])

    def transform(self, test_case, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            if state is None:
                return (self.Result.stop, state)
            else:
                if self.arg not in ["b", "c"]:
                    raise UnknownArgumentError()

                prog2 = prog2[0:state["del1"][1]] + prog2[state[self.arg][0]:state[self.arg][1]] + prog2[state["del2"][0]:]

                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (self.Result.ok, state)
                else:
                    state = self.advance(test_case, state)
