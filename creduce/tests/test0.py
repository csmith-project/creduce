#!/usr/bin/env python3

import subprocess
import sys

class InterestingnessTest:
    def check(self):
        raise NotImplementedError("Please use a custom interestingness test class!")

    def run(self):
        result = self.check()
        if result:
            sys.exit(0)
        else:
            sys.exit(1)

class Test0InterestingnessTest(InterestingnessTest):
    def __init__(self, test_cases):
        #FIXME: Need to use super magic?
        self.test_cases = list(test_cases)

    def check(self):
        try:
            proc = subprocess.run(["clang", "-pedantic", "-Wall", "-O0", self.test_cases[0]], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

            if ("incompatible redeclaration" in proc.stdout or
                    "ordered comparison between pointer" in proc.stdout or
                    "eliding middle term" in proc.stdout or
                    "end of non-void function" in proc.stdout or
                    "invalid in C99" in proc.stdout or
                    "specifies type" in proc.stdout or
                    "should return a value" in proc.stdout or
                    "too few argument" in proc.stdout or
                    "too many argument" in proc.stdout or
                    "return type of 'main" in proc.stdout or
                    "uninitialized" in proc.stdout or
                    "incompatible pointer to" in proc.stdout or
                    "incompatible integer to" in proc.stdout or
                    "type specifier missing" in proc.stdout):
                return False

            proc = subprocess.run(["gcc", "-c", "-Wextra", "-Wall", "-O", self.test_cases[0]], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

            if ("uninitialized" in proc.stdout or
                    "control reaches end" in proc.stdout or
                    "no semicolon at end" in proc.stdout or
                    "incompatible pointer" in proc.stdout or
                    "cast from pointer to integer" in proc.stdout or
                    "ordered comparison of pointer with integer" in proc.stdout or
                    "declaration does not declare anything" in proc.stdout or
                    "expects type" in proc.stdout or
                    "assumed to have one element" in proc.stdout or
                    "division by zero" in proc.stdout or
                    "pointer from integer" in proc.stdout or
                    "incompatible implicit" in proc.stdout or
                    "excess elements in struct initializer" in proc.stdout or
                    "comparison between pointer and integer" in proc.stdout):
                return False

            with open(self.test_cases[0], "r") as f:
                for l in f:
                    if "goto" in l:
                        return True

                return False
        except subprocess.CalledProcessError as err:
            return False

if __name__ == "__main__":
    test = Test0InterestingnessTest(["file1.c"])
    test.run()
