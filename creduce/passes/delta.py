import re
import enum

class DeltaPass:
    @enum.unique
    class Result(enum.Enum):
        ok = 0
        stop = 1
        error = 2

    @classmethod
    def check_prerequisites(cls):
        raise NotImplementedError

    @classmethod
    def new(cls, test_case, arg):
        raise NotImplementedError

    @classmethod
    def advance(cls, test_case, arg, state):
        raise NotImplementedError

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        raise NotImplementedError

    @classmethod
    def transform(cls, test_case, arg, state):
        raise NotImplementedError

    @classmethod
    def _replace_nth_match(cls, pattern, string, n, replace_fn):
        for i, match in enumerate(re.finditer(pattern, string, re.DOTALL)):
            if i == n:
                return string[:match.start()] + replace_fn(match) + string[match.end():]

        return None
