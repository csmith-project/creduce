import re

from . import AbstractPass
from ..utils import nestedmatcher

class PeepPass(AbstractPass):
    border = r"[*{([:,})\];]"
    border_or_space = r"(?:(?:" + border + r")|\s)"
    border_or_space_optional = r"(?:(?:" + border + r")|\s)?"
    border_or_space_pattern = nestedmatcher.RegExPattern(border_or_space)
    border_or_space_optional_pattern = nestedmatcher.RegExPattern(border_or_space_optional)

    varnum = r"(?:[-+]?[0-9a-zA-Z\_]+)"
    varnum_pattern = nestedmatcher.RegExPattern(varnum)
    balanced_parens_pattern = nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.parens)
    varnumexp_pattern = nestedmatcher.OrPattern(varnum_pattern, balanced_parens_pattern)

    call_parts = [varnum_pattern, nestedmatcher.RegExPattern(r"\s*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.parens)]

    field = r"\.(?:" + varnum + r")"
    index = r"\[(?:" + varnum + r")\]"
    fullvar = [nestedmatcher.RegExPattern(r"[&*]*"), varnumexp_pattern, nestedmatcher.RegExPattern(r"(?:(?:" + field + r")|(?:" + index + r"))*")]
    arith = r"\+|-|%|/|\*"
    comp = r"<=|>=|<|>|==|!=|="
    logic = r"&&|\|\|"
    bit = r"\||&|\^|<<|>>"
    binop = [nestedmatcher.RegExPattern(r"(?:" + arith + r")|(?:" + comp + r")|(?:" + logic + r")|(?:" + bit + r")|->")]

    regexes_to_replace = [
        ([nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.angles)], ""),
        ([nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.parens)], ""),
        ([nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies)], ""),
        ([nestedmatcher.RegExPattern(r"namespace[^{]*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies)], ""),
        ([nestedmatcher.RegExPattern(r"=\s*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies)], ""),
        ([nestedmatcher.RegExPattern(r":\s*[0-9]+\s*;")], ";"),
        ([nestedmatcher.RegExPattern(r";")], ""),
        ([nestedmatcher.RegExPattern(r"\^=")], "="),
        ([nestedmatcher.RegExPattern(r"\|=")], "="),
        ([nestedmatcher.RegExPattern(r"\+=\s*1")], "++"),
        ([nestedmatcher.RegExPattern(r"-=\s*1")], "--"),
        ([nestedmatcher.RegExPattern(r"&=")], "="),
        ([nestedmatcher.RegExPattern(r'".*"')], ""),
        ([nestedmatcher.RegExPattern(r"checksum = ")], ""),
        ([nestedmatcher.RegExPattern(r"'.*'")], ""),
        ([nestedmatcher.RegExPattern(r"&=")], "="),
        ([nestedmatcher.RegExPattern(r"\+=")], "="),
        ([nestedmatcher.RegExPattern(r"-=")], "="),
        ([nestedmatcher.RegExPattern(r"\*=")], "="),
        ([nestedmatcher.RegExPattern(r"/=")], "="),
        ([nestedmatcher.RegExPattern(r"%=")], "="),
        ([nestedmatcher.RegExPattern(r"<<=")], "="),
        ([nestedmatcher.RegExPattern(r">>=")], "="),
        ([nestedmatcher.RegExPattern(r"\+")], ""),
        ([nestedmatcher.RegExPattern(r"-")], ""),
        ([nestedmatcher.RegExPattern(r":")], ""),
        ([nestedmatcher.RegExPattern(r",")], ""),
        ([nestedmatcher.RegExPattern(r"::")], ""),
        ([nestedmatcher.RegExPattern(r"!")], ""),
        ([nestedmatcher.RegExPattern(r"~")], ""),
        ([nestedmatcher.RegExPattern(r"while")], "if"),
        ([nestedmatcher.RegExPattern(r'"[^"]*"')], ""),
        ([nestedmatcher.RegExPattern(r'"[^"]*",')], ""),
        ([nestedmatcher.RegExPattern(r"struct\s*[^{]*\s*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies)], ""),
        ([nestedmatcher.RegExPattern(r"union\s*[^{]*\s*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies)], ""),
        ([nestedmatcher.RegExPattern(r"enum\s*[^{]*\s*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies)], ""),
        ([nestedmatcher.RegExPattern(r"if\s*"), nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.parens)], ""),
    ]

    # these match when preceded and followed by border_or_space
    delimited_regexes_to_replace = [
        ([varnumexp_pattern, nestedmatcher.RegExPattern(r"\s*:")], ""),
        ([varnumexp_pattern, nestedmatcher.RegExPattern(r"\s*:")], ""),
        ([nestedmatcher.RegExPattern(r"goto\s+(?:" + varnum + r");")], ""),
        ([nestedmatcher.RegExPattern(r"class")], "struct"),
        ([nestedmatcher.RegExPattern(r"char")], "int"),
        ([nestedmatcher.RegExPattern(r"short")], "int"),
        ([nestedmatcher.RegExPattern(r"long")], "int"),
        ([nestedmatcher.RegExPattern(r"signed")], "int"),
        ([nestedmatcher.RegExPattern(r"unsigned")], "int"),
        ([nestedmatcher.RegExPattern(r"float")], "int"),
        ([nestedmatcher.RegExPattern(r"double")], "int"),
        ([nestedmatcher.RegExPattern(r"wchar_t")], "int"),
        ([nestedmatcher.RegExPattern(r"bool")], "int"),
        ([nestedmatcher.RegExPattern(r"typeof\s+(?:" + varnum + r")")], "int"),
        ([nestedmatcher.RegExPattern(r"__typeof__\s+(?:" + varnum + r")")], "int"),
        ([nestedmatcher.RegExPattern(r"int\s+argc\s*,\s*.*\s*\*argv\[\]")], "void"),
        ([nestedmatcher.RegExPattern(r"int\s+argc\s*,\s*.*\s*\*\s*\*argv")], "void"),
        ([nestedmatcher.RegExPattern(r"int[^;]*;")], ""),
        ([nestedmatcher.RegExPattern(r"for")], ""),
        ([nestedmatcher.RegExPattern(r'".*"')], ""),
        ([nestedmatcher.RegExPattern(r"'.*'")], ""),
        ([nestedmatcher.RegExPattern(r'"[^"]*"')], ""),
        ([nestedmatcher.RegExPattern(r"'[^']*'")], ""),
        ([nestedmatcher.RegExPattern(r'""')], "0"),
        (call_parts + [nestedmatcher.RegExPattern(r",")], "0"),
        (call_parts + [nestedmatcher.RegExPattern(r",")], ""),
        (call_parts, "0"),
        (call_parts, ""),
        ]

    __subexprs = [
        fullvar + [nestedmatcher.RegExPattern(r"\s*")] + binop + [nestedmatcher.RegExPattern(r"\s*")] + fullvar,
        fullvar + [nestedmatcher.RegExPattern(r"\s*")] + binop,
        binop + [nestedmatcher.RegExPattern(r"\s*")] + fullvar,
        fullvar,
        [nestedmatcher.RegExPattern(r":\s*")] + fullvar,
        [nestedmatcher.RegExPattern(r"::\s*")] + fullvar,
        fullvar + [nestedmatcher.RegExPattern(r"\s*:")],
        fullvar + [nestedmatcher.RegExPattern(r"\s*::")],
        fullvar + [nestedmatcher.RegExPattern(r"\s*\?\s*")] + fullvar + [nestedmatcher.RegExPattern(r"\s*:\s*")] + fullvar,
    ]

    for x in __subexprs:
        delimited_regexes_to_replace.append((x, "0"))
        delimited_regexes_to_replace.append((x, "1"))
        delimited_regexes_to_replace.append((x, ""))
        delimited_regexes_to_replace.append((x + [nestedmatcher.RegExPattern(r"\s*,")], "0"))
        delimited_regexes_to_replace.append((x + [nestedmatcher.RegExPattern(r"\s*,")], "1"))
        delimited_regexes_to_replace.append((x + [nestedmatcher.RegExPattern(r"\s*,")], ""))
        delimited_regexes_to_replace.append(([nestedmatcher.RegExPattern(r",\s*")] + x, "0"))
        delimited_regexes_to_replace.append(([nestedmatcher.RegExPattern(r",\s*")] + x, "1"))
        delimited_regexes_to_replace.append(([nestedmatcher.RegExPattern(r",\s*")] + x, ""))

    def check_prerequisites(self):
        return True

    def new(self, test_case):
        return {"pos" : 0, "regex" : 0}

    def advance(self, test_case, state):
        new_state = state.copy()

        if self.arg == "a":
            lim = len(self.regexes_to_replace)
        elif self.arg == "b":
            lim = len(self.delimited_regexes_to_replace);
        elif self.arg == "c":
            lim = 1
        else:
            raise UnknownArgumentError()

        new_state["regex"] += 1

        if new_state["regex"] >= lim:
            new_state["regex"] = 0
            new_state["pos"] += 1

        return new_state

    def advance_on_success(self, test_case, state):
        return state

    def transform(self, test_case, state):
        new_state = state.copy()

        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            if new_state["pos"] > len(prog):
                return (self.Result.stop, new_state)

            if self.arg == "a":
                l = self.regexes_to_replace[new_state["regex"]]
                search = l[0];
                replace = l[1];

                m = nestedmatcher.search(search, prog2, pos=new_state["pos"], search=False)

                if m is not None:
                    prog2 = prog2[0:m["all"][0]] + replace + prog2[m["all"][1]:]

                    if prog != prog2:
                        with open(test_case, "w") as out_file:
                            out_file.write(prog2)

                        return (self.Result.ok, new_state)
            elif self.arg == "b":
                l = self.delimited_regexes_to_replace[new_state["regex"]]
                search = l[0]
                replace = l[1]

                if prog2.startswith(","):
                    front = (self.border_or_space_optional_pattern, "delim1")
                else:
                    front = (self.border_or_space_pattern, "delim1")

                if prog2.endswith(","):
                    back = (self.border_or_space_optional_pattern, "delim2")
                else:
                    back = (self.border_or_space_pattern, "delim2")

                search = [front] + search + [back]

                m = nestedmatcher.search(search, prog2, pos=new_state["pos"], search=False)

                if m is not None:
                    prog2 = prog2[0:m["delim1"][1]] + replace + prog2[m["delim2"][0]:]

                    if prog != prog2:
                        with open(test_case, "w") as out_file:
                            out_file.write(prog2)

                        return (self.Result.ok, new_state)
            elif self.arg == "c":
                search = [nestedmatcher.RegExPattern(r"^while\s*"),
                          nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.parens),
                          nestedmatcher.RegExPattern(r"\s*"),
                          (nestedmatcher.BalancedPattern(nestedmatcher.BalancedExpr.curlies), "body")]

                m = nestedmatcher.search(search, prog2, pos=new_state["pos"], search=False)

                if m is not None:
                    body = prog2[m["body"][0]:m["body"][1]]
                    body = re.sub(r"break\s*;", "", body)
                    prog2 = prog2[0:m["all"][0]] + body + prog2[m["all"][1]:]

                    if prog != prog2:
                        with open(test_case, "w") as out_file:
                            out_file.write(prog2)

                        return (self.Result.ok, new_state)
            else:
                raise UnknownArgumentError()

            new_state = self.advance(test_case, new_state)
