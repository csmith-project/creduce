import re
import enum
import logging
import copy

class BinaryState:
    def __init__(self):
        pass

    @staticmethod
    def create(instances):
        if not instances:
            return None
        self = BinaryState()
        self.instances = instances
        self.chunk = instances
        self.index = 0
        return self

    def copy(self):
        return copy.copy(self)

    def end(self):
        return min(self.index + self.chunk, self.instances)

    def advance(self):
        self = self.copy()
        original_index = self.index
        self.index += self.chunk
        if self.index >= self.instances:
            self.chunk = int(self.chunk / 2)
            if self.chunk <= 1:
                return None
            logging.debug("granularity reduced to {}".format(self.chunk))
            self.index = 0
        else:
            logging.debug("***ADVANCE*** from {} to {} with chunk {}".format(original_index, self.index, self.chunk))
        return self

    def advance_on_success(self, instances):
        if not instances:
            return None
        self.instances = instances
        if self.index >= self.instances:
            return self.advance()
        else:
            return self

class AbstractPass:
    @enum.unique
    class Result(enum.Enum):
        ok = 0
        stop = 1
        error = 2

    @enum.unique
    class Option(enum.Enum):
        slow = "slow"
        windows = "windows"

    def __init__(self, arg=None, external_programs=None):
        self.external_programs = external_programs
        self.arg = arg

    def __repr__(self):
        if self.arg is not None:
            return "{}::{}".format(type(self).__name__, self.arg)
        else:
            return "{}".format(type(self).__name__)

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
