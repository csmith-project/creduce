#TODO: Extract passes into json files

import enum
import logging
import os
import platform
import sys

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

from .utils.error import PassOptionError
from .utils.error import PrerequisitesNotFoundError

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
                raise PassOptionError()
        else:
            self.include = None

        if exclude is not None:
            tmp = set(exclude)

            if self._check_pass_options(tmp):
                self.exclude = tmp
            else:
                raise PassOptionError()
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

    groups = {PassGroup.all : {"first" : [Pass(IncludesDeltaPass, "0"), #0
                                          Pass(UnIfDefDeltaPass, "0", exclude={Pass.Option.windows}), #0
                                  Pass(CommentsDeltaPass, "0"), #0
                                  Pass(BlankDeltaPass, "0"), #1
                                  Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #2
                                  Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #3
                                  Pass(LinesDeltaPass, "0"), #20
                                  Pass(LinesDeltaPass, "1"), #21
                                  Pass(LinesDeltaPass, "2"), #22
                                  Pass(LinesDeltaPass, "3"), #23
                                  Pass(LinesDeltaPass, "4"), #24
                                  Pass(LinesDeltaPass, "6"), #25
                                  Pass(LinesDeltaPass, "8"), #26
                                  Pass(LinesDeltaPass, "10"), #27
                                  Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #33
                                  Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #34
                                  Pass(ClangDeltaPass, "remove-unused-function"), #40
                                  Pass(BalancedDeltaPass, "curly"), #41
                                  Pass(BalancedDeltaPass, "curly2"), #42
                                  Pass(BalancedDeltaPass, "curly3"), #43
                                  Pass(ClangDeltaPass, "callexpr-to-value"), #49
                                  Pass(ClangDeltaPass, "replace-callexpr"), #50
                                  Pass(ClangDeltaPass, "simplify-callexpr"), #51
                                  Pass(ClangDeltaPass, "remove-unused-enum-member"), #51
                                  Pass(ClangDeltaPass, "remove-enum-member-value"), #52
                                  Pass(ClangBinarySearchDeltaPass, "remove-unused-var"), #53
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
                                 Pass(ClangBinarySearchDeltaPass, "remove-unused-var"), #223
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
                                 Pass(LinesDeltaPass, "3"), #413
                                 Pass(LinesDeltaPass, "4"), #414
                                 Pass(LinesDeltaPass, "6"), #415
                                 Pass(LinesDeltaPass, "8"), #416
                                 Pass(LinesDeltaPass, "10"), #417
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
                                        Pass(LinesDeltaPass, "1"), #21
                                        Pass(LinesDeltaPass, "2"), #22
                                        Pass(LinesDeltaPass, "3"), #23
                                        Pass(LinesDeltaPass, "4"), #24
                                        Pass(LinesDeltaPass, "6"), #25
                                        Pass(LinesDeltaPass, "8"), #26
                                        Pass(LinesDeltaPass, "10"), #27
                                        Pass(ClangBinarySearchDeltaPass, "replace-function-def-with-decl"), #33
                                        Pass(ClangBinarySearchDeltaPass, "remove-unused-function"), #34
                                        Pass(ClangDeltaPass, "remove-unused-function"), #40
                                        Pass(BalancedDeltaPass, "curly"), #41
                                        Pass(BalancedDeltaPass, "curly2"), #42
                                        Pass(BalancedDeltaPass, "curly3"), #43
                                        Pass(ClangDeltaPass, "callexpr-to-value"), #49
                                        Pass(ClangDeltaPass, "replace-callexpr"), #50
                                        Pass(ClangDeltaPass, "simplify-callexpr"), #51
                                        Pass(ClangDeltaPass, "remove-unused-enum-member"), #51
                                        Pass(ClangDeltaPass, "remove-enum-member-value"), #52
                                        Pass(ClangBinarySearchDeltaPass, "remove-unused-var"), #53
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
                                       Pass(ClangBinarySearchDeltaPass, "remove-unused-var"), #223
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
                                       Pass(LinesDeltaPass, "3"), #413
                                       Pass(LinesDeltaPass, "4"), #414
                                       Pass(LinesDeltaPass, "6"), #415
                                       Pass(LinesDeltaPass, "8"), #416
                                       Pass(LinesDeltaPass, "10"), #417
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

    def __init__(self, test_manager):
        self.test_manager = test_manager
        self.tidy = False

    def reduce(self, skip_initial=False, pass_group=PassGroup.all, pass_options=set()):
        pass_options = set(pass_options)

        if sys.platform == "win32":
            pass_options.add(self.Pass.Option.windows)

        pass_group = self._prepare_pass_group(pass_group, pass_options)

        self._check_prerequisites(pass_group)
        self.test_manager.check_sanity()

        logging.info("===< {} >===".format(os.getpid()))
        logging.info("running {} interestingness test{} in parallel".format(self.test_manager.parallel_tests,
                                                                            "" if self.test_manager.parallel_tests == 1 else "s"))

        if not self.tidy:
            self.test_manager.backup_test_cases()

        if not skip_initial:
            logging.info("INITIAL PASSES")
            self._run_additional_passes(pass_group["first"])

        logging.info("MAIN PASSES")
        self._run_main_passes(pass_group["main"])

        logging.info("CLEANUP PASS")
        self._run_additional_passes(pass_group["last"])

        logging.info("===================== done ====================")
        return True

    @staticmethod
    def _check_prerequisites(pass_group):
        passes = set()
        missing = []

        for category in pass_group:
            passes |= set(map(lambda p: p.pass_, pass_group[category]))

        for p in passes:
            if not p.check_prerequisites():
                logging.error("Prereqs not found for pass {}".format(p))
                missing.append(p)

        if missing:
            raise PrerequisitesNotFoundError(missing)

    def _run_additional_passes(self, passes):
        for p in passes:
            self.test_manager.run_pass(p.pass_, p.arg)

    def _run_main_passes(self, passes):
        while True:
            total_file_size = self.test_manager.total_file_size

            for p in passes:
                self.test_manager.run_pass(p.pass_, p.arg)

            logging.info("Termination check: size was {}; now {}".format(total_file_size, self.test_manager.total_file_size))

            if  self.test_manager.total_file_size >= total_file_size:
                break

    def _prepare_pass_group(self, pass_group, pass_options):
        group = self.groups[pass_group]

        def pass_filter(p):
            return (((p.include is None) or bool(p.include & pass_options)) and
                    ((p.exclude is None) or not bool(p.exclude & pass_options)))

        for category in group:
            group[category] = [p for p in group[category] if pass_filter(p)]

        return group
