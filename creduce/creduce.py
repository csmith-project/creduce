#!/usr/bin/env python3

import argparse
import filecmp
import multiprocessing
import multiprocessing.connection
import os
import re
import shutil
import subprocess
import sys
import tempfile

debug = False

class CReduceError(Exception):
    pass

class UnknownArgumentCReduceError(CReduceError):
    pass

class CReduceInvalidFileError(CReduceError):
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

class CReduceInvalidTestCaseError(CReduceInvalidFileError):
    def __str__(self):
        return "The specified test case '{}' cannot be {}!".format(self.path, self._get_error_name())

class InterestingnessTest:
    def check(self):
        raise NotImplementedError("Please use a custom interestingness test class!")

    def run(self):
        result = self.check()
        if result:
            sys.exit(0)
        else:
            sys.exit(1)

class SimpleInterestingnessTest(InterestingnessTest):
    def __init__(self, test_cases):
        #FIXME: Need to use super magic?
        self.test_cases = list(test_cases)

    def check(self):
        try:
            proc = subprocess.run(["clang", "-fsyntax-only", self.test_cases[0]], check=True)
        except subprocess.CalledProcessError:
            return False

        return True

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

class Test1InterestingnessTest(InterestingnessTest):
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
                    if "<<" in l:
                        return True

                return False
        except subprocess.CalledProcessError as err:
            return False

class Test2InterestingnessTest(InterestingnessTest):
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
                    if "0x342F2529DAF1EF7ALL" in l:
                        return True

                return False
        except subprocess.CalledProcessError as err:
            return False

class Test3InterestingnessTest(InterestingnessTest):
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
                    if r"++" in l:
                        return True

                return False
        except subprocess.CalledProcessError as err:
            return False

class Test6InterestingnessTest(InterestingnessTest):
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

            proc = subprocess.run(["gcc", "-S", "-Wextra", "-Wall", "-Ofast", "-o", "small.s", self.test_cases[0]], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

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

            with open("small.s", "r") as f:
                for l in f:
                    if "xmm" in l:
                        return True

                return False
        except subprocess.CalledProcessError as err:
            return False

class Test7InterestingnessTest(InterestingnessTest):
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

            proc = subprocess.run(["gcc", "-c", "-Wextra", "-Wall", self.test_cases[0]], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

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

            proc = subprocess.run(["gcc", "-S", "-w", "-O3", "-o", "small.s", self.test_cases[0]], universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)

            with open("small.s", "r") as f:
                for l in f:
                    if "xmm" in l:
                        return True

                return False
        except subprocess.CalledProcessError as err:
            return False

class DeltaPass:
    @classmethod
    def new(cls, test_case, arg):
        return 1
        raise NotImplementedError

    @classmethod
    def advance(cls, test_case, arg, state):
        return 1
        raise NotImplementedError

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return 1
        raise NotImplementedError

    @classmethod
    def transform(cls, test_case, arg, state):
        return (1, 1)
        raise NotImplementedError

    @classmethod
    def _replace_nth_match(cls, pattern, string, n, replace_fn):
        for i, match in enumerate(re.finditer(pattern, string, re.DOTALL)):
            if i == n and match is not None:
                return string[:match.start()] + replace_fn(match) + string[match.end():]

        return None

class IncludeIncludesDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 1

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        success = cls.__transform(test_case, state)
        return (CReduce.RES_OK if success else CReduce.RES_STOP, state)

    @classmethod
    def __transform(cls, test_case, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                includes = 0
                matched = False

                for line in in_file:
                    include_match = re.match('\s*#\s*include\s*"(.*?)"', line)

                    if include_match is not None:
                        includes += 1

                        if includes == state:
                            try:
                                with open(include_match.group(1), "r") as inc_file:
                                    matched = True
                                    tmp_file.write(inc_file.read())
                                    continue
                            except Exception:
                                pass

                    tmp_file.write(line)

            if matched:
                shutil.move(tmp_file.name, test_case)
            else:
                os.unlink(tmp_file.name)

        return matched

class IncludesDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 1

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        success = cls.__transform(test_case, state)
        return (CReduce.RES_OK if success else CReduce.RES_STOP, state)

    @classmethod
    def __transform(cls, test_case, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                includes = 0
                matched = False

                for line in in_file:
                    include_match = re.match("\s*#\s*include", line)

                    if include_match is not None:
                        includes += 1

                        if includes == state:
                            matched = True
                            continue

                    tmp_file.write(line)

            if matched:
                shutil.move(tmp_file.name, test_case)
            else:
                os.unlink(tmp_file.name)

        return matched

class UnIfDefDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        proc = subprocess.run(["unifdef", "-s", test_case], universal_newlines=True, stdout=subprocess.PIPE)

        defs = {}

        for l in proc.stdout.splitlines():
            defs[l] = 1

        deflist = list(sorted(defs.keys()))

        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            while True:
                du = "-D" if state % 2 == 0 else "-U"
                n_index = state / 2

                if n_index >= len(deflist):
                    #FIXME: No unlink in Perl script
                    os.unlink(tmp_file.name)
                    return (CReduce.RES_STOP, state)

                def_ = deflist[n_index]

                proc = subprocess.run(["unifdef", "-B", "-x", "2", "{}{}".format(du, def_), "-o", tmp_file.name, test_case], universal_newlines=True)

                if filecmp.cmp(test_case, tmp_file.name, shallow=False):
                    state += 1
                    continue

                shutil.move(tmp_file.name, test_case)
                return (CReduce.RES_OK, state)

class CommentsDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return -2

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        while True:
            if state == -2:
                replace_fn = lambda m: m.group(2) if m is not None and m.group(2) is not None else ""
                prog2 = re.sub(r"/\*[^*]*\*+([^/*][^*]*\*+)*/|(\"(\.|[^\"\\])*\"|'(\.|[^'\\])*'|.[^/\"'\\]*)", replace_fn, prog2, flags=re.DOTALL)
            elif state == -1:
                prog2 = re.sub(r"//.*$", "", prog2, flags=re.MULTILINE)
            else:
                pass

            if prog == prog2 and state == -2:
                state = -1
                continue

            if prog != prog2:
                with open(test_case, "w") as out_file:
                    out_file.write(prog2)

                return (CReduce.RES_OK, state)
            else:
                return (CReduce.RES_STOP, state)

class BlankDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @staticmethod
    def __transform(test_case, pattern):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            with open(test_case, "r") as in_file:
                matched = False

                for l in in_file:
                    if re.match(pattern, l) is not None:
                        matched = True
                    else:
                        tmp_file.write(l)

            if matched:
                shutil.move(tmp_file.name, test_case)
            else:
                os.unlink(tmp_file.name)

        return matched

    @classmethod
    def transform(cls, test_case, arg, state):
        patterns = [r"^\s*$", r"^#"]

        if state >= len(patterns):
            return (CReduce.RES_STOP, state)
        else:
            success = False

            while not success and state < len(patterns):
                success = cls.__transform(test_case, patterns[state])
                state += 1

            return (CReduce.RES_OK if success else CReduce.RES_STOP, state)

class ClangBinarySearchDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return {"start": 1}

    @classmethod
    def advance(cls, test_case, arg, state):
        if "start" in state:
            return state
        else:
            new_state = state.copy()
            new_state["index"] = state["index"] + state["chunk"]

            if debug:
                print("ADVANCE: index = {}, chunk = {}".format(new_state["index"], new_state["chunk"]))

            return new_state

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @staticmethod
    def __count_instances(test_case, arg):
        proc = subprocess.run(["clang_delta", "--query-instances={}".format(arg), test_case], universal_newlines=True, stdout=subprocess.PIPE)

        m = re.match("Available transformation instances: ([0-9]+)$", proc.stdout)

        if m is None:
            return 0
        else:
            return int(m.group(1))

    @staticmethod
    def __rechunk(state):
        if state["chunk"] < 10:
            return False

        state["chunk"] = round(float(state["chunk"]) / 2.0)
        state["index"] = 1

        if debug:
            print("granularity = {}".format(state["chunk"]))

        return True

    @classmethod
    def transform(cls, test_case, arg, state):
        new_state = state.copy()

        if "start" in new_state:
            del new_state["start"]

            instances = cls.__count_instances(test_case, arg)

            new_state["chunk"] = instances
            new_state["instances"] = instances
            new_state["index"] = 1

            if debug:
                print("intial granularity = {}".format(instances))

        while True:
            with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
                if debug:
                    print("TRANSFORM: index = {}, chunk = {}, instances = {}".format(new_state["index"], new_state["chunk"], new_state["instances"]))

                if new_state["index"] <= new_state["instances"]:
                    end = min(new_state["instances"], new_state["index"] + new_state["chunk"])
                    dec = end - new_state["index"] + 1

                    try:
                        if debug:
                            print(" ".join(["clang_delta", "--transformation={}".format(arg), "--counter={}".format(new_state["index"]), "--to-counter={}".format(end), test_case]))
                        proc = subprocess.run(["clang_delta", "--transformation={}".format(arg), "--counter={}".format(new_state["index"]), "--to-counter={}".format(end), test_case], universal_newlines=True, stdout=tmp_file)

                        if proc.returncode == 0:
                            shutil.move(tmp_file.name, test_case)
                            return (CReduce.RES_OK, new_state)
                        else:
                            if proc.returncode == 255:
                                pass
                            elif proc.returncode == 1:
                                os.unlink(tmp_file.name)
                                if debug:
                                    print("out of instances!")
                                if not cls.__rechunk(new_state):
                                    return (CReduce.RES_STOP, new_state)
                                continue
                            else:
                                os.unlink(tmp_file.name)
                                return (CReduce.RES_ERROR, new_state)

                        shutil.move(tmp_file.name, test_case)

                    except subprocess.CalledProcessError as err:
                        return (CReduce.RES_ERROR, new_state)
                else:
                    if not cls.__rechunk(new_state):
                        return (CReduce.RES_STOP, new_state)

        return (CReduce.RES_OK, new_state)

class LinesDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return {"start": 1}

    @classmethod
    def advance(cls, test_case, arg, state):
        new_state = state.copy()
        pos = new_state["index"]
        new_state["index"] -= new_state["chunk"]

        if debug:
            print("***ADVANCE*** from {} to {} with chunk {}".format(pos, new_state["index"], new_state["chunk"]))

        return new_state

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        new_state = state.copy()

        if "start" in new_state:
            del new_state["start"]

            if debug:
                print("***TRANSFORM START***")

            with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
                with open(test_case, "r") as in_file:
                    proc = subprocess.run(["topformflat", arg], stdin=in_file, stdout=subprocess.PIPE, universal_newlines=True)

                for l in proc.stdout.splitlines(keepends=True):
                    if not l.isspace():
                        tmp_file.write(l)

                shutil.move(tmp_file.name, test_case)

            with open(test_case, "r") as in_file:
                data = in_file.readlines()

            new_state["index"] = len(data)
            new_state["chunk"] = len(data)
            return (CReduce.RES_OK, new_state)
        else:
            if debug:
                print("***TRANSFORM REGULAR chunk {} at {}***".format(new_state["chunk"], new_state["index"]));

            with open(test_case, "r") as in_file:
                data = in_file.readlines()

            while True:
                new_state["index"] = min(new_state["index"], len(data))

                if new_state["index"] >= 0 and len(data) > 0 and new_state["chunk"] > 0:
                    start = max(0, new_state["index"] - new_state["chunk"])
                    chunk = new_state["chunk"]
                    old_len = len(data)
                    data = data[0:start] + data[start + chunk:]

                    if debug:
                        print("went from {} lines to {} with chunk {}".format(old_len, len(data), new_state["chunk"]))

                    with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
                        tmp_file.writelines(data)
                        shutil.move(tmp_file.name, test_case)
                        break
                else:
                    if new_state["chunk"] <= 1:
                        return (CReduce.RES_STOP, new_state)

                    new_state["chunk"] = int(float(new_state["chunk"]) / 2.0)
                    new_state["index"] = len(data)

                    if debug:
                        print("granularity reduced to {}".format(new_state["chunk"]))

            return (CReduce.RES_OK, new_state)

class SpecialDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        if arg == "a":
            replace_fn = lambda m: 'printf("%d\\n", (int){})'.format(m.group("list").split(",")[0])
            prog2 = cls._replace_nth_match(r"transparent_crc\s*\((?P<list>.*?)\)", prog2, state, replace_fn)
        elif arg == "b":
            prog2 = cls._replace_nth_match('extern "C"', prog2, state, lambda m: "")
        elif arg == "c":
            prog2 = cls._replace_nth_match('extern "C\+\+"', prog2, state, lambda m: "")
        else:
            raise UnknownArgumentCReduceError()

        if prog2 is not None and prog != prog2:
            with open(test_case, "w") as out_file:
                out_file.write(prog2)

            return (CReduce.RES_OK, state)
        else:
            return (CReduce.RES_STOP, state)

class TernaryDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        raise NotImplementedError("Balanced parenthesis cannot be matched in Python regex")
        #with open(test_case, "r") as in_file:
        #    prog = in_file.read()
        #    prog2 = prog

        #prog2 = cls._replace_nth_match(r"(?P<del1>$borderorspc)(?P<a>$varnumexp)\s*\?\s*(?P<b>$varnumexp)\s*:\s*(?P<c>$varnumexp)(?<del2>$borderorspc)", prog2, state, replace_fn)

        #if arg == "b":
        #    replace_fn = lambda m: m.group("del1") + m.group("b") + m.group("del2")
        #elif arg == "c":
        #    replace_fn = lambda m: m.group("del1") + m.group("c") + m.group("del2")
        #else:
        #    raise UnknownArgumentCReduceError()

        #if prog != prog2:
        #    with open(test_case, "w") as out_file:
        #        out_file.write(prog2)

        #    return (CReduce.RES_OK, state)
        #else:
        #    return (CReduce.RES_STOP, state)

class BalancedDeltaPass(DeltaPass):
    pass

class ClangDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 1

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            try:
                proc = subprocess.run(["clang_delta", "--transformation={}".format(arg), "--counter={}".format(state), test_case], universal_newlines=True, stdout=tmp_file)

                if proc.returncode == 0:
                    shutil.move(tmp_file.name, test_case)
                    return (CReduce.RES_OK, state)
                else:
                    os.unlink(tmp_file.name)

                    if proc.returncode == 255 or proc.returncode == 1:
                        return (CReduce.RES_STOP, state)
                    else:
                        return (CREDUCE.RES_ERROR, state)
            except subprocess.CalledProcessError as err:
                return (CREDUCE.RES_ERROR, state)

class PeepDeltaPass(DeltaPass):
    pass

class IntsDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            prog = in_file.read()
            prog2 = prog

        re_border_or_space = r"(?:(?:[*,:;{}[\]()])|\s)"

        #FIXME: Only stop if no match is found. Not if no change is made
        #FIXME: Could potentially match variable names

        while True:
            if arg == "a":
                # Delete first digit
                replace_fn = lambda m: m.group("pref") + m.group("numpart") + m.group("suf")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0}[+-]?(?:0|(0[xX]))?)[0-9a-fA-F](?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "b":
                # Delete prefix
                # FIXME: Made 0x mandatory

                replace_fn = lambda m: m.group("del") + m.group("numpart") + m.group("suf")
                prog2 = cls._replace_nth_match(r"(?P<del>{0})(?P<pref>[+-]?(?:0|(0[xX])))(?P<numpart>[0-9a-fA-F]+)(?P<suf>[ULul]*{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "c":
                # Delete suffix
                #FIXME: Changed star to plus for suffix
                replace_fn = lambda m: m.group("pref") + m.group("numpart") + m.group("del")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0}[+-]?(?:0|(0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "d":
                # Hex to dec
                replace_fn = lambda m: m.group("pref") + str(int(m.group("numpart"), 16)) + m.group("suf")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0})(?P<numpart>0[Xx][0-9a-fA-F]+)(?P<suf>[ULul]*{0})".format(re_border_or_space), prog2, state, replace_fn)
            elif arg == "e":
                #FIXME: Same as c?!
                replace_fn = lambda m: m.group("pref") + m.group("numpart") + m.group("del")
                prog2 = cls._replace_nth_match(r"(?P<pref>{0}[+-]?(?:0|(0[xX]))?)(?P<numpart>[0-9a-fA-F]+)[ULul]+(?P<del>{0})".format(re_border_or_space), prog2, state, replace_fn)
            else:
                raise UnknownArgumentCReduceError()

            if prog2 is None:
                return (CReduce.RES_STOP, state)
            else:
                if prog != prog2:
                    with open(test_case, "w") as out_file:
                        out_file.write(prog2)

                    return (CReduce.RES_OK, state)
                else:
                    state += 1

class IndentDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def transform(cls, test_case, arg, state):
        with open(test_case, "r") as in_file:
            old = in_file.read()

        while True:
            if arg == "regular":
                if state != 0:
                    return (CReduce.RES_STOP, state)
                else:
                    cmd = ["clang-format", "-i", test_case]
            elif arg == "final":
                if state == 0:
                    cmd = ["indent", "-nbad", "-nbap", "-nbbb", "-cs", "-pcs", "-prs", "-saf", "-sai", "-saw", "-sob", "-ss", test_case]
                elif state == 1:
                    cmd = ["astyle", test_case]
                elif state == 2:
                    cmd = ["clang-format", "-i", test_case]
                else:
                    return (CReduce.RES_STOP, state)

            subprocess.run(cmd, universal_newlines=True)

            with open(test_case, "r") as in_file:
                new = in_file.read()

            if old == new:
                state += 1
            else:
                break

        return (CReduce.RES_OK, state)

class ClexDeltaPass(DeltaPass):
    @classmethod
    def new(cls, test_case, arg):
        return 0

    @classmethod
    def advance(cls, test_case, arg, state):
        return state + 1

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            try:
                proc = subprocess.run(["clex", str(arg), str(state), test_case], universal_newlines=True, stdout=tmp_file)

                if proc.returncode == 51:
                    shutil.copy(tmp_file.name, test_case)
                    return (CReduce.RES_OK, state)
                else:
                    os.unlink(tmp_file.name)

                    if proc.returncode == 71:
                        return (CReduce.RES_STOP, state)
                    else:
                        return (CREDUCE.RES_ERROR, state)
            except subprocess.CalledProcessError as err:
                return (CREDUCE.RES_ERROR, state)

class CReduce:
    RES_OK = 0
    RES_STOP = 1
    RES_ERROR = 2
    default_passes = [
            {"pass": IncludeIncludesDeltaPass, "arg": "0", "pri": 100},
            {"pass": IncludesDeltaPass, "arg": "0", "first_pass_pri": 0},
            #{"pass": UnIfDefDeltaPass, "arg": "0", "pri": 450, "first_pass_pri": 0},
            {"pass": CommentsDeltaPass, "arg": "0", "pri": 451, "first_pass_pri":  0},
            {"pass": BlankDeltaPass, "arg": "0", "first_pass_pri":  1},
            {"pass": ClangBinarySearchDeltaPass, "arg": "replace-function-def-with-decl", "first_pass_pri":  2},
            {"pass": ClangBinarySearchDeltaPass, "arg": "remove-unused-function", "first_pass_pri":  3},
            {"pass": LinesDeltaPass, "arg": "0", "pri": 410, "first_pass_pri":  20, "last_pass_pri": 999},
            {"pass": LinesDeltaPass, "arg": "0", "first_pass_pri":  21},
            #{"pass": LinesDeltaPass, "arg": "0", "first_pass_pri":  22},
            {"pass": LinesDeltaPass, "arg": "1", "pri": 411, "first_pass_pri":  23},
            {"pass": LinesDeltaPass, "arg": "1", "first_pass_pri":  24},
            #{"pass": LinesDeltaPass, "arg": "1", "first_pass_pri":  25},
            {"pass": LinesDeltaPass, "arg": "2", "pri": 412, "first_pass_pri":  27},
            {"pass": LinesDeltaPass, "arg": "2", "first_pass_pri":  28},
            #{"pass": LinesDeltaPass, "arg": "2", "first_pass_pri":  29},
            {"pass": LinesDeltaPass, "arg": "10", "pri": 413, "first_pass_pri":  30},
            {"pass": LinesDeltaPass, "arg": "10", "first_pass_pri":  31},
            #{"pass": LinesDeltaPass, "arg": "10", "first_pass_pri":  32},
            {"pass": ClangBinarySearchDeltaPass, "arg": "replace-function-def-with-decl", "first_pass_pri": 33},
            {"pass": ClangBinarySearchDeltaPass, "arg": "remove-unused-function", "first_pass_pri": 34},
            {"pass": LinesDeltaPass, "arg": "0", "first_pass_pri":  35},
            {"pass": LinesDeltaPass, "arg": "1", "first_pass_pri":  36},
            {"pass": LinesDeltaPass, "arg": "2", "first_pass_pri":  37},
            {"pass": LinesDeltaPass, "arg": "10", "first_pass_pri":  38},
            {"pass": SpecialDeltaPass, "arg": "a", "first_pass_pri": 110},
            {"pass": SpecialDeltaPass, "arg": "b", "pri": 555, "first_pass_pri": 110},
            {"pass": SpecialDeltaPass, "arg": "c", "pri": 555, "first_pass_pri": 110},
            #{"pass": TernaryDeltaPass, "arg": "b", "pri": 104},
            #{"pass": TernaryDeltaPass, "arg": "c", "pri": 105},
            #{"pass": BalancedDeltaPass, "arg": "curly", "pri": 110, "first_pass_pri":  41},
            #{"pass": BalancedDeltaPass, "arg": "curly2", "pri": 111, "first_pass_pri":  42},
            #{"pass": BalancedDeltaPass, "arg": "curly3", "pri": 112, "first_pass_pri":  43},
            #{"pass": BalancedDeltaPass, "arg": "parens", "pri": 113},
            #{"pass": BalancedDeltaPass, "arg": "angles", "pri": 114},
            #{"pass": BalancedDeltaPass, "arg": "curly-only", "pri": 150},
            #{"pass": BalancedDeltaPass, "arg": "parens-only", "pri": 151},
            #{"pass": BalancedDeltaPass, "arg": "angles-only", "pri": 152},
            {"pass": ClangDeltaPass, "arg": "remove-namespace", "pri": 200},
            {"pass": ClangDeltaPass, "arg": "aggregate-to-scalar", "pri": 201},
            #{"pass": ClangDeltaPass, "arg": "binop-simplification", "pri": 201},
            {"pass": ClangDeltaPass, "arg": "local-to-global", "pri": 202},
            {"pass": ClangDeltaPass, "arg": "param-to-global", "pri": 203},
            {"pass": ClangDeltaPass, "arg": "param-to-local", "pri": 204},
            {"pass": ClangDeltaPass, "arg": "remove-nested-function", "pri": 205},
            {"pass": ClangDeltaPass, "arg": "rename-fun", "last_pass_pri": 207},
            {"pass": ClangDeltaPass, "arg": "union-to-struct", "pri": 208},
            {"pass": ClangDeltaPass, "arg": "rename-param", "last_pass_pri": 209},
            {"pass": ClangDeltaPass, "arg": "rename-var", "last_pass_pri": 210},
            {"pass": ClangDeltaPass, "arg": "rename-class", "last_pass_pri": 211},
            {"pass": ClangDeltaPass, "arg": "rename-cxx-method", "last_pass_pri": 212},
            {"pass": ClangDeltaPass, "arg": "return-void", "pri": 212},
            {"pass": ClangDeltaPass, "arg": "simple-inliner", "pri": 213},
            {"pass": ClangDeltaPass, "arg": "reduce-pointer-level", "pri": 214},
            {"pass": ClangDeltaPass, "arg": "lift-assignment-expr", "pri": 215},
            {"pass": ClangDeltaPass, "arg": "copy-propagation", "pri": 216},
            {"pass": ClangDeltaPass, "arg": "callexpr-to-value", "pri": 217, "first_pass_pri": 49},
            {"pass": ClangDeltaPass, "arg": "replace-callexpr", "pri": 218, "first_pass_pri": 50},
            {"pass": ClangDeltaPass, "arg": "simplify-callexpr", "pri": 219, "first_pass_pri": 51},
            {"pass": ClangDeltaPass, "arg": "remove-unused-function", "pri": 220, "first_pass_pri": 40},
            {"pass": ClangDeltaPass, "arg": "remove-unused-enum-member", "pri": 221, "first_pass_pri": 51},
            {"pass": ClangDeltaPass, "arg": "remove-enum-member-value", "pri": 222, "first_pass_pri": 52},
            {"pass": ClangDeltaPass, "arg": "remove-unused-var", "pri": 223, "first_pass_pri": 53},
            {"pass": ClangDeltaPass, "arg": "simplify-if", "pri": 224},
            {"pass": ClangDeltaPass, "arg": "reduce-array-dim", "pri": 225},
            {"pass": ClangDeltaPass, "arg": "reduce-array-size", "pri": 226},
            {"pass": ClangDeltaPass, "arg": "move-function-body", "pri": 227},
            {"pass": ClangDeltaPass, "arg": "simplify-comma-expr", "pri": 228},
            {"pass": ClangDeltaPass, "arg": "simplify-dependent-typedef", "pri": 229},
            {"pass": ClangDeltaPass, "arg": "replace-simple-typedef", "pri": 230},
            {"pass": ClangDeltaPass, "arg": "replace-dependent-typedef", "pri": 231},
            {"pass": ClangDeltaPass, "arg": "replace-one-level-typedef-type", "pri": 232},
            {"pass": ClangDeltaPass, "arg": "remove-unused-field", "pri": 233},
            {"pass": ClangDeltaPass, "arg": "instantiate-template-type-param-to-int", "pri": 234},
            {"pass": ClangDeltaPass, "arg": "instantiate-template-param", "pri": 235},
            {"pass": ClangDeltaPass, "arg": "template-arg-to-int", "pri": 236},
            {"pass": ClangDeltaPass, "arg": "template-non-type-arg-to-int", "pri": 237},
            {"pass": ClangDeltaPass, "arg": "reduce-class-template-param", "pri": 238},
            {"pass": ClangDeltaPass, "arg": "remove-trivial-base-template", "pri": 239},
            {"pass": ClangDeltaPass, "arg": "class-template-to-class", "pri": 240},
            {"pass": ClangDeltaPass, "arg": "remove-base-class", "pri": 241},
            {"pass": ClangDeltaPass, "arg": "replace-derived-class", "pri": 242},
            {"pass": ClangDeltaPass, "arg": "remove-unresolved-base", "pri": 243},
            {"pass": ClangDeltaPass, "arg": "remove-ctor-initializer", "pri": 244},
            {"pass": ClangDeltaPass, "arg": "replace-class-with-base-template-spec", "pri": 245},
            {"pass": ClangDeltaPass, "arg": "simplify-nested-class", "pri": 246},
            {"pass": ClangDeltaPass, "arg": "remove-unused-outer-class", "pri": 247},
            {"pass": ClangDeltaPass, "arg": "empty-struct-to-int", "pri": 248},
            {"pass": ClangDeltaPass, "arg": "remove-pointer", "pri": 249},
            {"pass": ClangDeltaPass, "arg": "remove-pointer-pairs", "pri": 250},
            {"pass": ClangDeltaPass, "arg": "remove-array", "pri": 251},
            {"pass": ClangDeltaPass, "arg": "remove-addr-taken", "pri": 252},
            {"pass": ClangDeltaPass, "arg": "simplify-struct", "pri": 253},
            {"pass": ClangDeltaPass, "arg": "replace-undefined-function", "pri": 254},
            {"pass": ClangDeltaPass, "arg": "replace-array-index-var", "pri": 255},
            {"pass": ClangDeltaPass, "arg": "replace-dependent-name", "pri": 256},
            {"pass": ClangDeltaPass, "arg": "simplify-recursive-template-instantiation", "pri": 257},
            {"pass": ClangDeltaPass, "arg": "combine-global-var", "last_pass_pri": 990},
            {"pass": ClangDeltaPass, "arg": "combine-local-var", "last_pass_pri": 991},
            {"pass": ClangDeltaPass, "arg": "simplify-struct-union-decl", "last_pass_pri": 992},
            {"pass": ClangDeltaPass, "arg": "move-global-var", "last_pass_pri": 993},
            {"pass": ClangDeltaPass, "arg": "unify-function-decl", "last_pass_pri": 994},
            #{"pass": PeepDeltaPass, "arg": "a", "pri": 500},
            {"pass": IntsDeltaPass, "arg": "a", "pri": 600},
            {"pass": IntsDeltaPass, "arg": "b", "pri": 601},
            {"pass": IntsDeltaPass, "arg": "c", "pri": 602},
            {"pass": IntsDeltaPass, "arg": "d", "pri": 603},
            {"pass": IntsDeltaPass, "arg": "e", "pri": 603},
            {"pass": IndentDeltaPass, "arg": "regular", "pri": 1000},
            {"pass": ClexDeltaPass, "arg": "delete-string", "last_pass_pri": 1001},
            {"pass": IndentDeltaPass, "arg": "final", "last_pass_pri": 9999},
            {"pass": ClexDeltaPass, "arg": "rm-toks-1", "pri": 9031},
            {"pass": ClexDeltaPass, "arg": "rm-toks-2", "pri": 9030},
            {"pass": ClexDeltaPass, "arg": "rm-toks-3", "pri": 9029},
            {"pass": ClexDeltaPass, "arg": "rm-toks-4", "pri": 9028},
            {"pass": ClexDeltaPass, "arg": "rm-toks-5", "pri": 9027},
            {"pass": ClexDeltaPass, "arg": "rm-toks-6", "pri": 9026},
            {"pass": ClexDeltaPass, "arg": "rm-toks-7", "pri": 9025},
            {"pass": ClexDeltaPass, "arg": "rm-toks-8", "pri": 9024},
            {"pass": ClexDeltaPass, "arg": "rm-toks-9", "pri": 9023},
            {"pass": ClexDeltaPass, "arg": "rm-toks-10", "pri": 9022},
            {"pass": ClexDeltaPass, "arg": "rm-toks-11", "pri": 9021},
            {"pass": ClexDeltaPass, "arg": "rm-toks-12", "pri": 9020},
            {"pass": ClexDeltaPass, "arg": "rm-toks-13", "pri": 9019},
            {"pass": ClexDeltaPass, "arg": "rm-toks-14", "pri": 9018},
            {"pass": ClexDeltaPass, "arg": "rm-toks-15", "pri": 9017},
            {"pass": ClexDeltaPass, "arg": "rm-toks-16", "pri": 9016},
    ]

    def __init__(self, interestingness_test, test_cases):
        self.itest = interestingness_test
        self.test_cases = []
        self.total_file_size = 0

        for test_case in test_cases:
            self._check_file_permissions(test_case, [os.F_OK, os.R_OK, os.W_OK], CReduceInvalidTestCaseError)
            self.test_cases.append(os.path.abspath(test_case))
            self.total_file_size += os.path.getsize(test_case)

    def reduce(self, parallel_tests, skip_initial, tidy):
        self.parallel_tests = parallel_tests
        self.orig_dir = os.getcwd()
        self.variants = []

        if not self._check_sanity():
            return False

        print("===< {} >===".format(os.getpid()))

        if not tidy:
            self._backup_files(self.test_cases)

        if not skip_initial:
            print("INITIAL PASSES")
            self._run_additional_passes("first_pass_pri")

        print("MAIN PASSES")
        self._run_main_passes()

        print("CLEANUP PASS")
        self._run_additional_passes("last_pass_pri")

        print("===================== done ====================")

        #TODO: Output statistics and reduced test cases
        return True

    @staticmethod
    def _check_file_permissions(path, modes, error):
        for m in modes:
            if not os.access(path, m):
                if error is not None:
                    raise error(path, m)
                else:
                    return False

        return True

    def _check_prerequisites(self):
        pass

    def _check_sanity(self):
        print("sanity check... ", end='')

        with tempfile.TemporaryDirectory() as tmp_dir:
            print("tmpdir = {}".format(tmp_dir))

            os.chdir(tmp_dir)
            self._copy_test_cases(tmp_dir)

            #TODO: Output error create extra dir
            result = self.itest.check()

            if result:
                print("successful")

            os.chdir(self.orig_dir)

        return result

    @staticmethod
    def _backup_files(files):
        for f in files:
            orig_file = "{}.orig".format(os.path.splitext(f)[0])

            if not os.path.exists(orig_file):
                # Copy file and preserve attributes
                shutil.copy2(f, orig_file)

    def _copy_test_cases(self, tmp_dir):
        for f in self.test_cases:
            shutil.copy(f, tmp_dir)

    def _run_additional_passes(self, priority):
        passes = CReduce.get_passes(CReduce.default_passes, priority)

        for p in passes:
            self._run_delta_pass(p["pass"], p["arg"])

    def _run_main_passes(self):
        passes = CReduce.get_passes(CReduce.default_passes, "pri")

        while True:
            for p in passes:
                self._run_delta_pass(p["pass"], p["arg"])

            total_file_size = self.get_total_file_size()

            print("Termination check: size was {}; now {}".format(self.total_file_size, total_file_size))

            if total_file_size >= self.total_file_size:
                break
            else:
                self.total_file_size = total_file_size

    def _fork_variant(self, variant_path):
        process = multiprocessing.Process(target=self.itest.run)
        process.start()
        os.setpgid(process.pid, process.pid)
        return process

    def _wait_for_results(self):
        descriptors = list(map(lambda x: x["proc"].sentinel, self.variants))
        return multiprocessing.connection.wait(descriptors)

    def _kill_variants(self):
        for v in self.variants:
            #FIXME: Does not not work on Windows
            proc = v["proc"]

            if proc.is_alive():
                os.killpg(v["proc"].pid, 15)

        self.variants = []
        self.num_running = 0

    def _run_delta_pass(self, pass_, arg):
        #TODO: Check for zero size

        print("===< {} :: {} >===".format(pass_.__name__, arg))

        for test_case in self.test_cases:
            test_case_name = os.path.basename(test_case)
            state = pass_.new(test_case_name, arg)
            again = True
            stopped = False
            self.num_running = 0

            while again:
                while not stopped and self.num_running < self.parallel_tests:
                    with tempfile.TemporaryDirectory() as tmp_dir:
                        os.chdir(tmp_dir)
                        self._copy_test_cases(tmp_dir)

                        variant_path = os.path.join(tmp_dir, test_case_name)

                        (result, state) = pass_.transform(variant_path, arg, state)

                        if result != self.RES_OK and result != self.RES_STOP:
                            #TODO: Report bug
                            pass
                        elif result == self.RES_STOP or result == self.RES_ERROR:
                            stopped = True
                        else:
                            #TODO: Report failure
                            proc = self._fork_variant(variant_path)
                            variant = {"proc" : proc, "state": state, "tmp_dir": tmp_dir, "variant_path": variant_path}
                            self.variants.append(variant)
                            self.num_running += 1
                            state = pass_.advance(variant_path, arg, state)

                        os.chdir(self.orig_dir)

                        if self.num_running > 0:
                            finished = self._wait_for_results()
                            self.num_running -= len(finished)

                        while len(self.variants) > 0:
                            variant = self.variants[0]

                            if variant["proc"].is_alive():
                                break

                            self.variants.pop(0)

                            if variant["proc"].exitcode == 0:
                                self._kill_variants()
                                state = pass_.advance_on_success(variant["variant_path"], arg, variant["state"])
                                shutil.copy(variant["variant_path"], test_case)
                                stopped = False
                                if debug:
                                    print("delta test success")
                                pct = 100 - (os.path.getsize(test_case) * 100.0 / self.total_file_size)
                                print("({} %, {} bytes)".format(round(pct, 1), os.path.getsize(test_case)))
                            else:
                                if debug:
                                    print("delta test failure")

                if stopped or len(self.variants) == 0:
                    break

    def get_total_file_size(self):
        size = 0

        for test_case in self.test_cases:
            size += os.path.getsize(test_case)

        return size

    @staticmethod
    def get_passes(passes, priority):
        passes = filter(lambda p: priority in p, passes)
        return sorted(passes, key=lambda p: p[priority])

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="C-Reduce")
    parser.add_argument("--n", type=int, default=1, help="Number of cores to use; C-Reduce tries to automatically pick a good setting but its choice may be too low or high for your situation")
    parser.add_argument("--tidy", action="store_true", default=False, help="Do not make a backup copy of each file to reduce as file.orig")
    parser.add_argument("--skip-initial-passes", action="store_true", default=False, help="Skip initial passes (useful if input is already partially reduced)")
    parser.add_argument("itest", metavar="INTERESTINGNESS_TEST", help="Executable to check interestingness of test cases")
    parser.add_argument("test_cases", metavar="TEST_CASE", nargs="+", help="Test cases")

    args = parser.parse_args()

    tests = {"test0": Test0InterestingnessTest,
             "test1": Test1InterestingnessTest,
             "test2": Test2InterestingnessTest,
             "test3": Test3InterestingnessTest,
             "test6": Test6InterestingnessTest,
             "test7": Test7InterestingnessTest}

    print(tests[args.itest].__name__)
    itest = tests[args.itest](map(os.path.basename, args.test_cases))

    reducer = CReduce(itest, args.test_cases)
    reducer.reduce(args.n, args.skip_initial_passes, args.tidy)
