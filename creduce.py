#!/usr/bin/env python3

import argparse
import logging
import multiprocessing
import os
import os.path
import shutil
import sys
import time

import importlib.util

# If the creduce modules cannot be found
# add the known install location to the path
if importlib.util.find_spec("creduce") is None:
    script_path = os.path.dirname(os.path.realpath(__file__))
    sys.path.append(os.path.join(script_path, "..", "share"))

from creduce import CReduce
from creduce.passes import AbstractPass
from creduce.utils.error import CReduceError
from creduce.utils.error import MissingPassGroupsError
from creduce.utils.info import ExternalPrograms
from creduce.utils import testing
from creduce.utils import statistics

def get_share_dir():
    script_path = os.path.dirname(os.path.realpath(__file__))

    # Test all known locations for the creduce directory
    share_dirs = [
            os.path.join(script_path, "..", "share", "creduce"),
            os.path.join(script_path, "creduce")
            ]

    for d in share_dirs:
        if os.path.isdir(d):
            return d

    raise CReduceError("Cannot find creduce module directory!")

def get_libexec_dir():
    script_path = os.path.dirname(os.path.realpath(__file__))

    # Test all known locations for the creduce directory
    libexec_dirs = [
            os.path.join(script_path, "..", "libexec"),
            #FIXME: The programs are in sub directories
            os.path.join(script_path)
            ]

    for d in libexec_dirs:
        if os.path.isdir(d):
            return d

    raise CReduceError("Cannot find libexec directory!")

def find_external_programs():
    programs = ExternalPrograms()
    libexec_dir = get_libexec_dir()

    for prog_key in ExternalPrograms.programs:
        prog = programs[prog_key]

        path = shutil.which(prog)

        if path is None:
            path = shutil.which(prog, path=libexec_dir)

        if path is not None:
            programs[prog_key] = path

    return programs

def get_pass_group_path(name):
    return os.path.join(get_share_dir(), "pass_groups", name + ".json")

def get_available_pass_groups():
    pass_group_dir = os.path.join(get_share_dir(), "pass_groups")

    if not os.path.isdir(pass_group_dir):
        raise MissingPassGroupsError()

    group_names = []

    for entry in os.listdir(pass_group_dir):
        path = os.path.join(pass_group_dir, entry)

        if not os.path.isfile(path):
            continue

        try:
            pass_group_dict = CReduce.load_pass_group_file(path)
            CReduce.parse_pass_group_dict(pass_group_dict, set(), None)
        except MissingPassGroupsError:
            logging.warning("Skipping file {}. Not valid pass group.".format(path))
        else:
            (name, _) = os.path.splitext(entry)
            group_names.append(name)

    return group_names

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
    parser.add_argument("--sllooww", action="store_true", default=False, help="Try harder to reduce, but perhaps take a long time to do so")
    parser.add_argument("--also-interesting", metavar="EXIT_CODE", type=int, help="A process exit code (somewhere in the range 64-113 would be usual) that, when returned by the interestingness test, will cause C-Reduce to save a copy of the variant")
    parser.add_argument("--debug", action="store_true", default=False, help="Print debug information")
    parser.add_argument("--log-level", type=str, choices=["INFO", "DEBUG", "WARNING", "ERROR"], default="INFO", help="Define the verbosity of the logged events")
    parser.add_argument("--log-file", type=str, help="Log events into LOG_FILE instead of stderr. New events are appended to the end of the file")
    parser.add_argument("--no-kill", action="store_true", default=False, help="Wait for testing instances to terminate on their own instead of killing them (only useful for debugging)")
    parser.add_argument("--no-give-up", action="store_true", default=False, help="Don't give up on a pass that hasn't made progress for {} iterations".format(testing.AbstractTestManager.GIVEUP_CONSTANT))
    parser.add_argument("--print-diff", action="store_true", default=False, help="Show changes made by transformations, for debugging")
    parser.add_argument("--save-temps", action="store_true", default=False, help="Don't delete /tmp/creduce-xxxxxx directories on termination")
    parser.add_argument("--skip-initial-passes", action="store_true", default=False, help="Skip initial passes (useful if input is already partially reduced)")
    parser.add_argument("--timing", action="store_true", default=False, help="Print timestamps about reduction progress")
    parser.add_argument("--timeout", type=int, nargs="?", const=300, help="Interestingness test timeout in seconds")
    parser.add_argument("--no-cache", action="store_true", default=False, help="Don't cache behavior of passes")
    parser.add_argument("--skip-key-off", action="store_true", default=False, help="Disable skipping the rest of the current pass when \"s\" is pressed")
    parser.add_argument("--max-improvement", metavar="BYTES", type=int, help="Largest improvement in file size from a single transformation that C-Reduce should accept (useful only to slow C-Reduce down)")
    passes_group = parser.add_mutually_exclusive_group()
    passes_group.add_argument("--pass-group", type=str, choices=get_available_pass_groups(), help="Set of passes used during the reduction")
    passes_group.add_argument("--pass-group-file", type=str, help="JSON file defining a custom pass group")
    parser.add_argument("--test-manager", type=str, choices=["conservative", "fast-conservative", "non-deterministic"], help="Strategy for the testing reduction process")
    parser.add_argument("--no-fast-test", action="store_true", help="Use the general test runner even if a faster implementation is available")
    parser.add_argument("interestingness_test", metavar="INTERESTINGNESS_TEST", help="Executable to check interestingness of test cases")
    parser.add_argument("test_cases", metavar="TEST_CASE", nargs="+", help="Test cases")

    args = parser.parse_args()

    log_config = {}

    if args.timing:
        log_config["format"] = "%(asctime)s@%(levelname)s@%(name)s@%(message)s"
    else:
        log_config["format"] = "@%(levelname)s@%(name)s@%(message)s"

    if args.debug:
        log_config["level"] = logging.DEBUG
    else:
        log_config["level"] = getattr(logging, args.log_level.upper())

    if args.log_file is not None:
        log_config["filename"] = args.log_file

    logging.basicConfig(**log_config)

    pass_options = set()

    if sys.platform == "win32":
        pass_options.add(AbstractPass.Option.windows)

    if args.sllooww:
        pass_options.add(AbstractPass.Option.slow)

    if args.pass_group is not None:
        pass_group_file = get_pass_group_path(args.pass_group)
    elif args.pass_group_file is not None:
        pass_group_file = args.pass_group_file
    else:
        pass_group_file = get_pass_group_path("all")

    external_programs = find_external_programs()

    pass_group_dict = CReduce.load_pass_group_file(pass_group_file)
    pass_group = CReduce.parse_pass_group_dict(pass_group_dict, pass_options, external_programs)

    if (not args.no_fast_test and
        testing.PythonTestRunner.is_valid_test(args.interestingness_test)):
        test_runner = testing.PythonTestRunner(args.interestingness_test, args.timeout, args.save_temps, args.no_kill)
    else:
        test_runner = testing.GeneralTestRunner(args.interestingness_test, args.timeout, args.save_temps, args.no_kill)

    pass_statistic = statistics.PassStatistic()

    if args.test_manager == "fast-conservative":
        test_manager_class = testing.FastConservativeTestManager
    elif args.test_manager == "non-deterministic":
        test_manager_class = testing.NonDeterministicTestManager
    else:
        test_manager_class = testing.ConservativeTestManager

    test_manager = test_manager_class(test_runner, pass_statistic, args.test_cases, args.n, args.no_cache, args.skip_key_off, args.shaddap, args.die_on_pass_bug, args.print_diff, args.max_improvement, args.no_give_up, args.also_interesting)

    reducer = CReduce(test_manager)

    reducer.tidy = args.tidy

    # Track runtime
    if args.timing:
        time_start = time.monotonic()

    try:
        reducer.reduce(pass_group, skip_initial=args.skip_initial_passes)
    except CReduceError as err:
        print(err)
    else:
        print("pass statistics:")

        for item in pass_statistic.sorted_results:
            print("method {pass} worked {worked} times and failed {failed} times".format(**item))

        for test_case in test_manager.sorted_test_cases:
            with open(test_case, mode="r") as test_case_file:
                print(test_case_file.read())

    if args.timing:
        time_stop = time.monotonic()
        print("Runtime: {} seconds".format(round((time_stop - time_start))))

    logging.shutdown()
