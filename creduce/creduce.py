import difflib
import enum
import filecmp
import importlib.util
import logging
import math
import multiprocessing
import multiprocessing.connection
import os
import platform
import shutil
import signal
import subprocess
import sys
import tempfile
import weakref

from .passes.delta import DeltaPass
from .passes.balanced import BalancedDeltaPass
from .passes.blank import BlankDeltaPass
from .passes.clang import ClangDeltaPass
from .passes.clangbinarysearch import ClangBinarySearchDeltaPass
from .passes.clex import ClexDeltaPass
from .passes.comments import CommentsDeltaPass
from .passes.includeincludes import IncludeIncludesDeltaPass
from .passes.includes import IncludesDeltaPass
from .passes.indent import IndentDeltaPass
from .passes.ints import IntsDeltaPass
from .passes.lines import LinesDeltaPass
from .passes.peep import PeepDeltaPass
from .passes.special import SpecialDeltaPass
from .passes.ternary import TernaryDeltaPass
from .passes.unifdef import UnIfDefDeltaPass

from .utils.error import InsaneTestCaseError
from .utils.error import InvalidTestCaseError
from .utils.error import PassBugError
from .utils.error import ZeroSizeError

def check_test(module_name, test_cases):
    module_spec = importlib.util.find_spec(module_name)
    module = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(module)
    return module.check(test_cases)

def run_test(module_name, test_cases):
    module_spec = importlib.util.find_spec(module_name)
    module = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(module)
    module.run(test_cases)

class TemporaryDirectory:
    def __init__(self, **kwargs):
        if "delete" in kwargs:
            self.delete = kwargs["delete"]
            del kwargs["delete"]
        else:
            self.delete = False

        self.name = tempfile.mkdtemp(**kwargs)

        if self.delete:
            self._finalizer = weakref.finalize(
                self, self._cleanup, self.name)

    @classmethod
    def _cleanup(cls, name):
        shutil.rmtree(name)

    def __repr__(self):
        return "<{} {!r}>".format(self.__class__.__name__, self.name)

    def __enter__(self):
        return self.name

    def __exit__(self, exc, value, tb):
        if self.delete:
            self.cleanup()

    def cleanup(self):
        if self._finalizer.detach():
            self._cleanup(self.name)

class CReduce:
    @enum.unique
    class PassOption(enum.Enum):
        sanitize = "sanitize"
        slow = "slow"
        windows = "windows"

    @enum.unique
    class PassGroup(enum.Enum):
        all = "all"
        opencl120 = "opencl1.2"
        debug = "debug"

        def __str__(self):
            return self.value

    #TODO: How can we get this information?
    PACKAGE = "TODO"
    COMMIT = "TODO"
    GIVEUP_CONSTANT = 50000
    MAX_CRASH_DIRS = 10
    MAX_EXTRA_DIRS = 25000

    groups = {PassGroup.all : {"first" : [{"pass" : IncludesDeltaPass, "arg" : "0"}, #0
                                          {"pass" : UnIfDefDeltaPass, "arg" : "0", "exclude" : {PassOption.windows}}, #0
                                  {"pass" : CommentsDeltaPass, "arg" : "0"}, #0
                                  {"pass" : BlankDeltaPass, "arg" : "0"}, #1
                                  {"pass" : ClangBinarySearchDeltaPass, "arg" : "replace-function-def-with-decl"}, #2
                                  {"pass" : ClangBinarySearchDeltaPass, "arg" : "remove-unused-function"}, #3
                                  {"pass" : LinesDeltaPass, "arg" : "0"}, #20
                                  {"pass" : LinesDeltaPass, "arg" : "0"}, #21
                                  #{"pass" : LinesDeltaPass, "arg" : "0"}, #22
                                  {"pass" : LinesDeltaPass, "arg" : "1"}, #23
                                  {"pass" : LinesDeltaPass, "arg" : "1"}, #24
                                  #{"pass" : LinesDeltaPass, "arg" : "1"}, #25
                                  {"pass" : LinesDeltaPass, "arg" : "2"}, #27
                                  {"pass" : LinesDeltaPass, "arg" : "2"}, #28
                                  #{"pass" : LinesDeltaPass, "arg" : "2"}, #29
                                  {"pass" : LinesDeltaPass, "arg" : "10"}, #30
                                  {"pass" : LinesDeltaPass, "arg" : "10"}, #31
                                  #{"pass" : LinesDeltaPass, "arg" : "10"}, #32
                                  {"pass" : ClangBinarySearchDeltaPass, "arg" : "replace-function-def-with-decl"}, #33
                                  {"pass" : ClangBinarySearchDeltaPass, "arg" : "remove-unused-function"}, #34
                                  {"pass" : LinesDeltaPass, "arg" : "0"}, #35
                                  {"pass" : LinesDeltaPass, "arg" : "1"}, #36
                                  {"pass" : LinesDeltaPass, "arg" : "2"}, #37
                                  {"pass" : LinesDeltaPass, "arg" : "10"}, #38
                                  {"pass" : ClangDeltaPass, "arg" : "remove-unused-function"}, #40
                                  {"pass" : BalancedDeltaPass, "arg" : "curly"}, #41
                                  {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #42
                                  {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #43
                                  {"pass" : ClangDeltaPass, "arg" : "callexpr-to-value"}, #49
                                  {"pass" : ClangDeltaPass, "arg" : "replace-callexpr"}, #50
                                  {"pass" : ClangDeltaPass, "arg" : "simplify-callexpr"}, #51
                                  {"pass" : ClangDeltaPass, "arg" : "remove-unused-enum-member"}, #51
                                  {"pass" : ClangDeltaPass, "arg" : "remove-enum-member-value"}, #52
                                  {"pass" : ClangDeltaPass, "arg" : "remove-unused-var"}, #53
                                  {"pass" : SpecialDeltaPass, "arg" : "a"}, #110
                                  {"pass" : SpecialDeltaPass, "arg" : "b"}, #110
                                  {"pass" : SpecialDeltaPass, "arg" : "c"}, #110
                                 ],
                       "main" : [{"pass" : IncludeIncludesDeltaPass, "arg" : "0"}, #100
                                 {"pass" : TernaryDeltaPass, "arg" : "b"}, #104
                                 {"pass" : TernaryDeltaPass, "arg" : "c"}, #105
                                 {"pass" : BalancedDeltaPass, "arg" : "curly"}, #110
                                 {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #111
                                 {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #112
                                 {"pass" : BalancedDeltaPass, "arg" : "parens"}, #113
                                 {"pass" : BalancedDeltaPass, "arg" : "angles"}, #114
                                 {"pass" : BalancedDeltaPass, "arg" : "square"}, #115
                                 {"pass" : BalancedDeltaPass, "arg" : "curly-inside"}, #150
                                 {"pass" : BalancedDeltaPass, "arg" : "parens-inside"}, #151
                                 {"pass" : BalancedDeltaPass, "arg" : "angles-inside"}, #152
                                 {"pass" : BalancedDeltaPass, "arg" : "square-inside"}, #153
                                 {"pass" : BalancedDeltaPass, "arg" : "curly-only"}, #160
                                 {"pass" : BalancedDeltaPass, "arg" : "parens-only"}, #161
                                 {"pass" : BalancedDeltaPass, "arg" : "angles-only"}, #162
                                 {"pass" : BalancedDeltaPass, "arg" : "square-only"}, #163
                                 {"pass" : ClangDeltaPass, "arg" : "remove-namespace"}, #200
                                 {"pass" : ClangDeltaPass, "arg" : "aggregate-to-scalar"}, #201
                                 #{"pass" : (ClangDeltaPass, "arg" : "binop-simplification"}, #201
                                 {"pass" : ClangDeltaPass, "arg" : "local-to-global"}, #202
                                 {"pass" : ClangDeltaPass, "arg" : "param-to-global"}, #203
                                 {"pass" : ClangDeltaPass, "arg" : "param-to-local"}, #204
                                 {"pass" : ClangDeltaPass, "arg" : "remove-nested-function"}, #205
                                 {"pass" : ClangDeltaPass, "arg" : "union-to-struct"}, #208
                                 {"pass" : ClangDeltaPass, "arg" : "return-void"}, #212
                                 {"pass" : ClangDeltaPass, "arg" : "simple-inliner"}, #213
                                 {"pass" : ClangDeltaPass, "arg" : "reduce-pointer-level"}, #214
                                 {"pass" : ClangDeltaPass, "arg" : "lift-assignment-expr"}, #215
                                 {"pass" : ClangDeltaPass, "arg" : "copy-propagation"}, #216
                                 {"pass" : ClangDeltaPass, "arg" : "callexpr-to-value"}, #217
                                 {"pass" : ClangDeltaPass, "arg" : "replace-callexpr"}, #218
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-callexpr"}, #219
                                 {"pass" : ClangDeltaPass, "arg" : "remove-unused-function"}, #220
                                 {"pass" : ClangDeltaPass, "arg" : "remove-unused-enum-member"}, #221
                                 {"pass" : ClangDeltaPass, "arg" : "remove-enum-member-value"}, #222
                                 {"pass" : ClangDeltaPass, "arg" : "remove-unused-var"}, #223
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-if"}, #224
                                 {"pass" : ClangDeltaPass, "arg" : "reduce-array-dim"}, #225
                                 {"pass" : ClangDeltaPass, "arg" : "reduce-array-size"}, #226
                                 {"pass" : ClangDeltaPass, "arg" : "move-function-body"}, #227
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-comma-expr"}, #228
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-dependent-typedef"}, #229
                                 {"pass" : ClangDeltaPass, "arg" : "replace-simple-typedef"}, #230
                                 {"pass" : ClangDeltaPass, "arg" : "replace-dependent-typedef"}, #231
                                 {"pass" : ClangDeltaPass, "arg" : "replace-one-level-typedef-type"}, #232
                                 {"pass" : ClangDeltaPass, "arg" : "remove-unused-field"}, #233
                                 {"pass" : ClangDeltaPass, "arg" : "instantiate-template-type-param-to-int"}, #234
                                 {"pass" : ClangDeltaPass, "arg" : "instantiate-template-param"}, #235
                                 {"pass" : ClangDeltaPass, "arg" : "template-arg-to-int"}, #236
                                 {"pass" : ClangDeltaPass, "arg" : "template-non-type-arg-to-int"}, #237
                                 {"pass" : ClangDeltaPass, "arg" : "reduce-class-template-param"}, #238
                                 {"pass" : ClangDeltaPass, "arg" : "remove-trivial-base-template"}, #239
                                 {"pass" : ClangDeltaPass, "arg" : "class-template-to-class"}, #240
                                 {"pass" : ClangDeltaPass, "arg" : "remove-base-class"}, #241
                                 {"pass" : ClangDeltaPass, "arg" : "replace-derived-class"}, #242
                                 {"pass" : ClangDeltaPass, "arg" : "remove-unresolved-base"}, #243
                                 {"pass" : ClangDeltaPass, "arg" : "remove-ctor-initializer"}, #244
                                 {"pass" : ClangDeltaPass, "arg" : "replace-class-with-base-template-spec"}, #245
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-nested-class"}, #246
                                 {"pass" : ClangDeltaPass, "arg" : "remove-unused-outer-class"}, #247
                                 {"pass" : ClangDeltaPass, "arg" : "empty-struct-to-int"}, #248
                                 {"pass" : ClangDeltaPass, "arg" : "remove-pointer"}, #249
                                 {"pass" : ClangDeltaPass, "arg" : "reduce-pointer-pairs"}, #250
                                 {"pass" : ClangDeltaPass, "arg" : "remove-array"}, #251
                                 {"pass" : ClangDeltaPass, "arg" : "remove-addr-taken"}, #252
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-struct"}, #253
                                 {"pass" : ClangDeltaPass, "arg" : "replace-undefined-function"}, #254
                                 {"pass" : ClangDeltaPass, "arg" : "replace-array-index-var"}, #255
                                 {"pass" : ClangDeltaPass, "arg" : "replace-array-access-with-index"}, #256
                                 {"pass" : ClangDeltaPass, "arg" : "replace-dependent-name"}, #257
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-recursive-template-instantiation"}, #258
                                 {"pass" : LinesDeltaPass, "arg" : "0"}, #410
                                 {"pass" : LinesDeltaPass, "arg" : "1"}, #411
                                 {"pass" : LinesDeltaPass, "arg" : "2"}, #412
                                 {"pass" : LinesDeltaPass, "arg" : "10"}, #413
                                 {"pass" : UnIfDefDeltaPass, "arg" : "0", "exclude" : {PassOption.windows}}, #450
                                 {"pass" : CommentsDeltaPass, "arg" : "0"}, #451
                                 {"pass" : PeepDeltaPass, "arg" : "a"}, #500
                                 {"pass" : SpecialDeltaPass, "arg" : "b"}, #555
                                 {"pass" : SpecialDeltaPass, "arg" : "c"}, #555
                                 {"pass" : IntsDeltaPass, "arg" : "a"}, #600
                                 {"pass" : IntsDeltaPass, "arg" : "b"}, #601
                                 {"pass" : IntsDeltaPass, "arg" : "c"}, #602
                                 {"pass" : IntsDeltaPass, "arg" : "d"}, #603
                                 {"pass" : IntsDeltaPass, "arg" : "e"}, #603
                                 {"pass" : IndentDeltaPass, "arg" : "regular"}, #1000
                                 {"pass" : ClexDeltaPass, "arg" : "delete-string", "include" : {PassOption.sanitize}}, #1001
                                 {"pass" : ClexDeltaPass, "arg" : "remove-asm-line", "include" : {PassOption.sanitize}}, #1002
                                 {"pass" : ClexDeltaPass, "arg" : "remove-asm-comment", "include" : {PassOption.sanitize}}, #1003
                                 {"pass" : ClexDeltaPass, "arg" : "shorten-string", "include" : {PassOption.sanitize}}, #1010
                                 {"pass" : ClexDeltaPass, "arg" : "x-string", "include" : {PassOption.sanitize}}, #1011
                                 #{"pass" : (ClexDeltaPass, "arg" : "collapse-toks", "include" : {PassOption.sanitize}}, #5000
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-32", "include" : {PassOption.slow}}, #9000
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-31", "include" : {PassOption.slow}}, #9001
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-30", "include" : {PassOption.slow}}, #9002
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-29", "include" : {PassOption.slow}}, #9003
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-28", "include" : {PassOption.slow}}, #9004
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-27", "include" : {PassOption.slow}}, #9005
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-26", "include" : {PassOption.slow}}, #9006
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-25", "include" : {PassOption.slow}}, #9007
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-24", "include" : {PassOption.slow}}, #9008
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-23", "include" : {PassOption.slow}}, #9009
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-22", "include" : {PassOption.slow}}, #9010
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-21", "include" : {PassOption.slow}}, #9011
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-20", "include" : {PassOption.slow}}, #9012
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-19", "include" : {PassOption.slow}}, #9013
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-18", "include" : {PassOption.slow}}, #9014
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-17", "include" : {PassOption.slow}}, #9015
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-16"}, #9016
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-15"}, #9017
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-14"}, #9018
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-13"}, #9019
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-12"}, #9020
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-11"}, #9021
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-10"}, #9022
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-9"}, #9023
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-8"}, #9024
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-7"}, #9025
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-6"}, #9026
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-5"}, #9027
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-4"}, #9028
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-3"}, #9029
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-2"}, #9030
                                 {"pass" : ClexDeltaPass, "arg" : "rm-toks-1"}, #9031
                                 {"pass" : ClexDeltaPass, "arg" : "rm-tok-pattern-8", "include" : {PassOption.slow}}, #9100
                                 {"pass" : ClexDeltaPass, "arg" : "rm-tok-pattern-4", "exclude" : {PassOption.slow}}, #9100
                                 {"pass" : PeepDeltaPass, "arg" : "b", "include" : {PassOption.slow}}, #9500
                                ],
                       "last" : [{"pass" : ClangDeltaPass, "arg" : "rename-fun"}, #207
                                 {"pass" : ClangDeltaPass, "arg" : "rename-param"}, #209
                                 {"pass" : ClangDeltaPass, "arg" : "rename-var"}, #210
                                 {"pass" : ClangDeltaPass, "arg" : "rename-class"}, #211
                                 {"pass" : ClangDeltaPass, "arg" : "rename-cxx-method"}, #212
                                 {"pass" : ClangDeltaPass, "arg" : "combine-global-var"}, #990
                                 {"pass" : ClangDeltaPass, "arg" : "combine-local-var"}, #991
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-struct-union-decl"}, #992
                                 {"pass" : ClangDeltaPass, "arg" : "move-global-var"}, #993
                                 {"pass" : ClangDeltaPass, "arg" : "unify-function-decl"}, #994
                                 {"pass" : LinesDeltaPass, "arg" : "0"}, #999
                                 {"pass" : ClexDeltaPass, "arg" : "rename-toks", "include" : {PassOption.sanitize}}, #1000
                                 {"pass" : ClexDeltaPass, "arg" : "delete-string"}, #1001
                                 {"pass" : IndentDeltaPass, "arg" : "final"}, #9999
                                ]
                      },
              PassGroup.opencl120 : {"first" : [{"pass" : IncludesDeltaPass, "arg" : "0"}, #0
                                        {"pass" : UnIfDefDeltaPass, "arg" : "0", "exclude" : {PassOption.windows}}, #0
                                        {"pass" : CommentsDeltaPass, "arg" : "0"}, #0
                                        {"pass" : BlankDeltaPass, "arg" : "0"}, #1
                                        {"pass" : ClangBinarySearchDeltaPass, "arg" : "replace-function-def-with-decl"}, #2
                                        {"pass" : ClangBinarySearchDeltaPass, "arg" : "remove-unused-function"}, #3
                                        {"pass" : LinesDeltaPass, "arg" : "0"}, #20
                                        {"pass" : LinesDeltaPass, "arg" : "0"}, #21
                                        #{"pass" : LinesDeltaPass, "arg" : "0"}, #22
                                        {"pass" : LinesDeltaPass, "arg" : "1"}, #23
                                        {"pass" : LinesDeltaPass, "arg" : "1"}, #24
                                        #{"pass" : LinesDeltaPass, "arg" : "1"}, #25
                                        {"pass" : LinesDeltaPass, "arg" : "2"}, #27
                                        {"pass" : LinesDeltaPass, "arg" : "2"}, #28
                                        #{"pass" : LinesDeltaPass, "arg" : "2"}, #29
                                        {"pass" : LinesDeltaPass, "arg" : "10"}, #30
                                        {"pass" : LinesDeltaPass, "arg" : "10"}, #31
                                        #{"pass" : LinesDeltaPass, "arg" : "10"}, #32
                                        {"pass" : ClangBinarySearchDeltaPass, "arg" : "replace-function-def-with-decl"}, #33
                                        {"pass" : ClangBinarySearchDeltaPass, "arg" : "remove-unused-function"}, #34
                                        {"pass" : LinesDeltaPass, "arg" : "0"}, #35
                                        {"pass" : LinesDeltaPass, "arg" : "1"}, #36
                                        {"pass" : LinesDeltaPass, "arg" : "2"}, #37
                                        {"pass" : LinesDeltaPass, "arg" : "10"}, #38
                                        {"pass" : ClangDeltaPass, "arg" : "remove-unused-function"}, #40
                                        {"pass" : BalancedDeltaPass, "arg" : "curly"}, #41
                                        {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #42
                                        {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #43
                                        {"pass" : ClangDeltaPass, "arg" : "callexpr-to-value"}, #49
                                        {"pass" : ClangDeltaPass, "arg" : "replace-callexpr"}, #50
                                        {"pass" : ClangDeltaPass, "arg" : "simplify-callexpr"}, #51
                                        {"pass" : ClangDeltaPass, "arg" : "remove-unused-enum-member"}, #51
                                        {"pass" : ClangDeltaPass, "arg" : "remove-enum-member-value"}, #52
                                        {"pass" : ClangDeltaPass, "arg" : "remove-unused-var"}, #53
                                        {"pass" : SpecialDeltaPass, "arg" : "a"}, #110
                                        {"pass" : SpecialDeltaPass, "arg" : "b"}, #110
                                        {"pass" : SpecialDeltaPass, "arg" : "c"}, #110
                                       ],
                             "main" : [{"pass" : IncludeIncludesDeltaPass, "arg" : "0"}, #100
                                       {"pass" : TernaryDeltaPass, "arg" : "b"}, #104
                                       {"pass" : TernaryDeltaPass, "arg" : "c"}, #105
                                       {"pass" : BalancedDeltaPass, "arg" : "curly"}, #110
                                       {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #111
                                       {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #112
                                       {"pass" : BalancedDeltaPass, "arg" : "parens"}, #113
                                       {"pass" : BalancedDeltaPass, "arg" : "angles"}, #114
                                       {"pass" : BalancedDeltaPass, "arg" : "square"}, #115
                                       {"pass" : BalancedDeltaPass, "arg" : "curly-inside"}, #150
                                       {"pass" : BalancedDeltaPass, "arg" : "parens-inside"}, #151
                                       {"pass" : BalancedDeltaPass, "arg" : "angles-inside"}, #152
                                       {"pass" : BalancedDeltaPass, "arg" : "square-inside"}, #153
                                       {"pass" : BalancedDeltaPass, "arg" : "curly-only"}, #160
                                       {"pass" : BalancedDeltaPass, "arg" : "parens-only"}, #161
                                       {"pass" : BalancedDeltaPass, "arg" : "angles-only"}, #162
                                       {"pass" : BalancedDeltaPass, "arg" : "square-only"}, #163
                                       {"pass" : ClangDeltaPass, "arg" : "aggregate-to-scalar"}, #201
                                       #{"pass" : (ClangDeltaPass, "arg" : "binop-simplification"}, #201
                                       {"pass" : ClangDeltaPass, "arg" : "param-to-local"}, #204
                                       {"pass" : ClangDeltaPass, "arg" : "union-to-struct"}, #208
                                       {"pass" : ClangDeltaPass, "arg" : "return-void"}, #212
                                       {"pass" : ClangDeltaPass, "arg" : "simple-inliner"}, #213
                                       {"pass" : ClangDeltaPass, "arg" : "reduce-pointer-level"}, #214
                                       {"pass" : ClangDeltaPass, "arg" : "lift-assignment-expr"}, #215
                                       {"pass" : ClangDeltaPass, "arg" : "copy-propagation"}, #216
                                       {"pass" : ClangDeltaPass, "arg" : "callexpr-to-value"}, #217
                                       {"pass" : ClangDeltaPass, "arg" : "replace-callexpr"}, #218
                                       {"pass" : ClangDeltaPass, "arg" : "simplify-callexpr"}, #219
                                       {"pass" : ClangDeltaPass, "arg" : "remove-unused-function"}, #220
                                       {"pass" : ClangDeltaPass, "arg" : "remove-unused-enum-member"}, #221
                                       {"pass" : ClangDeltaPass, "arg" : "remove-enum-member-value"}, #222
                                       {"pass" : ClangDeltaPass, "arg" : "remove-unused-var"}, #223
                                       {"pass" : ClangDeltaPass, "arg" : "simplify-if"}, #224
                                       {"pass" : ClangDeltaPass, "arg" : "reduce-array-dim"}, #225
                                       {"pass" : ClangDeltaPass, "arg" : "reduce-array-size"}, #226
                                       {"pass" : ClangDeltaPass, "arg" : "move-function-body"}, #227
                                       {"pass" : ClangDeltaPass, "arg" : "simplify-comma-expr"}, #228
                                       {"pass" : ClangDeltaPass, "arg" : "simplify-dependent-typedef"}, #229
                                       {"pass" : ClangDeltaPass, "arg" : "replace-simple-typedef"}, #230
                                       {"pass" : ClangDeltaPass, "arg" : "replace-dependent-typedef"}, #231
                                       {"pass" : ClangDeltaPass, "arg" : "replace-one-level-typedef-type"}, #232
                                       {"pass" : ClangDeltaPass, "arg" : "remove-unused-field"}, #233
                                       {"pass" : ClangDeltaPass, "arg" : "empty-struct-to-int"}, #248
                                       {"pass" : ClangDeltaPass, "arg" : "remove-pointer"}, #249
                                       {"pass" : ClangDeltaPass, "arg" : "reduce-pointer-pairs"}, #250
                                       {"pass" : ClangDeltaPass, "arg" : "remove-array"}, #251
                                       {"pass" : ClangDeltaPass, "arg" : "remove-addr-taken"}, #252
                                       {"pass" : ClangDeltaPass, "arg" : "simplify-struct"}, #253
                                       {"pass" : ClangDeltaPass, "arg" : "replace-undefined-function"}, #254
                                       {"pass" : ClangDeltaPass, "arg" : "replace-array-index-var"}, #255
                                       {"pass" : ClangDeltaPass, "arg" : "replace-array-access-with-index"}, #256
                                       {"pass" : ClangDeltaPass, "arg" : "replace-dependent-name"}, #257
                                       {"pass" : LinesDeltaPass, "arg" : "0"}, #410
                                       {"pass" : LinesDeltaPass, "arg" : "1"}, #411
                                       {"pass" : LinesDeltaPass, "arg" : "2"}, #412
                                       {"pass" : LinesDeltaPass, "arg" : "10"}, #413
                                       {"pass" : UnIfDefDeltaPass, "arg" : "0", "exclude" : {PassOption.windows}}, #450
                                       {"pass" : CommentsDeltaPass, "arg" : "0"}, #451
                                       {"pass" : PeepDeltaPass, "arg" : "a"}, #500
                                       {"pass" : SpecialDeltaPass, "arg" : "b"}, #555
                                       {"pass" : SpecialDeltaPass, "arg" : "c"}, #555
                                       {"pass" : IntsDeltaPass, "arg" : "a"}, #600
                                       {"pass" : IntsDeltaPass, "arg" : "b"}, #601
                                       {"pass" : IntsDeltaPass, "arg" : "c"}, #602
                                       {"pass" : IntsDeltaPass, "arg" : "d"}, #603
                                       {"pass" : IntsDeltaPass, "arg" : "e"}, #603
                                       {"pass" : IndentDeltaPass, "arg" : "regular"}, #1000
                                       {"pass" : ClexDeltaPass, "arg" : "delete-string", "include" : {PassOption.sanitize}}, #1001
                                       {"pass" : ClexDeltaPass, "arg" : "shorten-string", "include" : {PassOption.sanitize}}, #1010
                                       {"pass" : ClexDeltaPass, "arg" : "x-string", "include" : {PassOption.sanitize}}, #1011
                                       #{"pass" : (ClexDeltaPass, "arg" : "collapse-toks", "include" : {PassOption.sanitize}}, #5000
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-32", "include" : {PassOption.slow}}, #9000
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-31", "include" : {PassOption.slow}}, #9001
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-30", "include" : {PassOption.slow}}, #9002
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-29", "include" : {PassOption.slow}}, #9003
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-28", "include" : {PassOption.slow}}, #9004
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-27", "include" : {PassOption.slow}}, #9005
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-26", "include" : {PassOption.slow}}, #9006
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-25", "include" : {PassOption.slow}}, #9007
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-24", "include" : {PassOption.slow}}, #9008
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-23", "include" : {PassOption.slow}}, #9009
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-22", "include" : {PassOption.slow}}, #9010
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-21", "include" : {PassOption.slow}}, #9011
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-20", "include" : {PassOption.slow}}, #9012
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-19", "include" : {PassOption.slow}}, #9013
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-18", "include" : {PassOption.slow}}, #9014
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-17", "include" : {PassOption.slow}}, #9015
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-16"}, #9016
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-15"}, #9017
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-14"}, #9018
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-13"}, #9019
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-12"}, #9020
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-11"}, #9021
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-10"}, #9022
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-9"}, #9023
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-8"}, #9024
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-7"}, #9025
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-6"}, #9026
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-5"}, #9027
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-4"}, #9028
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-3"}, #9029
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-2"}, #9030
                                       {"pass" : ClexDeltaPass, "arg" : "rm-toks-1"}, #9031
                                       {"pass" : ClexDeltaPass, "arg" : "rm-tok-pattern-8", "include" : {PassOption.slow}}, #9100
                                       {"pass" : ClexDeltaPass, "arg" : "rm-tok-pattern-4", "exclude" : {PassOption.slow}}, #9100
                                       {"pass" : PeepDeltaPass, "arg" : "b", "include" : {PassOption.slow}}, #9500
                                      ],
                             "last" : [{"pass" : ClangDeltaPass, "arg" : "rename-fun"}, #207
                                       {"pass" : ClangDeltaPass, "arg" : "rename-param"}, #209
                                       {"pass" : ClangDeltaPass, "arg" : "rename-var"}, #210
                                       {"pass" : ClangDeltaPass, "arg" : "combine-local-var"}, #991
                                       {"pass" : ClangDeltaPass, "arg" : "simplify-struct-union-decl"}, #992
                                       {"pass" : ClangDeltaPass, "arg" : "unify-function-decl"}, #994
                                       {"pass" : LinesDeltaPass, "arg" : "0"}, #999
                                       {"pass" : ClexDeltaPass, "arg" : "rename-toks", "include" : {PassOption.sanitize}}, #1000
                                       {"pass" : ClexDeltaPass, "arg" : "delete-string"}, #1001
                                       {"pass" : IndentDeltaPass, "arg" : "final"}, #9999
                                      ]
                            },
    }

    def __init__(self, test_module_name, test_cases):
        self.test_module_name = test_module_name
        self.test_cases = []
        self.total_file_size = 0
        self.orig_total_file_size = 0
        self.tidy = False
        self.silent_pass_bug = False
        self.die_on_pass_bug = False
        self.also_interesting = -1
        self.no_kill = False
        self.no_setpgrp = False
        self.no_give_up = False
        self.print_diff = False
        self.save_temps = False
        self.max_improvement = None

        for test_case in test_cases:
            self._check_file_permissions(test_case, [os.F_OK, os.R_OK, os.W_OK], InvalidTestCaseError)
            self.test_cases.append(os.path.abspath(test_case))
            self.total_file_size += os.path.getsize(test_case)

        self.orig_total_file_size = self.total_file_size

    def reduce(self, parallel_tests, skip_initial=False, pass_group=PassGroup.all, pass_options=set()):
        self.__parallel_tests = parallel_tests
        self.__orig_dir = os.getcwd()
        self.__statistics = {}

        if platform.system() == "Windows":
            pass_options.add(self.PassOption.windows)

        pass_group = self._prepare_pass_group(pass_group, pass_options)
        missing = self._check_prerequisites(pass_group)

        if missing is not None:
            logging.error("Prereqs not found for pass {}".format(missing))
            return False

        if not self._check_sanity():
            return False

        logging.info("===< {} >===".format(os.getpid()))
        logging.info("running {} interestingness test{} in parallel".format(self.__parallel_tests,
                                                                            "" if self.__parallel_tests == 1 else "s"))

        if not self.tidy:
            self._backup_files(self.test_cases)

        self.__statistics = self._init_pass_statistics(pass_group)

        if not skip_initial:
            logging.info("INITIAL PASSES")
            self._run_additional_passes(pass_group["first"])

        logging.info("MAIN PASSES")
        self._run_main_passes(pass_group["main"])

        logging.info("CLEANUP PASS")
        self._run_additional_passes(pass_group["last"])

        self.total_file_size = self._get_total_file_size()

        logging.info("===================== done ====================")

        #FIXME: This should go to the commandline script!
        print("pass statistics:")

        def sort_statistics(item):
            return (-item["worked"], str(item["pass"]), item["arg"])

        sorted_statistics = sorted(self.__statistics.values(), key=sort_statistics)

        for item in sorted_statistics:
            print("method {pass} :: {arg} worked {worked} times and failed {failed} times".format(**item))

        def sort_test_cases(test_case):
            return os.path.getsize(test_case)

        sorted_test_cases = sorted(self.test_cases, key=sort_test_cases)

        for test_case in sorted_test_cases:
            with open(test_case, mode="r") as test_case_file:
                print(test_case_file.read())

        return True

    @staticmethod
    def _generate_statistics_key(pass_, arg):
        return str(pass_) + str(arg)

    @staticmethod
    def _init_pass_statistics(pass_group):
        stats = {}

        for category in pass_group:
            for p in pass_group[category]:
                key = CReduce._generate_statistics_key(p["pass"], p["arg"])
                stats[key] = {"pass" : p["pass"],
                              "arg" : p["arg"],
                              "worked" : 0,
                              "failed" : 0}

        return stats

    def _update_pass_statistics(self, pass_, arg, success):
        key = self._generate_statistics_key(pass_, arg)

        if success:
            self.__statistics[key]["worked"] += 1
        else:
            self.__statistics[key]["failed"] += 1

    @staticmethod
    def _check_file_permissions(path, modes, error):
        for m in modes:
            if not os.access(path, m):
                if error is not None:
                    raise error(path, m)
                else:
                    return False

        return True

    @staticmethod
    def _check_prerequisites(pass_group):
        passes = set()

        for category in pass_group:
            passes |= set(map(lambda p: p["pass"], pass_group[category]))

        for p in passes:
            if not p.check_prerequisites():
                return p

        return None

    def _check_sanity(self):
        logging.debug("perform sanity check... ")

        with TemporaryDirectory(prefix="creduce-", delete=(not self.save_temps)) as tmp_dir_name:
            logging.debug("sanity check tmpdir = {}".format(tmp_dir_name))

            os.chdir(tmp_dir_name)
            self._copy_test_cases(tmp_dir_name)

            result = check_test(self.test_module_name, self.test_cases)

            if result:
                logging.debug("sanity check successful")
                os.chdir(self.__orig_dir)
                return True
            else:
                #FIXME: Pass test invocation to exception
                raise InsaneTestCaseError(self.test_cases, "TODO")

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

    def _run_additional_passes(self, passes):
        for p in passes:
            self._run_delta_pass(p["pass"], p["arg"])

    def _run_main_passes(self, passes):
        while True:
            for p in passes:
                self._run_delta_pass(p["pass"], p["arg"])

            total_file_size = self._get_total_file_size()

            logging.info("Termination check: size was {}; now {}".format(self.total_file_size, total_file_size))

            if total_file_size >= self.total_file_size:
                break
            else:
                self.total_file_size = total_file_size

    def _create_variant(self, variant_path):
        process = multiprocessing.Process(target=run_test, args=(self.test_module_name, [variant_path]))
        process.start()

        if not self.no_setpgrp and platform.system() != "Windows":
            os.setpgid(process.pid, process.pid)

        return process

    @staticmethod
    def _wait_for_results(variants):
        descriptors = [v["proc"].sentinel for v in variants if v["proc"].is_alive()]

        # On Windows it is only possible to wait on 64 processes
        if platform.system() == "Windows":
            descriptors = descriptors[0:64]

        #logging.warning("Waiting for {}".format(descriptors))

        # If all processes have already ended do not wait
        if not descriptors:
            return descriptors

        return multiprocessing.connection.wait(descriptors)

    def _kill_variants(self, variants):
        for v in variants:
            proc = v["proc"]

            #logging.warning("Kill {}".format(v["proc"].sentinel))

            if proc.is_alive() and not self.no_kill:
                if platform.system() == "Windows":
                    subprocess.run(["TASKKILL", "/F", "/T", "/PID", str(proc.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                elif self.no_setpgrp:
                    os.kill(proc.pid, signal.SIGTERM)
                else:
                    os.killpg(proc.pid, signal.SIGTERM)

            proc.join()

        # Performs implicit cleanup of the temporary directories
        variants.clear()

    @staticmethod
    def _get_running_variants(variants):
        return [v for v in variants if v["proc"].is_alive()]

    @staticmethod
    def _get_successful_variants(variants):
        return [v for v in variants if not v["proc"].is_alive() and v["proc"].exitcode == 0]

    @staticmethod
    def _has_successful_variant(variants):
        return any(v["proc"].exitcode == 0 for v in variants if not v["proc"].is_alive())

    def _run_delta_pass(self, pass_, arg):
        logging.info("===< {} :: {} >===".format(pass_.__name__, arg))

        if self._get_total_file_size() == 0:
            raise ZeroSizeError(self.test_cases)

        for test_case in self.test_cases:
            state = pass_.new(test_case, arg)
            stopped = False
            since_success = 0
            variants = []

            while True:
                # Create new variants and launch tests as long as:
                # (a) there has been no error and the transformation space is not exhausted,
                # (b) the test fot the first variant in the list is still running,
                # (c) the maximum number of parallel test instances has not been reached, and
                # (d) no earlier variant has already been successful (don't waste resources)
                while (not stopped and
                       (not variants or variants[0]["proc"].is_alive()) and
                       len(self._get_running_variants(variants)) < self.__parallel_tests and
                       not self._has_successful_variant(variants)):
                    tmp_dir = TemporaryDirectory(prefix="creduce-", delete=(not self.save_temps))

                    os.chdir(tmp_dir.name)
                    self._copy_test_cases(tmp_dir.name)

                    variant_path = os.path.join(tmp_dir.name, os.path.basename(test_case))

                    (result, state) = pass_.transform(variant_path, arg, state)

                    if result != DeltaPass.Result.ok and result != DeltaPass.Result.stop:
                        if not self.silent_pass_bug:
                            self._report_pass_bug(pass_, arg, state if result == DeltaPass.Result.error else "unknown return code")

                    if result == DeltaPass.Result.stop or result == DeltaPass.Result.error:
                        stopped = True
                    else:
                        if self.print_diff:
                            diff_str = self._diff_files(test_case, variant_path)
                            #TODO: Can we print somehow different?
                            print(diff_str)

                        # Report bug if transform did not change the file
                        if filecmp.cmp(test_case, variant_path):
                            if not self.silent_pass_bug:
                                self._report_pass_bug(pass_, arg, "pass failed to modify the variant")

                            stopped = True
                        else:
                            proc = self._create_variant(variant_path)
                            variant = {"proc": proc, "state": state, "tmp_dir": tmp_dir, "variant_path": variant_path}
                            #logging.warning("Fork {}".format(proc.sentinel))
                            variants.append(variant)
                            #logging.warning("forked {}, num_running = {}, variants = {}".format(proc.pid, len(self._get_running_variants(variants)), len(variants)))
                            state = pass_.advance(test_case, arg, state)

                    os.chdir(self.__orig_dir)

                #logging.debug("parent is waiting")
                # Only wait if the first variant is not ready yet
                if variants and variants[0]["proc"].is_alive():
                    self._wait_for_results(variants)
                #logging.warning("Processes finished")

                while variants:
                    variant = variants[0]

                    if variant["proc"].is_alive():
                        #logging.warning("First still alive")
                        break

                    variants.pop(0)
                    #logging.warning("Handle {}".format(variant["proc"].sentinel))

                    if (variant["proc"].exitcode == 0 and
                        (self.max_improvement is None or
                         self._file_size_difference(test_case, variant["variant_path"]) < self.max_improvement)):
                        self._kill_variants(variants)
                        shutil.copy(variant["variant_path"], test_case)
                        state = pass_.advance_on_success(test_case, arg, variant["state"])
                        stopped = False
                        since_success = 0
                        self._update_pass_statistics(pass_, arg, success=True)
                        logging.debug("delta test success")

                        total_file_size = self._get_total_file_size()
                        pct = 100 - (total_file_size * 100.0 / self.orig_total_file_size)
                        logging.info("({}%, {} bytes)".format(round(pct, 1), total_file_size))
                    else:
                        since_success += 1
                        self._update_pass_statistics(pass_, arg, success=False)
                        logging.debug("delta test failure")

                    if (self.also_interesting != -1 and
                        self.also_interesting == variant["proc"].exitcode):
                        extra_dir = self._get_extra_dir(self.__orig_dir, "creduce_extra_", self.MAX_EXTRA_DIRS)

                        if extra_dir is not None:
                            shutil.move(variant["tmp_dir"], extra_dir)
                            logging.info("Created extra directory {} for you to look at later".format(extra_dir))

                    # Implicitly performs cleanup of temporary directories
                    variant = None

                # nasty heuristic for avoiding getting stuck by buggy passes
                # that keep reporting success w/o making progress
                if not self.no_give_up and since_success > self.GIVEUP_CONSTANT:
                    self._kill_variants(variants)

                    if not self.silent_pass_bug:
                        self._report_pass_bug(pass_, arg, "pass got stuck")

                    # Abort pass for this test case and
                    # start same pass with next test case
                    break

                if stopped and not variants:
                    # Abort pass for this test case and
                    # start same pass with next test case
                    break

    def _get_total_file_size(self):
        size = 0

        for test_case in self.test_cases:
            size += os.path.getsize(test_case)

        return size

    def _prepare_pass_group(self, pass_group, pass_options):
        group = self.groups[pass_group]

        def pass_filter(p):
            return ((("include" not in p) or bool(p["include"] & pass_options)) and
                    (("exclude" not in p) or not bool(p["exclude"] & pass_options)))

        for category in group:
            group[category] = [p for p in group[category] if pass_filter(p)]

        return group

    @staticmethod
    def _get_extra_dir(path, prefix, max_number):
        for i in range(0, max_number + 1):
            digits = int(round(math.log10(max_number), 0))
            extra_dir = os.path.join(path,
                                     ("{0}{1:0" + str(digits) + "d}").format(prefix, i))

            if not os.path.exists(extra_dir):
                break

        # just bail if we've already created enough of these dirs, no need to
        # clutter things up even more...
        if os.path.exists(extra_dir):
            return None

        return extra_dir

    def _report_pass_bug(self, delta_method, delta_arg, problem):
        if not self.die_on_pass_bug:
            logging.warning("{}::{} has encountered a non fatal bug: {}".format(delta_method, delta_arg, problem))

        crash_dir = self._get_extra_dir(self.__orig_dir, "creduce_bug_", self.MAX_CRASH_DIRS)

        if crash_dir == None:
            return

        os.mkdir(crash_dir)
        os.chdir(crash_dir)
        self._copy_test_cases(crash_dir)

        if not self.die_on_pass_bug:
            logging.debug("Please consider tarring up {} and mailing it to creduce-bugs@flux.utah.edu and we will try to fix the bug.".format(crash_dir))

        with open("PASS_BUG_INFO.TXT", mode="w") as info_file:
            info_file.write("{}\n".format(self.PACKAGE))
            info_file.write("{}\n".format(self.COMMIT))
            info_file.write("{}\n".format(platform.uname()))
            info_file.write(PassBugError.MSG.format(delta_method, delta_arg, problem, crash_dir))

        if self.die_on_pass_bug:
            raise PassBugError(delta_method, delta_arg, problem, crash_dir)

    @staticmethod
    def _diff_files(orig_file, changed_file):
        with open(orig_file, mode="r") as f:
            orig_file_lines = f.readlines()

        with open(changed_file, mode="r") as f:
            changed_file_lines = f.readlines()

        diffed_lines = difflib.unified_diff(orig_file_lines, changed_file_lines, orig_file, changed_file)

        return "".join(diffed_lines)

    @staticmethod
    def _file_size_difference(orig_file, changed_file):
        return (os.path.getsize(orig_file) - os.path.getsize(changed_file))
