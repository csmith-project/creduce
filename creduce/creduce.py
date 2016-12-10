import enum
import json
import logging
import os
import platform
import sys

from . import passes
from .utils.error import CReduceError
from .utils.error import PassOptionError
from .utils.error import PrerequisitesNotFoundError

class CReduce:
#FIXME: Write this file in cmake
    class Info:
        PACKAGE = "%PACKAGE%"
        COMMIT = "%COMMIT%"

    pass_name_mapping = {
        "balanced": passes.BalancedPass,
        "blank": passes.BlankPass,
        "clang": passes.ClangPass,
        "clangbinarysearch": passes.ClangBinarySearchPass,
        "clex": passes.ClexPass,
        "comments": passes.CommentsPass,
        "includeincludes": passes.IncludeIncludesPass,
        "includes": passes.IncludesPass,
        "indent": passes.IndentPass,
        "ints": passes.IntsPass,
        "lines": passes.LinesPass,
        "peep": passes.PeepPass,
        "special": passes.SpecialPass,
        "ternary": passes.TernaryPass,
        "unifdef": passes.UnIfDefPass,
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
    def parse_pass_group_dict(cls, pass_group_dict, pass_options, external_programs):
        pass_group = {}

        def parse_options(options):
            valid_options = set()

            for opt in options:
                try:
                    valid_options.add(passes.AbstractPass.Option(opt))
                except ValueError:
                    raise PassOptionError(opt)

            return valid_options

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

                pass_instance = pass_class(external_programs, pass_dict.get("arg"))
                pass_group[category].append(pass_instance)

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
        missing = set()

        for category in pass_group:
            for p in pass_group[category]:
                if not p.check_prerequisites():
                    logging.error("Prereqs not found for pass {}".format(p))
                    missing.add(str(p))

        if missing:
            raise PrerequisitesNotFoundError(missing)

    def _run_additional_passes(self, passes):
        for p in passes:
            self.test_manager.run_pass(p)

    def _run_main_passes(self, passes):
        while True:
            total_file_size = self.test_manager.total_file_size

            for p in passes:
                self.test_manager.run_pass(p)

            logging.info("Termination check: size was {}; now {}".format(total_file_size, self.test_manager.total_file_size))

            if  self.test_manager.total_file_size >= total_file_size:
                break
