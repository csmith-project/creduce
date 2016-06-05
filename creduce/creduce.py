#!/usr/bin/env python3

import argparse
import enum
import filecmp
import logging
import multiprocessing
import multiprocessing.connection
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile

from passes.delta import DeltaPass
from passes.balanced import BalancedDeltaPass
from passes.blank import BlankDeltaPass
from passes.clang import ClangDeltaPass
from passes.clangbinarysearch import ClangBinarySearchDeltaPass
from passes.clex import ClexDeltaPass
from passes.comments import CommentsDeltaPass
from passes.includeincludes import IncludeIncludesDeltaPass
from passes.includes import IncludesDeltaPass
from passes.indent import IndentDeltaPass
from passes.ints import IntsDeltaPass
from passes.lines import LinesDeltaPass
from passes.peep import PeepDeltaPass
from passes.special import SpecialDeltaPass
from passes.ternary import TernaryDeltaPass
from passes.unifdef import UnIfDefDeltaPass

from tests.test0 import *
from tests.test1 import *
from tests.test2 import *
from tests.test3 import *
from tests.test6 import *
from tests.test7 import *

from utils.error import InvalidTestCaseError

class CReduce:
    @enum.unique
    class PassOption(enum.Enum):
        not_implemented = "not_implemented"
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

    GIVEUP_CONSTANT = 1000

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
                                 {"pass" : TernaryDeltaPass, "arg" : "b", "exclude" : {PassOption.not_implemented}}, #104
                                 {"pass" : TernaryDeltaPass, "arg" : "c", "exclude" : {PassOption.not_implemented}}, #105
                                 {"pass" : BalancedDeltaPass, "arg" : "curly"}, #110
                                 {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #111
                                 {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #112
                                 {"pass" : BalancedDeltaPass, "arg" : "parens"}, #113
                                 {"pass" : BalancedDeltaPass, "arg" : "angles"}, #114
                                 {"pass" : BalancedDeltaPass, "arg" : "curly-only"}, #150
                                 {"pass" : BalancedDeltaPass, "arg" : "parens-only"}, #151
                                 {"pass" : BalancedDeltaPass, "arg" : "angles-only"}, #152
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
                                 {"pass" : ClangDeltaPass, "arg" : "replace-dependent-name"}, #256
                                 {"pass" : ClangDeltaPass, "arg" : "simplify-recursive-template-instantiation"}, #257
                                 {"pass" : LinesDeltaPass, "arg" : "0"}, #410
                                 {"pass" : LinesDeltaPass, "arg" : "1"}, #411
                                 {"pass" : LinesDeltaPass, "arg" : "2"}, #412
                                 {"pass" : LinesDeltaPass, "arg" : "10"}, #413
                                 {"pass" : UnIfDefDeltaPass, "arg" : "0", "exclude" : {PassOption.windows}}, #450
                                 {"pass" : CommentsDeltaPass, "arg" : "0"}, #451
                                 {"pass" : PeepDeltaPass, "arg" : "a", "exclude" : {PassOption.not_implemented}}, #500
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
                                 {"pass" : PeepDeltaPass, "arg" : "b", "include" : {PassOption.slow}, "exclude" : {PassOption.not_implemented}}, #9500
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
                                       {"pass" : TernaryDeltaPass, "arg" : "b", "exclude" : {PassOption.not_implemented}}, #104
                                       {"pass" : TernaryDeltaPass, "arg" : "c", "exclude" : {PassOption.not_implemented}}, #105
                                       {"pass" : BalancedDeltaPass, "arg" : "curly"}, #110
                                       {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #111
                                       {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #112
                                       {"pass" : BalancedDeltaPass, "arg" : "parens"}, #113
                                       {"pass" : BalancedDeltaPass, "arg" : "angles"}, #114
                                       {"pass" : BalancedDeltaPass, "arg" : "curly-only"}, #150
                                       {"pass" : BalancedDeltaPass, "arg" : "parens-only"}, #151
                                       {"pass" : BalancedDeltaPass, "arg" : "angles-only"}, #152
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
                                       {"pass" : ClangDeltaPass, "arg" : "replace-dependent-name"}, #256
                                       {"pass" : LinesDeltaPass, "arg" : "0"}, #410
                                       {"pass" : LinesDeltaPass, "arg" : "1"}, #411
                                       {"pass" : LinesDeltaPass, "arg" : "2"}, #412
                                       {"pass" : LinesDeltaPass, "arg" : "10"}, #413
                                       {"pass" : UnIfDefDeltaPass, "arg" : "0", "exclude" : {PassOption.windows}}, #450
                                       {"pass" : CommentsDeltaPass, "arg" : "0"}, #451
                                       {"pass" : PeepDeltaPass, "arg" : "a", "exclude" : {PassOption.not_implemented}}, #500
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
                                       {"pass" : PeepDeltaPass, "arg" : "b", "include" : {PassOption.slow}, "exclude" : {PassOption.not_implemented}}, #9500
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
              PassGroup.debug: {"first" : [],
                             "main" : [{"pass" : BalancedDeltaPass, "arg" : "curly"}, #110
                                       {"pass" : BalancedDeltaPass, "arg" : "curly2"}, #111
                                       {"pass" : BalancedDeltaPass, "arg" : "curly3"}, #112
                                       {"pass" : BalancedDeltaPass, "arg" : "parens"}, #113
                                       {"pass" : BalancedDeltaPass, "arg" : "angles"}, #114
                                       {"pass" : BalancedDeltaPass, "arg" : "curly-only"}, #150
                                       {"pass" : BalancedDeltaPass, "arg" : "parens-only"}, #151
                                       {"pass" : BalancedDeltaPass, "arg" : "angles-only"}, #152
                                      ],
                             "last" : []
                            },
    }

    def __init__(self, interestingness_test, test_cases):
        self.interestingness_test = interestingness_test
        self.test_cases = []
        self.total_file_size = 0
        self.tidy = True

        for test_case in test_cases:
            self._check_file_permissions(test_case, [os.F_OK, os.R_OK, os.W_OK], InvalidTestCaseError)
            self.test_cases.append(os.path.abspath(test_case))
            self.total_file_size += os.path.getsize(test_case)

    def reduce(self, parallel_tests, skip_initial=False, pass_group=PassGroup.all, pass_options=set()):
        self.__parallel_tests = parallel_tests
        self.__orig_dir = os.getcwd()
        self.__variants = []

        pass_options.add(self.PassOption.not_implemented)

        if platform.system() == "Windows":
            pass_options.add(self.PassOption.windows)

        pass_group = self._prepare_pass_group(pass_group, pass_options)
        missing = self._check_prerequisites(pass_group)

        if missing is not None:
            print("Prereqs not found for pass {}".format(missing))
            return False

        if not self._check_sanity():
            return False

        print("===< {} >===".format(os.getpid()))

        if not self.tidy:
            self._backup_files(self.test_cases)

        if not skip_initial:
            print("INITIAL PASSES")
            self._run_additional_passes(pass_group["first"])

        print("MAIN PASSES")
        self._run_main_passes(pass_group["main"])

        print("CLEANUP PASS")
        self._run_additional_passes(pass_group["last"])

        self.total_file_size = self._get_total_file_size()

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
        print("sanity check... ", end='')

        with tempfile.TemporaryDirectory(prefix="creduce-") as tmp_dir:
            print("tmpdir = {}".format(tmp_dir))

            os.chdir(tmp_dir)
            self._copy_test_cases(tmp_dir)

            #TODO: Output error create extra dir
            result = self.interestingness_test.check()

            if result:
                print("successful")

            os.chdir(self.__orig_dir)

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

    def _run_additional_passes(self, passes):
        for p in passes:
            self._run_delta_pass(p["pass"], p["arg"])

    def _run_main_passes(self, passes):
        while True:
            for p in passes:
                self._run_delta_pass(p["pass"], p["arg"])

            total_file_size = self._get_total_file_size()

            print("Termination check: size was {}; now {}".format(self.total_file_size, total_file_size))

            if total_file_size >= self.total_file_size:
                break
            else:
                self.total_file_size = total_file_size

    def _fork_variant(self, variant_path):
        process = multiprocessing.Process(target=self.interestingness_test.run)
        process.start()

        if platform.system() != "Windows":
            os.setpgid(process.pid, process.pid)

        return process

    def _wait_for_results(self):
        descriptors = [v["proc"].sentinel for v in self.__variants if v["proc"].is_alive()]

        # On Windows it is only possible to wait on 64 processes
        if platform.system() == "Windows":
            descriptors = descriptors[0:64]

        #logging.warning("Waiting for {}".format(descriptors))

        # If all processes have already ended do not wait
        if not descriptors:
            return descriptors

        return multiprocessing.connection.wait(descriptors)

    def _kill_variants(self):
        for v in self.__variants:
            proc = v["proc"]

            #logging.warning("Kill {}".format(v["proc"].sentinel))

            if proc.is_alive():
                if platform.system() == "Windows":
                    subprocess.run(["TASKKILL", "/F", "/T", "/PID", str(proc.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                else:
                    os.killpg(proc.pid, 15)

            proc.join()

            #v["tmp_dir"].cleanup()

        self.__variants = []
        self.__num_running = 0

    def _run_delta_pass(self, pass_, arg):
        #TODO: Check for zero size

        print("===< {} :: {} >===".format(pass_.__name__, arg))

        for test_case in self.test_cases:
            test_case_name = os.path.basename(test_case)
            state = pass_.new(test_case_name, arg)
            stopped = False
            self.__num_running = 0

            while True:
                while not stopped and self.__num_running < self.__parallel_tests:
                    tmp_dir = tempfile.TemporaryDirectory(prefix="creduce-")

                    os.chdir(tmp_dir.name)
                    self._copy_test_cases(tmp_dir.name)

                    variant_path = os.path.join(tmp_dir.name, test_case_name)

                    (result, state) = pass_.transform(variant_path, arg, state)

                    if result != DeltaPass.Result.ok and result != DeltaPass.Result.stop:
                        #TODO: Report bug
                        #TODO: Can we check for ERROR instead?
                        pass

                    if result == DeltaPass.Result.stop or result == DeltaPass.Result.error:
                        stopped = True
                    else:
                        #TODO: Report failure
                        proc = self._fork_variant(variant_path)
                        variant = {"proc": proc, "state": state, "tmp_dir": tmp_dir, "variant_path": variant_path}
                        #logging.warning("Fork {}".format(proc.sentinel))
                        self.__variants.append(variant)
                        self.__num_running += 1
                        #logging.warning("forked {}, num_running = {}, variants = {}".format(proc.pid, self.__num_running, len(self.__variants)))
                        state = pass_.advance(test_case, arg, state)

                    os.chdir(self.__orig_dir)

                if self.__num_running > 0:
                    #logging.debug("parent is waiting")
                    self._wait_for_results()
                    #logging.warning("Processes finished")

                while len(self.__variants) > 0:
                    variant = self.__variants[0]

                    if variant["proc"].is_alive():
                        #logging.warning("First still alive")
                        break

                    self.__variants.pop(0)
                    self.__num_running -= 1
                    #logging.warning("Handle {}".format(variant["proc"].sentinel))

                    if variant["proc"].exitcode == 0:
                        self._kill_variants()
                        shutil.copy(variant["variant_path"], test_case)
                        state = pass_.advance_on_success(test_case, arg, variant["state"])
                        stopped = False

                        logging.debug("delta test success")

                        #TODO: Do I need to take all test cases in to account?
                        pct = 100 - (os.path.getsize(test_case) * 100.0 / self.total_file_size)
                        print("({} %, {} bytes)".format(round(pct, 1), os.path.getsize(test_case)))
                    else:
                        logging.debug("delta test failure")

                    #variant["tmp_dir"].cleanup()
                    variant = None

                if stopped and len(self.__variants) == 0:
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

        pass_filter = lambda p: ((("include" not in p) or bool(p["include"] & pass_options)) and
                                 (("exclude" not in p) or not bool(p["exclude"] & pass_options)))

        for category in group:
            group[category] = [p for p in group[category] if pass_filter(p)]

        return group

if __name__ == "__main__":
    try:
        core_count = multiprocessing.cpu_count()
    except NotImplementedError:
        core_count = 1

    parser = argparse.ArgumentParser(description="C-Reduce")
    parser.add_argument("--n", "-n", type=int, default=core_count, help="Number of cores to use; C-Reduce tries to automatically pick a good setting but its choice may be too low or high for your situation")
    parser.add_argument("--tidy", action="store_true", default=False, help="Do not make a backup copy of each file to reduce as file.orig")
    parser.add_argument("--shaddap", action="store_true", default=False, help="Suppress output about non-fatal internal errors")
    parser.add_argument("--die-on-pass-bug", action="store_true", default=False, help="Terminate C-Reduce if a pass encounters an otherwise non-fatal problem")
    parser.add_argument("--sanitize", action="store_true", default=False, help="Attempt to obscure details from the original source file")
    parser.add_argument("--sllooww", action="store_true", default=False, help="Try harder to reduce, but perhaps take a long time to do so")
    parser.add_argument("--also-interesting", metavar="EXIT_CODE", type=int, nargs=1, help="A process exit code (somewhere in the range 64-113 would be usual) that, when returned by the interestingness test, will cause C-Reduce to save a copy of the variant")
    parser.add_argument("--debug", action="store_true", default=False, help="Print debug information")
    parser.add_argument("--no-kill", action="store_true", default=False, help="Wait for parallel instances to terminate on their own instead of killing them (only useful for debugging)")
    parser.add_argument("--no-give-up", action="store_true", default=False, help="Don't give up on a pass that hasn't made progress for {} iterations".format(CReduce.GIVEUP_CONSTANT))
    parser.add_argument("--print-diff", action="store_true", default=False, help="Show changes made by transformations, for debugging")
    parser.add_argument("--save-temps", action="store_true", default=False, help="Don't delete /tmp/creduce-xxxxxx directories on termination")
    parser.add_argument("--skip-initial-passes", action="store_true", default=False, help="Skip initial passes (useful if input is already partially reduced)")
    parser.add_argument("--timing", action="store_true", default=False, help="Print timestamps about reduction progress")
    parser.add_argument("--no-default-passes", action="store_true", default=False, help="Start with an empty pass schedule")
    parser.add_argument("--add-pass", metavar=("PASS", "SUBPASS", "PRIORITY"), nargs=3, help="Add the specified pass to the schedule")
    parser.add_argument("--skip-key-off", action="store_true", default=False, help="Disable skipping the rest of the current pass when \"s\" is pressed")
    parser.add_argument("--max-improvement", metavar="BYTES", type=int, nargs=1, help="Largest improvement in file size from a single transformation that C-Reduce should accept (useful only to slow C-Reduce down)")
    parser.add_argument("--pass-group", type=str, choices=list(map(str, CReduce.PassGroup)), default="all", help="Set of passes used during the reduction")
    parser.add_argument("interestingness_test", metavar="INTERESTINGNESS_TEST", help="Executable to check interestingness of test cases")
    parser.add_argument("test_cases", metavar="TEST_CASE", nargs="+", help="Test cases")

    args = parser.parse_args()
    pass_options = set()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    if args.sanitize:
        pass_options.add(CReduce.PassOption.sanitize)

    if args.sllooww:
        pass_options.add(CReduce.PassOption.slow)

    tests = {"test0": Test0InterestingnessTest,
             "test1": Test1InterestingnessTest,
             "test2": Test2InterestingnessTest,
             "test3": Test3InterestingnessTest,
             "test6": Test6InterestingnessTest,
             "test7": Test7InterestingnessTest}

    interestingness_test = tests[args.interestingness_test](map(os.path.basename, args.test_cases))

    reducer = CReduce(interestingness_test, args.test_cases)
    reducer.reduce(args.n,
                   skip_initial=args.skip_initial_passes,
                   pass_group=CReduce.PassGroup(args.pass_group),
                   pass_options=pass_options)
