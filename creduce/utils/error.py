import os

class CReduceError(Exception):
    pass

class CReduceException(Exception):
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

class ZeroSizeException(CReduceException):
    def __init__(self, test_cases):
        super().__init__()
        self.test_cases = test_cases

    def __str__(self):
        if len(self.test_cases) == 1:
            message = "The file being reduced has reached zero size; "
        else:
            message = "All files being reduced have reached zero size; "

        message += """our work here is done.

If you did not want a zero size file, you must help C-Reduce out by
making sure that your interestingness test does not find files like
this to be interesting."""
        return message

class PassBugException(CReduceException):
    MSG = """***************************************************

{}::{} has encountered a bug:
{}

Please consider tarring up {}
and mailing it to creduce-bugs@flux.utah.edu and we will try to fix the bug.

***************************************************
"""

    def __init__(self):
        super().__init__(delta_method, delta_arg, problem, crash_dir)
        self.delta_method = delta_method
        self.delta_arg = delta_arg
        self.problem = problem
        self.crash_dir = crash_dir

    def __str__(self):
        return self.MSG.format(self.delta_method, self.delta_arg, self.problem, self.crash_dir)
