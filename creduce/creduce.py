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

from .utils import parallel

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

class Pass:
    @enum.unique
    class Option(enum.Enum):
        sanitize = "sanitize"
        slow = "slow"
        windows = "windows"

    @classmethod
    def _check_pass_options(cls, options):
        return all(isinstance(opt, cls.Option) for opt in options)

    def __init__(self, pass_, arg, *, include=None, exclude=None):
        self.pass_ = pass_
        self.arg = arg

        if include is not None:
            tmp = set(include)

            if self._check_pass_options(tmp):
                self.include = tmp
            else:
                raise Pass.PassOptionError()
        else:
            self.include = None

        if exclude is not None:
            tmp = set(exclude)

            if self._check_pass_options(tmp):
                self.exclude = tmp
            else:
                raise Pass.PassOptionError()
        else:
            self.exclude = None

class CReduce:
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

    groups = {PassGroup.all : {"first" : [Pass(IncludesDeltaPass, "0"), #0
                                          Pass(UnIfDefDeltaPass, "0", exclude={Pass.Option.windows}), #0
                                  Pass(CommentsDeltaPass, "0"), #0
                                  Pass(BlankDeltaPass, "0"), #1
                                  Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #2
                                  Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #3
                                  Pass(LinesDeltaPass, "0"), #20
                                  Pass(LinesDeltaPass, "0"), #21
                                  #Pass(LinesDeltaPass, "0"), #22
                                  Pass(LinesDeltaPass, "1"), #23
                                  Pass(LinesDeltaPass, "1"), #24
                                  #Pass(LinesDeltaPass, "1"), #25
                                  Pass(LinesDeltaPass, "2"), #27
                                  Pass(LinesDeltaPass, "2"), #28
                                  #Pass(LinesDeltaPass, "2"), #29
                                  Pass(LinesDeltaPass, "10"), #30
                                  Pass(LinesDeltaPass, "10"), #31
                                  #Pass(LinesDeltaPass, "10"), #32
                                  Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #33
                                  Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #34
                                  Pass(LinesDeltaPass, "0"), #35
                                  Pass(LinesDeltaPass, "1"), #36
                                  Pass(LinesDeltaPass, "2"), #37
                                  Pass(LinesDeltaPass, "10"), #38
                                  Pass(ClangDeltaPass, "remove-unused-function"), #40
                                  Pass(BalancedDeltaPass, "curly"), #41
                                  Pass(BalancedDeltaPass, "curly2"), #42
                                  Pass(BalancedDeltaPass, "curly3"), #43
                                  Pass(ClangDeltaPass, "callexpr-to-value"), #49
                                  Pass(ClangDeltaPass, "replace-callexpr"), #50
                                  Pass(ClangDeltaPass, "simplify-callexpr"), #51
                                  Pass(ClangDeltaPass, "remove-unused-enum-member"), #51
                                  Pass(ClangDeltaPass, "remove-enum-member-value"), #52
                                  Pass(ClangDeltaPass, "remove-unused-var"), #53
                                  Pass(SpecialDeltaPass, "a"), #110
                                  Pass(SpecialDeltaPass, "b"), #110
                                  Pass(SpecialDeltaPass, "c"), #110
                                 ],
                       "main" : [Pass(IncludeIncludesDeltaPass, "0"), #100
                                 Pass(TernaryDeltaPass, "b"), #104
                                 Pass(TernaryDeltaPass, "c"), #105
                                 Pass(BalancedDeltaPass, "curly"), #110
                                 Pass(BalancedDeltaPass, "curly2"), #111
                                 Pass(BalancedDeltaPass, "curly3"), #112
                                 Pass(BalancedDeltaPass, "parens"), #113
                                 Pass(BalancedDeltaPass, "angles"), #114
                                 Pass(BalancedDeltaPass, "square"), #115
                                 Pass(BalancedDeltaPass, "curly-inside"), #150
                                 Pass(BalancedDeltaPass, "parens-inside"), #151
                                 Pass(BalancedDeltaPass, "angles-inside"), #152
                                 Pass(BalancedDeltaPass, "square-inside"), #153
                                 Pass(BalancedDeltaPass, "curly-only"), #160
                                 Pass(BalancedDeltaPass, "angles-only"), #162
                                 Pass(BalancedDeltaPass, "square-only"), #163
                                 Pass(ClangDeltaPass, "remove-namespace"), #200
                                 Pass(ClangDeltaPass, "aggregate-to-scalar"), #201
                                 #Pass(ClangDeltaPass, "binop-simplification"), #201
                                 Pass(ClangDeltaPass, "param-to-global"), #203
                                 Pass(ClangDeltaPass, "param-to-local"), #204
                                 Pass(ClangDeltaPass, "remove-nested-function"), #205
                                 Pass(ClangDeltaPass, "union-to-struct"), #208
                                 Pass(ClangDeltaPass, "return-void"), #212
                                 Pass(ClangDeltaPass, "simple-inliner"), #213
                                 Pass(ClangDeltaPass, "reduce-pointer-level"), #214
                                 Pass(ClangDeltaPass, "lift-assignment-expr"), #215
                                 Pass(ClangDeltaPass, "copy-propagation"), #216
                                 Pass(ClangDeltaPass, "callexpr-to-value"), #217
                                 Pass(ClangDeltaPass, "replace-callexpr"), #218
                                 Pass(ClangDeltaPass, "simplify-callexpr"), #219
                                 Pass(ClangDeltaPass, "remove-unused-function"), #220
                                 Pass(ClangDeltaPass, "remove-unused-enum-member"), #221
                                 Pass(ClangDeltaPass, "remove-enum-member-value"), #222
                                 Pass(ClangDeltaPass, "remove-unused-var"), #223
                                 Pass(ClangDeltaPass, "simplify-if"), #224
                                 Pass(ClangDeltaPass, "reduce-array-dim"), #225
                                 Pass(ClangDeltaPass, "reduce-array-size"), #226
                                 Pass(ClangDeltaPass, "move-function-body"), #227
                                 Pass(ClangDeltaPass, "simplify-comma-expr"), #228
                                 Pass(ClangDeltaPass, "simplify-dependent-typedef"), #229
                                 Pass(ClangDeltaPass, "replace-simple-typedef"), #230
                                 Pass(ClangDeltaPass, "replace-dependent-typedef"), #231
                                 Pass(ClangDeltaPass, "replace-one-level-typedef-type"), #232
                                 Pass(ClangDeltaPass, "remove-unused-field"), #233
                                 Pass(ClangDeltaPass, "instantiate-template-type-param-to-int"), #234
                                 Pass(ClangDeltaPass, "instantiate-template-param"), #235
                                 Pass(ClangDeltaPass, "template-arg-to-int"), #236
                                 Pass(ClangDeltaPass, "template-non-type-arg-to-int"), #237
                                 Pass(ClangDeltaPass, "reduce-class-template-param"), #238
                                 Pass(ClangDeltaPass, "remove-trivial-base-template"), #239
                                 Pass(ClangDeltaPass, "class-template-to-class"), #240
                                 Pass(ClangDeltaPass, "remove-base-class"), #241
                                 Pass(ClangDeltaPass, "replace-derived-class"), #242
                                 Pass(ClangDeltaPass, "remove-unresolved-base"), #243
                                 Pass(ClangDeltaPass, "remove-ctor-initializer"), #244
                                 Pass(ClangDeltaPass, "replace-class-with-base-template-spec"), #245
                                 Pass(ClangDeltaPass, "simplify-nested-class"), #246
                                 Pass(ClangDeltaPass, "remove-unused-outer-class"), #247
                                 Pass(ClangDeltaPass, "empty-struct-to-int"), #248
                                 Pass(ClangDeltaPass, "remove-pointer"), #249
                                 Pass(ClangDeltaPass, "reduce-pointer-pairs"), #250
                                 Pass(ClangDeltaPass, "remove-array"), #251
                                 Pass(ClangDeltaPass, "remove-addr-taken"), #252
                                 Pass(ClangDeltaPass, "simplify-struct"), #253
                                 Pass(ClangDeltaPass, "replace-undefined-function"), #254
                                 Pass(ClangDeltaPass, "replace-array-index-var"), #255
                                 Pass(ClangDeltaPass, "replace-array-access-with-index"), #256
                                 Pass(ClangDeltaPass, "replace-dependent-name"), #257
                                 Pass(ClangDeltaPass, "simplify-recursive-template-instantiation"), #258
                                 Pass(LinesDeltaPass, "0"), #410
                                 Pass(LinesDeltaPass, "1"), #411
                                 Pass(LinesDeltaPass, "2"), #412
                                 Pass(LinesDeltaPass, "10"), #413
                                 Pass(UnIfDefDeltaPass, "0", exclude={Pass.Option.windows}), #450
                                 Pass(CommentsDeltaPass, "0"), #451
                                 Pass(SpecialDeltaPass, "b"), #555
                                 Pass(SpecialDeltaPass, "c"), #555
                                 Pass(IndentDeltaPass, "regular"), #1000
                                 Pass(ClexDeltaPass, "rename-toks", include={Pass.Option.sanitize}), #1000
                                 Pass(ClexDeltaPass, "delete-string", include={Pass.Option.sanitize}), #1001
                                 Pass(ClexDeltaPass, "remove-asm-line", include={Pass.Option.sanitize}), #1002
                                 Pass(ClexDeltaPass, "remove-asm-comment", include={Pass.Option.sanitize}), #1003
                                 Pass(ClexDeltaPass, "shorten-string", include={Pass.Option.sanitize}), #1010
                                 Pass(ClexDeltaPass, "x-string", include={Pass.Option.sanitize}), #1011
                                 #Pass(ClexDeltaPass, "collapse-toks", include={Pass.Option.sanitize}), #5000
                                 Pass(BalancedDeltaPass, "parens-to-zero"), #9000
                                 Pass(ClexDeltaPass, "rm-toks-32", include={Pass.Option.slow}), #9000
                                 Pass(ClexDeltaPass, "rm-toks-31", include={Pass.Option.slow}), #9001
                                 Pass(ClexDeltaPass, "rm-toks-30", include={Pass.Option.slow}), #9002
                                 Pass(ClexDeltaPass, "rm-toks-29", include={Pass.Option.slow}), #9003
                                 Pass(ClexDeltaPass, "rm-toks-28", include={Pass.Option.slow}), #9004
                                 Pass(ClexDeltaPass, "rm-toks-27", include={Pass.Option.slow}), #9005
                                 Pass(ClexDeltaPass, "rm-toks-26", include={Pass.Option.slow}), #9006
                                 Pass(ClexDeltaPass, "rm-toks-25", include={Pass.Option.slow}), #9007
                                 Pass(ClexDeltaPass, "rm-toks-24", include={Pass.Option.slow}), #9008
                                 Pass(ClexDeltaPass, "rm-toks-23", include={Pass.Option.slow}), #9009
                                 Pass(ClexDeltaPass, "rm-toks-22", include={Pass.Option.slow}), #9010
                                 Pass(ClexDeltaPass, "rm-toks-21", include={Pass.Option.slow}), #9011
                                 Pass(ClexDeltaPass, "rm-toks-20", include={Pass.Option.slow}), #9012
                                 Pass(ClexDeltaPass, "rm-toks-19", include={Pass.Option.slow}), #9013
                                 Pass(ClexDeltaPass, "rm-toks-18", include={Pass.Option.slow}), #9014
                                 Pass(ClexDeltaPass, "rm-toks-17", include={Pass.Option.slow}), #9015
                                 Pass(ClexDeltaPass, "rm-toks-1"), #9016
                                 Pass(ClexDeltaPass, "rm-toks-2"), #9017
                                 Pass(ClexDeltaPass, "rm-toks-3"), #9018
                                 Pass(ClexDeltaPass, "rm-toks-4"), #9019
                                 Pass(ClexDeltaPass, "rm-toks-5"), #9020
                                 Pass(ClexDeltaPass, "rm-toks-6"), #9021
                                 Pass(ClexDeltaPass, "rm-toks-7"), #9022
                                 Pass(ClexDeltaPass, "rm-toks-8"), #9023
                                 Pass(ClexDeltaPass, "rm-toks-9"), #9024
                                 Pass(ClexDeltaPass, "rm-toks-10"), #9025
                                 Pass(ClexDeltaPass, "rm-toks-11"), #9026
                                 Pass(ClexDeltaPass, "rm-toks-12"), #9027
                                 Pass(ClexDeltaPass, "rm-toks-13"), #9028
                                 Pass(ClexDeltaPass, "rm-toks-14"), #9029
                                 Pass(ClexDeltaPass, "rm-toks-15"), #9030
                                 Pass(ClexDeltaPass, "rm-toks-16"), #9031
                                 Pass(ClexDeltaPass, "rm-tok-pattern-8", include={Pass.Option.slow}), #9100
                                 Pass(ClexDeltaPass, "rm-tok-pattern-4", exclude={Pass.Option.slow}), #9100
                                 Pass(ClangDeltaPass, "local-to-global"), #9500
                                 Pass(PeepDeltaPass, "a"), #9500
                                 Pass(PeepDeltaPass, "b", include={Pass.Option.slow}), #9500
                                 Pass(IntsDeltaPass, "a"), #9600
                                 Pass(IntsDeltaPass, "b"), #9601
                                 Pass(IntsDeltaPass, "c"), #9602
                                 Pass(IntsDeltaPass, "d"), #9603
                                 Pass(IntsDeltaPass, "e"), #9603
                                 Pass(BalancedDeltaPass, "parens-only"), #9700
                                ],
                       "last" : [Pass(ClangDeltaPass, "rename-fun"), #207
                                 Pass(ClangDeltaPass, "rename-param"), #209
                                 Pass(ClangDeltaPass, "rename-var"), #210
                                 Pass(ClangDeltaPass, "rename-class"), #211
                                 Pass(ClangDeltaPass, "rename-cxx-method"), #212
                                 Pass(ClangDeltaPass, "combine-global-var"), #990
                                 Pass(ClangDeltaPass, "combine-local-var"), #991
                                 Pass(ClangDeltaPass, "simplify-struct-union-decl"), #992
                                 Pass(ClangDeltaPass, "move-global-var"), #993
                                 Pass(ClangDeltaPass, "unify-function-decl"), #994
                                 Pass(LinesDeltaPass, "0"), #999
                                 Pass(ClexDeltaPass, "rename-toks", include={Pass.Option.sanitize}), #1000
                                 Pass(ClexDeltaPass, "delete-string"), #1001
                                 Pass(IndentDeltaPass, "final"), #9999
                                ]
                      },
              PassGroup.opencl120 : {"first" : [Pass(IncludesDeltaPass, "0"), #0
                                        Pass(UnIfDefDeltaPass, "0", exclude={Pass.Option.windows}), #0
                                        Pass(CommentsDeltaPass, "0"), #0
                                        Pass(BlankDeltaPass, "0"), #1
                                        Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #2
                                        Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #3
                                        Pass(LinesDeltaPass, "0"), #20
                                        Pass(LinesDeltaPass, "0"), #21
                                        #Pass(LinesDeltaPass, "0"), #22
                                        Pass(LinesDeltaPass, "1"), #23
                                        Pass(LinesDeltaPass, "1"), #24
                                        #Pass(LinesDeltaPass, "1"), #25
                                        Pass(LinesDeltaPass, "2"), #27
                                        Pass(LinesDeltaPass, "2"), #28
                                        #Pass(LinesDeltaPass, "2"), #29
                                        Pass(LinesDeltaPass, "10"), #30
                                        Pass(LinesDeltaPass, "10"), #31
                                        #Pass(LinesDeltaPass, "10"), #32
                                        Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #33
                                        Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #34
                                        Pass(LinesDeltaPass, "0"), #35
                                        Pass(LinesDeltaPass, "1"), #36
                                        Pass(LinesDeltaPass, "2"), #37
                                        Pass(LinesDeltaPass, "10"), #38
                                        Pass(ClangDeltaPass, "remove-unused-function"), #40
                                        Pass(BalancedDeltaPass, "curly"), #41
                                        Pass(BalancedDeltaPass, "curly2"), #42
                                        Pass(BalancedDeltaPass, "curly3"), #43
                                        Pass(ClangDeltaPass, "callexpr-to-value"), #49
                                        Pass(ClangDeltaPass, "replace-callexpr"), #50
                                        Pass(ClangDeltaPass, "simplify-callexpr"), #51
                                        Pass(ClangDeltaPass, "remove-unused-enum-member"), #51
                                        Pass(ClangDeltaPass, "remove-enum-member-value"), #52
                                        Pass(ClangDeltaPass, "remove-unused-var"), #53
                                        Pass(SpecialDeltaPass, "a"), #110
                                        Pass(SpecialDeltaPass, "b"), #110
                                        Pass(SpecialDeltaPass, "c"), #110
                                       ],
                             "main" : [Pass(IncludeIncludesDeltaPass, "0"), #100
                                       Pass(TernaryDeltaPass, "b"), #104
                                       Pass(TernaryDeltaPass, "c"), #105
                                       Pass(BalancedDeltaPass, "curly"), #110
                                       Pass(BalancedDeltaPass, "curly2"), #111
                                       Pass(BalancedDeltaPass, "curly3"), #112
                                       Pass(BalancedDeltaPass, "parens"), #113
                                       Pass(BalancedDeltaPass, "angles"), #114
                                       Pass(BalancedDeltaPass, "square"), #115
                                       Pass(BalancedDeltaPass, "curly-inside"), #150
                                       Pass(BalancedDeltaPass, "parens-inside"), #151
                                       Pass(BalancedDeltaPass, "angles-inside"), #152
                                       Pass(BalancedDeltaPass, "square-inside"), #153
                                       Pass(BalancedDeltaPass, "curly-only"), #160
                                       Pass(BalancedDeltaPass, "angles-only"), #162
                                       Pass(BalancedDeltaPass, "square-only"), #163
                                       Pass(ClangDeltaPass, "aggregate-to-scalar"), #201
                                       #Pass(ClangDeltaPass, "binop-simplification"), #201
                                       Pass(ClangDeltaPass, "param-to-local"), #204
                                       Pass(ClangDeltaPass, "union-to-struct"), #208
                                       Pass(ClangDeltaPass, "return-void"), #212
                                       Pass(ClangDeltaPass, "simple-inliner"), #213
                                       Pass(ClangDeltaPass, "reduce-pointer-level"), #214
                                       Pass(ClangDeltaPass, "lift-assignment-expr"), #215
                                       Pass(ClangDeltaPass, "copy-propagation"), #216
                                       Pass(ClangDeltaPass, "callexpr-to-value"), #217
                                       Pass(ClangDeltaPass, "replace-callexpr"), #218
                                       Pass(ClangDeltaPass, "simplify-callexpr"), #219
                                       Pass(ClangDeltaPass, "remove-unused-function"), #220
                                       Pass(ClangDeltaPass, "remove-unused-enum-member"), #221
                                       Pass(ClangDeltaPass, "remove-enum-member-value"), #222
                                       Pass(ClangDeltaPass, "remove-unused-var"), #223
                                       Pass(ClangDeltaPass, "simplify-if"), #224
                                       Pass(ClangDeltaPass, "reduce-array-dim"), #225
                                       Pass(ClangDeltaPass, "reduce-array-size"), #226
                                       Pass(ClangDeltaPass, "move-function-body"), #227
                                       Pass(ClangDeltaPass, "simplify-comma-expr"), #228
                                       Pass(ClangDeltaPass, "simplify-dependent-typedef"), #229
                                       Pass(ClangDeltaPass, "replace-simple-typedef"), #230
                                       Pass(ClangDeltaPass, "replace-dependent-typedef"), #231
                                       Pass(ClangDeltaPass, "replace-one-level-typedef-type"), #232
                                       Pass(ClangDeltaPass, "remove-unused-field"), #233
                                       Pass(ClangDeltaPass, "empty-struct-to-int"), #248
                                       Pass(ClangDeltaPass, "remove-pointer"), #249
                                       Pass(ClangDeltaPass, "reduce-pointer-pairs"), #250
                                       Pass(ClangDeltaPass, "remove-array"), #251
                                       Pass(ClangDeltaPass, "remove-addr-taken"), #252
                                       Pass(ClangDeltaPass, "simplify-struct"), #253
                                       Pass(ClangDeltaPass, "replace-undefined-function"), #254
                                       Pass(ClangDeltaPass, "replace-array-index-var"), #255
                                       Pass(ClangDeltaPass, "replace-array-access-with-index"), #256
                                       Pass(ClangDeltaPass, "replace-dependent-name"), #257
                                       Pass(LinesDeltaPass, "0"), #410
                                       Pass(LinesDeltaPass, "1"), #411
                                       Pass(LinesDeltaPass, "2"), #412
                                       Pass(LinesDeltaPass, "10"), #413
                                       Pass(UnIfDefDeltaPass, "0", exclude={Pass.Option.windows}), #450
                                       Pass(CommentsDeltaPass, "0"), #451
                                       Pass(SpecialDeltaPass, "b"), #555
                                       Pass(SpecialDeltaPass, "c"), #555
                                       Pass(IndentDeltaPass, "regular"), #1000
                                       Pass(ClexDeltaPass, "rename-toks", include={Pass.Option.sanitize}), #1000
                                       Pass(ClexDeltaPass, "delete-string", include={Pass.Option.sanitize}), #1001
                                       Pass(ClexDeltaPass, "shorten-string", include={Pass.Option.sanitize}), #1010
                                       Pass(ClexDeltaPass, "x-string", include={Pass.Option.sanitize}), #1011
                                       #Pass(ClexDeltaPass, "collapse-toks", include={Pass.Option.sanitize}), #5000
                                       Pass(BalancedDeltaPass, "parens-to-zero"), #9000
                                       Pass(ClexDeltaPass, "rm-toks-32", include={Pass.Option.slow}), #9000
                                       Pass(ClexDeltaPass, "rm-toks-31", include={Pass.Option.slow}), #9001
                                       Pass(ClexDeltaPass, "rm-toks-30", include={Pass.Option.slow}), #9002
                                       Pass(ClexDeltaPass, "rm-toks-29", include={Pass.Option.slow}), #9003
                                       Pass(ClexDeltaPass, "rm-toks-28", include={Pass.Option.slow}), #9004
                                       Pass(ClexDeltaPass, "rm-toks-27", include={Pass.Option.slow}), #9005
                                       Pass(ClexDeltaPass, "rm-toks-26", include={Pass.Option.slow}), #9006
                                       Pass(ClexDeltaPass, "rm-toks-25", include={Pass.Option.slow}), #9007
                                       Pass(ClexDeltaPass, "rm-toks-24", include={Pass.Option.slow}), #9008
                                       Pass(ClexDeltaPass, "rm-toks-23", include={Pass.Option.slow}), #9009
                                       Pass(ClexDeltaPass, "rm-toks-22", include={Pass.Option.slow}), #9010
                                       Pass(ClexDeltaPass, "rm-toks-21", include={Pass.Option.slow}), #9011
                                       Pass(ClexDeltaPass, "rm-toks-20", include={Pass.Option.slow}), #9012
                                       Pass(ClexDeltaPass, "rm-toks-19", include={Pass.Option.slow}), #9013
                                       Pass(ClexDeltaPass, "rm-toks-18", include={Pass.Option.slow}), #9014
                                       Pass(ClexDeltaPass, "rm-toks-17", include={Pass.Option.slow}), #9015
                                       Pass(ClexDeltaPass, "rm-toks-1"), #9016
                                       Pass(ClexDeltaPass, "rm-toks-2"), #9017
                                       Pass(ClexDeltaPass, "rm-toks-3"), #9018
                                       Pass(ClexDeltaPass, "rm-toks-4"), #9019
                                       Pass(ClexDeltaPass, "rm-toks-5"), #9020
                                       Pass(ClexDeltaPass, "rm-toks-6"), #9021
                                       Pass(ClexDeltaPass, "rm-toks-7"), #9022
                                       Pass(ClexDeltaPass, "rm-toks-8"), #9023
                                       Pass(ClexDeltaPass, "rm-toks-9"), #9024
                                       Pass(ClexDeltaPass, "rm-toks-10"), #9025
                                       Pass(ClexDeltaPass, "rm-toks-11"), #9026
                                       Pass(ClexDeltaPass, "rm-toks-12"), #9027
                                       Pass(ClexDeltaPass, "rm-toks-13"), #9028
                                       Pass(ClexDeltaPass, "rm-toks-14"), #9029
                                       Pass(ClexDeltaPass, "rm-toks-15"), #9030
                                       Pass(ClexDeltaPass, "rm-toks-16"), #9031
                                       Pass(ClexDeltaPass, "rm-tok-pattern-8", include={Pass.Option.slow}), #9100
                                       Pass(ClexDeltaPass, "rm-tok-pattern-4", exclude={Pass.Option.slow}), #9100
                                       Pass(PeepDeltaPass, "a"), #9500
                                       Pass(PeepDeltaPass, "b", include={Pass.Option.slow}), #9500
                                       Pass(IntsDeltaPass, "a"), #9600
                                       Pass(IntsDeltaPass, "b"), #9601
                                       Pass(IntsDeltaPass, "c"), #9602
                                       Pass(IntsDeltaPass, "d"), #9603
                                       Pass(IntsDeltaPass, "e"), #9603
                                       Pass(BalancedDeltaPass, "parens-only"), #9700
                                      ],
                             "last" : [Pass(ClangDeltaPass, "rename-fun"), #207
                                       Pass(ClangDeltaPass, "rename-param"), #209
                                       Pass(ClangDeltaPass, "rename-var"), #210
                                       Pass(ClangDeltaPass, "combine-local-var"), #991
                                       Pass(ClangDeltaPass, "simplify-struct-union-decl"), #992
                                       Pass(ClangDeltaPass, "unify-function-decl"), #994
                                       Pass(LinesDeltaPass, "0"), #999
                                       Pass(ClexDeltaPass, "rename-toks", include={Pass.Option.sanitize}), #1000
                                       Pass(ClexDeltaPass, "delete-string"), #1001
                                       Pass(IndentDeltaPass, "final"), #9999
                                      ]
                            },
    }

    def __init__(self, test_path, test_cases):
        self.test_path = os.path.abspath(test_path)
        self.test_cases = []
        self.cache = {}
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
        self.no_cache = False
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
            pass_options.add(self.Pass.Option.windows)

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
    def _generate_unique_pass_key(pass_, arg):
        return str(pass_) + str(arg)

    @staticmethod
    def _init_pass_statistics(pass_group):
        stats = {}

        for category in pass_group:
            for p in pass_group[category]:
                key = CReduce._generate_unique_pass_key(p.pass_, p.arg)
                stats[key] = {"pass" : p.pass_,
                              "arg" : p.arg,
                              "worked" : 0,
                              "failed" : 0}

        return stats

    def _update_pass_statistics(self, pass_, arg, success):
        key = self._generate_unique_pass_key(pass_, arg)

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
            passes |= set(map(lambda p: p.pass_, pass_group[category]))

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

            proc = parallel.create_variant(self.test_path, self.test_cases, self.no_setpgrp)
            proc.wait()

            if proc.returncode == 0:
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
            self._run_delta_pass(p.pass_, p.arg)

    def _run_main_passes(self, passes):
        while True:
            for p in passes:
                self._run_delta_pass(p.pass_, p.arg)

            total_file_size = self._get_total_file_size()

            logging.info("Termination check: size was {}; now {}".format(self.total_file_size, total_file_size))

            if total_file_size >= self.total_file_size:
                break
            else:
                self.total_file_size = total_file_size

    def _run_delta_pass(self, pass_, arg):
        logging.info("===< {} :: {} >===".format(pass_.__name__, arg))

        if self._get_total_file_size() == 0:
            raise ZeroSizeError(self.test_cases)

        for test_case in self.test_cases:
            if os.path.getsize(test_case) == 0:
                continue

            if not self.no_cache:
                with open(test_case, mode="r+") as tmp_file:
                    test_case_before_pass = tmp_file.read()

                    pass_key = self._generate_unique_pass_key(pass_, arg)

                    if (pass_key in self.cache and
                        test_case_before_pass in self.cache[pass_key]):
                        tmp_file.truncate(0)
                        tmp_file.write(self.cache[pass_key][test_case_before_pass])
                        logging.info("cache hit for {}".format(test_case))
                        continue

            state = pass_.new(test_case, arg)
            stopped = False
            since_success = 0
            variants = []

            while True:
                # Create new variants and launch tests as long as:
                # (a) there has been no error and the transformation space is not exhausted,
                # (b) there are not already to many variants (FIXME: can be removed later),
                # (c) the test fot the first variant in the list is still running,
                # (d) the maximum number of parallel test instances has not been reached, and
                # (e) no earlier variant has already been successful (don't waste resources)
                while (not stopped and
                       len(variants) < 200 and
                       (not variants or variants[0]["proc"].poll() is None) and
                       len(parallel.get_running_variants(variants)) < self.__parallel_tests and
                       not parallel.has_successful_variant(variants)):
                    tmp_dir = TemporaryDirectory(prefix="creduce-", delete=(not self.save_temps))

                    os.chdir(tmp_dir.name)
                    self._copy_test_cases(tmp_dir.name)

                    variant_path = os.path.join(tmp_dir.name, os.path.basename(test_case))
                    variants_paths = [os.path.join(tmp_dir.name, os.path.basename(tc)) for tc in self.test_cases]

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
                            proc = parallel.create_variant(self.test_path, variants_paths, self.no_setpgrp)
                            variant = {"proc": proc, "state": state, "tmp_dir": tmp_dir, "variant_path": variant_path}
                            #logging.warning("Fork {}".format(proc.sentinel))
                            variants.append(variant)
                            #logging.warning("forked {}, num_running = {}, variants = {}".format(proc.pid, len(self._get_running_variants(variants)), len(variants)))
                            state = pass_.advance(test_case, arg, state)

                    os.chdir(self.__orig_dir)

                # Only wait if the first variant is not ready yet
                if variants and variants[0]["proc"].poll() is None:
                    #logging.debug("parent is waiting")
                    parallel.wait_for_results(variants)
                    #logging.warning("Processes finished")

                while variants:
                    variant = variants[0]

                    if variant["proc"].poll() is None:
                        #logging.warning("First still alive")
                        break

                    variants.pop(0)
                    #logging.warning("Handle {}".format(variant["proc"]._handle))

                    if (variant["proc"].returncode == 0 and
                        (self.max_improvement is None or
                         self._file_size_difference(test_case, variant["variant_path"]) < self.max_improvement)):
                        parallel.kill_variants(variants, self.no_kill, self.no_setpgrp)
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
                        self.also_interesting == variant["proc"].returncode):
                        extra_dir = self._get_extra_dir(self.__orig_dir, "creduce_extra_", self.MAX_EXTRA_DIRS)

                        if extra_dir is not None:
                            shutil.move(variant["tmp_dir"], extra_dir)
                            logging.info("Created extra directory {} for you to look at later".format(extra_dir))

                    # Implicitly performs cleanup of temporary directories
                    variant = None

                # nasty heuristic for avoiding getting stuck by buggy passes
                # that keep reporting success w/o making progress
                if not self.no_give_up and since_success > self.GIVEUP_CONSTANT:
                    parallel.kill_variants(variants, self.no_kill, self.no_setpgrp)

                    if not self.silent_pass_bug:
                        self._report_pass_bug(pass_, arg, "pass got stuck")

                    # Abort pass for this test case and
                    # start same pass with next test case
                    break

                if stopped and not variants:
                    # Cache result of this pass
                    if not self.no_cache:
                        with open(test_case, mode="r") as tmp_file:
                            if pass_key not in self.cache:
                                self.cache[pass_key] = {}

                            self.cache[pass_key][test_case_before_pass] = tmp_file.read()

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
            return (((p.include is None) or bool(p.include & pass_options)) and
                    ((p.exclude is None) or not bool(p.exclude & pass_options)))

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
