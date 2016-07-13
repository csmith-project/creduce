import re
import enum

class AbstractPass:
    @enum.unique
    class Result(enum.Enum):
        ok = 0
        stop = 1
        error = 2

    @enum.unique
    class Option(enum.Enum):
        sanitize = "sanitize"
        slow = "slow"
        windows = "windows"

    def __init__(self, arg):
        self.arg = arg

    def __repr__(self):
        return "{}::{}".format(type(self).__name__, self.arg)

    def check_prerequisites(self):
        raise NotImplementedError("Class {} has not implemented 'check_prerequisites'!".format(type(self).__name__))

    def new(self, test_case):
        raise NotImplementedError("Class {} has not implemented 'new'!".format(type(self).__name__))

    def advance(self, test_case, state):
        raise NotImplementedError("Class {} has not implemented 'advance'!".format(type(self).__name__))

    def advance_on_success(self, test_case, state):
        raise NotImplementedError("Class {} has not implemented 'advance_on_success'!".format(type(self).__name__))

    def transform(self, test_case, state):
        raise NotImplementedError("Class {} has not implemented 'transform'!".format(type(self).__name__))
