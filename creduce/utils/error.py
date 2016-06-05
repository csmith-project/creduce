import os

class CReduceError(Exception):
    pass

class UnknownArgumentError(CReduceError):
    pass

class InvalidFileError(CReduceError):
    def __init__(self, path, error):
        self.path = path
        self.error = error

    def _get_error_name(self):
        if self.error == os.R_OK:
            return "read"
        elif self.error == os.W_OK:
            return "written"
        elif self.error == os.X_OK:
            return "executed"
        elif self.error == os.F_OK:
            return "accessed"

    def __str__(self):
        return "The specified file '{}' cannot be {}!".format(self.path, self._get_error_name())

class InvalidTestCaseError(InvalidFileError):
    def __str__(self):
        return "The specified test case '{}' cannot be {}!".format(self.path, self._get_error_name())

