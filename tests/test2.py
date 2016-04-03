#!/usr/bin/env python3

import subprocess
import sys

def run(test_cases):
    result = check(test_cases)

    if result:
        sys.exit(0)
    else:
        sys.exit(1)

def check(test_cases):
    test_case = test_cases[0]

    try:
        proc = subprocess.run(["clang", "-pedantic", "-Wall", "-O0", test_case], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

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

        proc = subprocess.run(["gcc", "-c", "-Wextra", "-Wall", "-O", test_case], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

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

        with open(test_case, "r") as f:
            for l in f:
                if "0x342F2529DAF1EF7ALL" in l:
                    return True

            return False
    except subprocess.CalledProcessError as err:
        return False

if __name__ == "__main__":
    run(["file1.c"])
