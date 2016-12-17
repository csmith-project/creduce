import os

class CReduceError(Exception):
    pass

class PrerequisitesNotFoundError(CReduceError):
    def __init__(self, missing):
        self.missing = missing

    def __str__(self):
        return "Missing prerequisites for passes {}!".format(", ".join(self.missing))

class UnknownArgumentError(CReduceError):
    def __init__(self, pass_, arg):
        self.pass_ = pass_
        self.arg = arg

    def __str__(self):
        return "The argument '{}' is not valid for pass '{}'!".format(self.arg, self.pass_.__name__)

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

class InvalidInterestingnessTestError(InvalidFileError):
    def __init__(self, path):
        self.path = path

    def __str__(self):
        return "The specified interestingness test '{}' cannot be executed!".format(self.path)

class ZeroSizeError(CReduceError):
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

class PassOptionError(CReduceError):
    pass

class MissingPassGroupsError(CReduceError):
    def __str__(self):
        return "Could not find a directory with definitions for pass groups!"

class PassBugError(CReduceError):
    MSG = """***************************************************

{} has encountered a bug:
{}

Please consider tarring up {}
and mailing it to creduce-bugs@flux.utah.edu and we will try to fix the bug.

***************************************************
"""

    def __init__(self, delta_pass, problem, crash_dir):
        super().__init__()
        self.delta_pass = delta_pass
        self.problem = problem
        self.crash_dir = crash_dir

    def __str__(self):
        return self.MSG.format(self.delta_pass, self.problem, self.crash_dir)

class InsaneTestCaseError(CReduceError):
    def __init__(self, test_cases, test):
        super().__init__()
        self.test_cases = test_cases
        self.test = test

    def __str__(self):
        message = """C-Reduce cannot run because the interestingness test does not return
zero. Please ensure that it does so not only in the directory where
you are invoking C-Reduce, but also in an arbitrary temporary
directory containing only the files that are being reduced. In other
words, running these commands:

  DIR=`mktemp -d`
  cp {test_cases} $DIR
  cd $DIR
  ./{test} {test_cases}
  echo $?

should result in "0" being echoed to the terminal.

See "creduce.py --help" for more information.""".format(test_cases=" ".join(self.test_cases), test=self.test)
        return message
