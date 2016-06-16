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
        raise NotImplementedError("Class {} has not implemented 'check_prerequisites'!".format(cls.__name__))

    @classmethod
    def new(cls, test_case, arg):
        raise NotImplementedError("Class {} has not implemented 'new'!".format(cls.__name__))

    @classmethod
    def advance(cls, test_case, arg, state):
        raise NotImplementedError("Class {} has not implemented 'advance'!".format(cls.__name__))

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        raise NotImplementedError("Class {} has not implemented 'advance_on_success'!".format(cls.__name__))

    @classmethod
    def transform(cls, test_case, arg, state):
        raise NotImplementedError("Class {} has not implemented 'transform'!".format(cls.__name__))
