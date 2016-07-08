#TODO: import all pass classes in passes module

import enum
import json
import logging
import os
import platform
import sys

from .passes import balanced
from .passes import blank
from .passes import clang
from .passes import clangbinarysearch
from .passes import clex
from .passes import comments
from .passes import includeincludes
from .passes import includes
from .passes import indent
from .passes import ints
from .passes import lines
from .passes import peep
from .passes import special
from .passes import ternary
from .passes import unifdef

from .utils.error import CReduceError
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
    pass_name_mapping = {
        "balanced": balanced.BalancedDeltaPass,
        "blank": blank.BlankDeltaPass,
        "clang": clang.ClangDeltaPass,
        "clangbinarysearch": clangbinarysearch.ClangBinarySearchDeltaPass,
        "clex": clex.ClexDeltaPass,
        "comments": comments.CommentsDeltaPass,
        "includeincludes": includeincludes.IncludeIncludesDeltaPass,
        "includes": includes.IncludesDeltaPass,
        "indent": indent.IndentDeltaPass,
        "ints": ints.IntsDeltaPass,
        "lines": lines.LinesDeltaPass,
        "peep": peep.PeepDeltaPass,
        "special": special.SpecialDeltaPass,
        "ternary": ternary.TernaryDeltaPass,
        "unifdef": unifdef.UnIfDefDeltaPass,
    }

    def __init__(self, test_manager):
        self.test_manager = test_manager
        self.tidy = False

    @classmethod
    def load_pass_group_file(cls, path):
        with open(path, mode="r") as pass_group_file:
            try:
                pass_group_dict = json.load(pass_group_file)
            except json.JSONDecodeError:
                raise CReduceError("Not valid JSON.")

        return pass_group_dict

    @classmethod
    def parse_pass_group_dict(cls, pass_group_dict, pass_options):
        pass_group = {}

        def parse_options(options):
            return set(Pass.Option(opt) for opt in options)

        def include_pass(pass_dict, options):
            return ((("include" not in pass_dict) or bool(parse_options(pass_dict["include"]) & options)) and
                    (("exclude" not in pass_dict) or not bool(parse_options(pass_dict["exclude"]) & options)))

        for category in ["first", "main", "last"]:
            if category not in pass_group_dict:
                raise CReduceError("Missing category {}".format(category))

            pass_group[category] = []

            for pass_dict in pass_group_dict[category]:
                if not include_pass(pass_dict, pass_options):
                    continue

                if "pass" not in pass_dict:
                    raise CReduceError("Invalid pass in category {}".format(category))

                try:
                    pass_class = cls.pass_name_mapping[pass_dict["pass"]]
                except KeyValueError:
                    raise CReduceError("Unkown pass {}".format(pass_dict["pass"]))

                if "arg" not in pass_dict:
                    raise CReduceError("Missing arg for pass {}".format(pass_dict["pass"]))

                #TODO: Create pass instances and get rid of Pass class
                pass_group[category].append(Pass(pass_class, pass_dict["arg"]))

        return pass_group

    def reduce(self, pass_group, skip_initial=False):
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
