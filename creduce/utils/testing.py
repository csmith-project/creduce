import asyncio
import difflib
import filecmp
import importlib.util
import logging
import math
import multiprocessing
import os
import os.path
import platform

# Currently the resource module is only available on Unix based systems
try:
    import resource
    # We're nice here and only open up to half of the allowed number of files
    MAX_OPEN_FILES = resource.getrlimit(resource.RLIMIT_NOFILE)[0] / 2
except ImportError:
    # Just assume that we might be able to open up to 1024 files
    MAX_OPEN_FILES = 1024

import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import weakref

from .. import CReduce
from ..passes import AbstractPass

from . import compat
from . import readkey
from .error import InsaneTestCaseError
from .error import InvalidInterestingnessTestError
from .error import InvalidTestCaseError
from .error import PassBugError
from .error import ZeroSizeError

def _run_test(module_spec, test_dir, test_cases):
    if sys.platform != "win32":
        pid = os.getpid()
        os.setpgid(pid, pid)

    os.chdir(test_dir)

    module = compat.importlib_module_from_spec(module_spec)
    module_spec.loader.exec_module(module)
    module.run(test_cases)

class AbstractTestEnvironment:
    def __init__(self, timeout, save_temps):
        self.test_case = None
        self.additional_files = set()
        self.state = None
        self._dir = tempfile.mkdtemp(prefix="creduce-")
        self.timeout = timeout
        self.save_temps = save_temps
        self._base_size = None

        if not save_temps:
            self._finalizer = weakref.finalize(self, self._cleanup, self.path)

    def __enter__(self):
        return self

    def __exit__(self, exc, value, tb):
        if not self.save_temps:
            self.cleanup()

    @classmethod
    def _cleanup(cls, name):
        try:
            shutil.rmtree(name)
        except FileNotFoundError:
            pass

    def cleanup(self):
        if self._finalizer.detach():
            self._cleanup(self.path)

    def copy_files(self, test_case, additional_files):
        if test_case is not None:
            self.test_case = os.path.basename(test_case)
            shutil.copy(test_case, self.path)
            self._base_size = os.path.getsize(test_case)

        for f in additional_files:
            self.additional_files.add(os.path.basename(f))
            shutil.copy(f, self.path)

    def dump(self, dst):
        if self.test_case is not None:
            shutil.copy(self.test_case_path, dst)

        for f in self.additional_files:
            shutil.copy(f, dst)

    @property
    def size_improvement(self):
        if self._base_size is None:
            return None
        else:
            return (self._base_size - os.path.getsize(self.test_case_path))

    @property
    def path(self):
        return self._dir

    @property
    def test_case_path(self):
        return os.path.join(self.path, self.test_case)

    @property
    def additional_files_paths(self):
        return [os.path.join(self.path, f) for f in self.additional_files]

    def start_test(self):
        raise NotImplementedError("Missing 'start_test' implementation in class '{}'".format(self.__class__))

    def has_result(self):
        raise NotImplementedError("Missing 'has_result' implementation in class '{}'".format(self.__class__))

    def check_result(self, result):
        raise NotImplementedError("Missing 'check_result' implementation in class '{}'".format(self.__class__))

    def wait_for_result(self):
        raise NotImplementedError("Missing 'wait_for_result' implementation in class '{}'".format(self.__class__))

class GeneralTestEnvironment(AbstractTestEnvironment):
    def __init__(self, test_script, timeout, save_temps):
        super().__init__(timeout, save_temps)

        self.test_script = test_script
        self.__exitcode = None
        self.__process = None
        self.__timer = None

    def __del__(self):
        if self.__timer is not None:
            self.__timer.cancel()

    def dump(self, dst):
        super().dump(dst)

        shutil.copy(self.test_script, dst)

    @property
    def process_handle(self):
        handle = None

        if sys.platform == "win32":
            if self.__process is not None:
                handle = self.__process._handle

        return handle

    @property
    def process_pid(self):
        if self.__process is None:
            return None
        else:
            return self.__process.pid

    @property
    def _exitcode(self):
        assert False, "Use check_result instead"

    @_exitcode.setter
    def _exitcode(self, exitcode):
        self.__exitcode = exitcode

    def start_test(self):
        cmd = [self.test_script]

        if self.test_case is not None:
            cmd.append(self.test_case_path)

        cmd.extend(self.additional_files_paths)

        if sys.platform != "win32":
            # Create process group to kill the whole process tree
            # Windows does not have this concept
            def preexec_fn():
                pid = os.getpid()
                os.setpgid(pid, pid)
        else:
            preexec_fn = None

        self.__process = subprocess.Popen(cmd, cwd=self.path, preexec_fn=preexec_fn, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        if self.timeout is not None:
            def timeout(process):
                # Only kill process if it is still running
                if process.poll() is None:
                    logging.debug("Test {} timed out!".format(process.pid))
                    AbstractTestRunner.killpg(process.pid)

            #TODO: Eventually the call to timeout should be canceled if the test gets killed otherwise or finishes
            # But currently there does not seem to be a way of doing it
            # So we just make sure that the timeout call checks if the process is still alive
            self.__timer = asyncio.get_event_loop().call_later(self.timeout, timeout, self.__process)

        return " ".join(cmd)

    def has_result(self):
        if self.__process is None:
            return False
        elif self.__exitcode is None:
            return (self.__process.poll() is not None)
        else:
            return (self.__exitcode is not None)

    def check_result(self, result):
        if self.__exitcode is None:
            self.__exitcode = self.__process.poll()

        return (self.__exitcode == result)

    def wait_for_result(self):
        if self.__process is not None:
            return self.__process.wait()

class PythonTestEnvironment(AbstractTestEnvironment):
    def __init__(self, module_spec, timeout, save_temps):
        super().__init__(timeout, save_temps)

        self.module_spec = module_spec
        self.__exitcode = None
        self.__process = None
        self.__timer = None

    def __del__(self):
        if self.__timer is not None:
            self.__timer.cancel()

    @property
    def process_handle(self):
        handle = None

        if sys.platform == "win32":
            if self.__process is not None:
                handle = self.__process.sentinel

        return handle

    @property
    def process_pid(self):
        if self.__process is None:
            return None
        else:
            return self.__process.pid

    @property
    def _exitcode(self):
        assert False, "Use check_result instead"

    @_exitcode.setter
    def _exitcode(self, exitcode):
        self.__exitcode = exitcode

    def start_test(self):
        files = []

        if self.test_case is not None:
            files.append(self.test_case_path)

        files.extend(self.additional_files_paths)

        self.__process = multiprocessing.Process(target=_run_test, args=(self.module_spec, self.path, files))
        self.__process.start()

        if self.timeout is not None:
            def timeout(process):
                # Only kill process if it is still running
                if process.poll() is None:
                    logging.debug("Test {} timed out!".format(process.pid))
                    AbstractTestRunner.killpg(process.pid)

            self.__timer = threading.Timer(self.timeout, timeout, [self.__process])
            self.__timer.start()

    def has_result(self):
        if self.__process is None:
            return False
        elif self.__exitcode is None:
            return (self.__process.exitcode is not None)
        else:
            return (self.__exitcode is not None)

    def check_result(self, result):
        if self.__exitcode is None:
            self.__exitcode = self.__process.exitcode

        return (self.__exitcode == result)

    def wait_for_result(self):
        if self.__process is not None:
            return self.__process.join()

class AbstractTestRunner:
    def __init__(self, test, timeout, save_temps, no_kill):
        if not self.is_valid_test(test):
            raise InvalidInterestingnessTestError(test)

        self.timeout = timeout
        self.save_temps = save_temps
        self.no_kill = no_kill

    @classmethod
    def is_valid_test(cls, test):
        raise NotImplementedError("Missing 'is_valid_test' implementation in class '{}'".format(cls))

    def create_environment(self):
        raise NotImplementedError("Missing 'create_environment' implementation in class '{}'".format(self.__class__))

    @classmethod
    def _wait_posix(cls, environments):
        (pid, status) = os.wait()

        for test_env in environments:
            if test_env.process_pid == pid:
                if os.WIFSIGNALED(status):
                    test_env._exitcode = -os.WTERMSIG(status)
                elif os.WIFEXITED(status):
                    test_env._exitcode = os.WEXITSTATUS(status)

                break

    @classmethod
    def _wait_win32(cls, environments):
        handles = [test_env.process_handle for test_env in environments if not test_env.has_result()]

        # On Windows it is only possible to wait on max. 64 processes at once
        # Just wait for the first 64 which is not perfect.
        # But who runs more than 64 processes anyway?
        multiprocessing.connection.wait(handles[0:64])

    @classmethod
    def wait(self, environments):
        if sys.platform == "win32":
            self._wait_win32(environments)
        else:
            self._wait_posix(environments)

    @classmethod
    def _kill_posix(cls, pid):
        try:
            os.killpg(pid, signal.SIGTERM)
        except ProcessLookupError:
            # The process might have ended by itself in the meantime
            pass
        except PermissionError:
            # On BSD based systems it is not allowed to kill a process group if it
            # consists of zombie processes
            # See: http://stackoverflow.com/questions/12521705/why-would-killpg-return-not-permitted-when-ownership-is-correct
            # Just do nothing in this case; everything has died and init will reap the zombies
            pass

    @classmethod
    def _kill_win32(cls, pid):
        compat.subprocess_run(["TASKKILL", "/T", "/PID", str(pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def kill(self, environments):
        for test_env in environments:
            #logging.debug("Kill {}".format(test_env.process_pid))

            if not test_env.has_result() and not self.no_kill:
                if sys.platform == "win32":
                    self._kill_win32(test_env.process_pid)
                else:
                    self._kill_posix(test_env.process_pid)

            test_env.wait_for_result()

    @classmethod
    def killpg(cls, pid):
        if sys.platform == "win32":
            cls._kill_win32(pid)
        else:
            cls._kill_posix(pid)


class GeneralTestRunner(AbstractTestRunner):
    def __init__(self, test_script, timeout, save_temps, no_kill):
        super().__init__(test_script, timeout, save_temps, no_kill)

        self.test_script = os.path.abspath(test_script)

    @classmethod
    def is_valid_test(cls, test_script):
        for mode in {os.F_OK, os.X_OK}:
            if not os.access(test_script, mode):
                return False

        return True

    def create_environment(self):
        return GeneralTestEnvironment(self.test_script, self.timeout, self.save_temps)

class PythonTestRunner(AbstractTestRunner):
    def __init__(self, test_module, timeout, save_temps, no_kill):
        super().__init__(test_module, timeout, save_temps, no_kill)

        if os.path.isfile(test_module):
            (module_name, _) = os.path.splitext(os.path.basename(test_module))
            self.module_spec = importlib.util.spec_from_file_location(module_name, os.path.abspath(test_module))
        else:
            self.module_spec = importlib.util.find_spec(test_module)

    @classmethod
    def is_valid_test(cls, test_script):
        try:
            if os.path.isfile(test_script):
                (module_name, _) = os.path.splitext(os.path.basename(test_script))
                module_spec = importlib.util.spec_from_file_location(module_name, test_script)
            else:
                module_spec = importlib.util.find_spec(test_script)

            if module_spec is None:
                return False

            module = compat.importlib_module_from_spec(module_spec)
            module_spec.loader.exec_module(module)
        except FileNotFoundError:
            return False

        return (getattr(module, "run", None) is not None)

    def create_environment(self):
        return PythonTestEnvironment(self.module_spec, self.timeout, self.save_temps)

class AbstractTestManager:
    GIVEUP_CONSTANT = 50000
    MAX_CRASH_DIRS = 10
    MAX_EXTRA_DIRS = 25000
    GIVEUP_CONSTANT = 50000

    def __init__(self, test_runner, pass_statistic, test_cases, parallel_tests, no_cache, skip_key_off, silent_pass_bug, die_on_pass_bug, print_diff, max_improvement, no_give_up, also_interesting):
        self.test_runner = test_runner
        self.pass_statistic = pass_statistic
        self.test_cases = set()
        self.parallel_tests = parallel_tests
        self.no_cache = no_cache
        self.skip_key_off = skip_key_off
        self.silent_pass_bug = silent_pass_bug
        self.die_on_pass_bug = die_on_pass_bug
        self.print_diff = print_diff
        self.max_improvement = max_improvement
        self.no_give_up = no_give_up
        self.also_interesting = also_interesting

        for test_case in test_cases:
            self._check_file_permissions(test_case, [os.F_OK, os.R_OK, os.W_OK], InvalidTestCaseError)
            self.test_cases.add(os.path.abspath(test_case))

        self._orig_total_file_size = self.total_file_size
        self._cache = {}

    @property
    def total_file_size(self):
        return self._get_file_size(self.test_cases)

    @property
    def sorted_test_cases(self):
        return sorted(self.test_cases, key=os.path.getsize)

    @staticmethod
    def _get_file_size(files):
        return sum(os.path.getsize(f) for f in files)

    def backup_test_cases(self):
        for f in self.test_cases:
            orig_file = "{}.orig".format(os.path.splitext(f)[0])

            if not os.path.exists(orig_file):
                # Copy file and preserve attributes
                shutil.copy2(f, orig_file)

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
    def _get_extra_dir(prefix, max_number):
        for i in range(0, max_number + 1):
            digits = int(round(math.log10(max_number), 0))
            extra_dir = ("{0}{1:0" + str(digits) + "d}").format(prefix, i)

            if not os.path.exists(extra_dir):
                break

        # just bail if we've already created enough of these dirs, no need to
        # clutter things up even more...
        if os.path.exists(extra_dir):
            return None

        return extra_dir

    def _report_pass_bug(self, test_env, problem):
        if not self.die_on_pass_bug:
            logging.warning("{} has encountered a non fatal bug: {}".format(self._pass, problem))

        crash_dir = self._get_extra_dir("creduce_bug_", self.MAX_CRASH_DIRS)

        if crash_dir == None:
            return

        os.mkdir(crash_dir)
        test_env.dump(crash_dir)

        if not self.die_on_pass_bug:
            logging.debug("Please consider tarring up {} and mailing it to creduce-bugs@flux.utah.edu and we will try to fix the bug.".format(crash_dir))

        with open(os.path.join(crash_dir, "PASS_BUG_INFO.TXT"), mode="w") as info_file:
            info_file.write("{}\n".format(CReduce.Info.PACKAGE_STRING))
            info_file.write("{}\n".format(CReduce.Info.GIT_VERSION))
            info_file.write("{}\n".format(platform.uname()))
            info_file.write(PassBugError.MSG.format(self._pass, problem, crash_dir))

        if self.die_on_pass_bug:
            raise PassBugError(self._pass, problem, crash_dir)

    @staticmethod
    def _diff_files(orig_file, changed_file):
        with open(orig_file, mode="r") as f:
            orig_file_lines = f.readlines()

        with open(changed_file, mode="r") as f:
            changed_file_lines = f.readlines()

        diffed_lines = difflib.unified_diff(orig_file_lines, changed_file_lines, orig_file, changed_file)

        return "".join(diffed_lines)

    def check_sanity(self):
        logging.debug("perform sanity check... ")

        test_env = self.test_runner.create_environment()

        logging.debug("sanity check tmpdir = {}".format(test_env.path))

        test_env.copy_files(None, self.test_cases)

        cmd = test_env.start_test()
        test_env.wait_for_result()

        if test_env.check_result(0):
            logging.debug("sanity check successful")
        else:
            raise InsaneTestCaseError(self.test_cases, cmd)

    def _get_active_tests(self):
        return [env for env in self._environments if not env.has_result()]

    def _get_finished_tests(self):
        return [env for env in self._environments if env.has_result()]

    def can_create_test_env(self):
        # Create new variants and launch tests as long as:
        # (a) there has been no error and the transformation space is not exhausted,
        if self._stopped or self._skip:
            return False

        # (b) there are not already to many open files
        if len(self._environments) >= MAX_OPEN_FILES:
            return False

        # (c) the test for the first variant in the list is still running,
        if self._environments and self._environments[0].has_result():
            return False

        # (d) the maximum number of parallel test instances has not been reached, and
        if len(self._get_active_tests()) >= self.parallel_tests:
            return False

        # (e) no earlier variant has already been successful (don't waste resources)
        if any(self._get_finished_tests()):
            return False

        return True

    def create_test_env(self):
        test_env = self.test_runner.create_environment()
        # Copy files from base env
        test_env.copy_files(self._base_test_env.test_case_path, self._base_test_env.additional_files_paths)
        # Copy state from base_env
        test_env.state = self._base_test_env.state

        (result, test_env.state) = self._pass.transform(test_env.test_case_path, test_env.state)

        #TODO: Can the state be altered if the transform fails?
        # If not this would need to be checked here
        # Transform can alter the state. This has to be reflected in the base test env
        self._base_test_env.state = test_env.state

        return (test_env, result)

    def run_pass(self, pass_):
        self._pass = pass_
        self._environments = []

        logging.info("===< {} >===".format(self._pass))

        if self.total_file_size == 0:
            raise ZeroSizeError(self.test_cases)

        for test_case in self.test_cases:
            self._current_test_case = test_case

            if self._get_file_size([test_case]) == 0:
                continue

            if not self.no_cache:
                with open(test_case, mode="r+") as tmp_file:
                    test_case_before_pass = tmp_file.read()

                    pass_key = repr(self._pass)

                    if (pass_key in self._cache and
                        test_case_before_pass in self._cache[pass_key]):
                        tmp_file.seek(0)
                        tmp_file.truncate(0)
                        tmp_file.write(self._cache[pass_key][test_case_before_pass])
                        logging.info("cache hit for {}".format(test_case))
                        continue

            # Create initial test environment
            self._base_test_env = self.test_runner.create_environment()
            self._base_test_env.copy_files(test_case, self.test_cases ^ {test_case})
            self._base_test_env.state = self._pass.new(self._base_test_env.test_case_path)

            self._stopped = (self._base_test_env.state is None)
            self._skip = False
            self._since_success = 0

            if not self.skip_key_off:
                logger = readkey.KeyLogger()

            while self._environments or not (self._stopped or self._skip):
                # Ignore more key presses after skip has been detected
                if not self.skip_key_off and not self._skip:
                    if logger.pressed_key() == "s":
                        self._skip = True
                        logging.info("****** skipping the rest of this pass ******")

                while self.can_create_test_env():
                    (test_env, result) = self.create_test_env()

                    if result != self._pass.Result.ok and result != self._pass.Result.stop:
                        if not self.silent_pass_bug:
                            self._report_pass_bug(test_env, str(test_env.state) if result == self._pass.Result.error else "unknown return code")

                    #TODO: Do we really want to stop when the transformation fails?
                    # Why not try the next one?
                    if result == self._pass.Result.stop or result == self._pass.Result.error:
                        self._stopped = True
                    else:
                        if self.print_diff:
                            diff_str = self._diff_files(self._base_test_env.test_case_path, test_env.test_case_path)
                            logging.info(diff_str)

                        # Report bug if transform did not change the file
                        if filecmp.cmp(self._base_test_env.test_case_path, test_env.test_case_path):
                            if not self.silent_pass_bug:
                                self._report_pass_bug(test_env, "pass failed to modify the variant")

                            self._stopped = True
                        else:
                            test_env.start_test()
                            self._environments.append(test_env)

                            state = self._pass.advance(self._base_test_env.test_case_path, self._base_test_env.state)

                            if state is not None:
                                self._base_test_env.state = state

                            self._stopped = (state is None)

                self.wait_for_results()
                self.process_results()
                self.cleanup_results()

                # nasty heuristic for avoiding getting stuck by buggy passes
                # that keep reporting success w/o making progress
                if not self.no_give_up and self._since_success > self.GIVEUP_CONSTANT:
                    self.test_runner.kill(self._environments)
                    self._environments = []

                    if not self.silent_pass_bug:
                        self._report_pass_bug(test_env, "pass got stuck")

                    # Abort pass for this test case and
                    # start same pass with next test case
                    break

            # Cache result of this pass
            if not self.no_cache:
                with open(test_case, mode="r") as tmp_file:
                    if pass_key not in self._cache:
                        self._cache[pass_key] = {}

                    self._cache[pass_key][test_case_before_pass] = tmp_file.read()

    def wait_for_results(self):
        logging.debug("Wait for results")

        # Only wait if the first variant is not ready yet
        if self._environments and not self._environments[0].has_result():
            self.test_runner.wait(self._environments)

    def process_results(self):
        logging.debug("Process results")

        while self._environments:
            test_env = self._environments[0]

            if not test_env.has_result():
                #logging.debug("First still alive")
                break

            self._environments.pop(0)
            #logging.info("Handle {}".format(test_env))

            if test_env.check_result(0):
                logging.debug("delta test success")

                if (self.max_improvement is not None and
                    test_env.size_improvement > self.max_improvement):
                    logging.debug("Too large improvement")
                    continue

                self.test_runner.kill(self._environments)
                self._environments = []

                self._base_test_env = test_env
                shutil.copy(self._base_test_env.test_case_path, self._current_test_case)
                state = self._pass.advance_on_success(test_env.test_case_path, self._base_test_env.state)

                if state is not None:
                    self._base_test_env.state = state
                    #logging.debug("Base state advance success: {}".format(self._base_test_env.state))

                self._stopped = (state is None)
                self._since_success = 0
                self.pass_statistic.update(self._pass, success=True)

                pct = 100 - (self.total_file_size * 100.0 / self._orig_total_file_size)
                logging.info("({}%, {} bytes)".format(round(pct, 1), self.total_file_size))
            else:
                logging.debug("delta test failure")

                self._since_success += 1
                self.pass_statistic.update(self._pass, success=False)

                if (self.also_interesting is not None and
                    test_env.check_result(self.also_interesting)):
                    extra_dir = self._get_extra_dir("creduce_extra_", self.MAX_EXTRA_DIRS)

                    if extra_dir is not None:
                        shutil.move(test_env.path, extra_dir)
                        logging.info("Created extra directory {} for you to look at later".format(extra_dir))

            # Implicitly performs cleanup of temporary directories
            test_env = None

    def cleanup_results(self):
        # Only keep unfinished environments or sucessful evironments with valid improvement size
        self._environments = [env for env in self._environments if not env.has_result() or
                                                                   (env.check_result(0) and
                                                                    (self.max_improvement is None or
                                                                     env.size_improvement <= self.max_improvement))]

class ConservativeTestManager(AbstractTestManager):
    pass

class FastConservativeTestManager(ConservativeTestManager):
    def process_results(self):
        super().process_results()

        # Seek for new speculative base test env
        found = False

        for i, test_env in enumerate(self._environments):
            if (self.max_improvement is not None and
                test_env.size_improvement > self.max_improvement):
                continue

            if test_env.check_result(0):
                found = True
                break

        if found:
            # Kill all later test envs as they are invalid anyway
            # but keep this one in as it has to be processed "officially" in order
            self.test_runner.kill(self._environments[(i + 1):])
            self._environments[(i + 1):] = []

            self._base_test_env = test_env
            shutil.copy(self._base_test_env.test_case_path, self._current_test_case)
            state = self._pass.advance_on_success(test_env.test_case_path, self._base_test_env.state)

            if state is not None:
                self._base_test_env.state = state

            self._stopped = (state is None)

class NonDeterministicTestManager(AbstractTestManager):
    def can_create_test_env(self):
        # Create new variants and launch tests as long as:
        # (a) there has been no error and the transformation space is not exhausted,
        if self._stopped:
            return False

        # (b) there are not already to many open files
        if len(self._environments) >= MAX_OPEN_FILES:
            return False

        # (c) no test has a positive result
        if any(test_env.check_result(0) for test_env in self._environments):
            return False

        # (d) the maximum number of parallel test instances has not been reached, and
        if len(self._get_active_tests()) >= self.parallel_tests:
            return False

        return True

    def create_test_env(self):
        test_env = self.test_runner.create_environment()
        # Copy files from base env
        test_env.copy_files(self._base_test_env.test_case_path, self._base_test_env.additional_files_paths)
        # Copy state from base_env
        test_env.state = self._base_test_env.state

        (result, test_env.state) = self._pass.transform(test_env.test_case_path, test_env.state)

        # Transform can alter the state. This has to be reflected in the base test env
        self._base_test_env.state = test_env.state

        return (test_env, result)

    def wait_for_results(self):
        logging.debug("Wait for results")

        # Only wait if no test is ready
        if self._environments and not any(env.has_result() for env in self._environments):
            self.test_runner.wait(self._environments)

    def process_results(self):
        logging.debug("Process results")

        found = False

        for test_env in self._environments:
            if not test_env.has_result():
                continue

            if (self.max_improvement is not None and
                test_env.size_improvement > self.max_improvement):
                continue

            if test_env.check_result(0):
                found = True
                break

        if found:
            logging.debug("delta test success")

            self.test_runner.kill(self._environments)
            self._environments = []

            self._base_test_env = test_env
            shutil.copy(self._base_test_env.test_case_path, self._current_test_case)
            state = self._pass.advance_on_success(test_env.test_case_path, self._base_test_env.state)

            if state is not None:
                self._base_test_env.state = state

            self._stopped = (state is None)
            self._since_success = 0
            self.pass_statistic.update(self._pass, success=True)

            pct = 100 - (self.total_file_size * 100.0 / self._orig_total_file_size)
            logging.info("({}%, {} bytes)".format(round(pct, 1), self.total_file_size))

            # Implicitly performs cleanup of temporary directories
            test_env = None
