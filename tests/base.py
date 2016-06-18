import sys

class InterestingnessTest:
    @classmethod
    def get_test_options(cls, env):
        return dict()

    def __init__(self, test_cases, options):
        self.test_cases = list(test_cases)
        self.options = options

    def check(self):
        raise NotImplementedError("Please use a custom interestingness test class!")

    def run(self):
        result = self.check()

        if result:
            sys.exit(0)
        else:
            sys.exit(1)
