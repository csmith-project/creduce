import importlib.util
#import logging
import multiprocessing
import os
import os.path
import signal
import subprocess
import sys

from .error import InvalidInterestingnessTestError

def _run_test(module_spec, test_cases):
    if sys.platform != "win32":
        pid = os.getpid()
        os.setpgid(pid, pid)

    module = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(module)
    module.run(test_cases)

class Variant:
    def __init__(self, process):
        self.__process = process
        self.__exitcode = None

        if isinstance(process, multiprocessing.Process):
            self.handle = process.sentinel
        elif isinstance(process, subprocess.Popen):
            if sys.platform == "win32":
                self.handle = process._handle
            else:
                self.handle = None
        else:
            raise NotImplementedError("The '{}' class cannot be used to create a variant!".format(process.__class__))

    def is_alive(self):
        return (self.exitcode is None)

    @property
    def pid(self):
        return self.__process.pid

    @property
    def exitcode(self):
        if self.__exitcode is None:
            if isinstance(self.__process, multiprocessing.Process):
                self.__exitcode = self.__process.exitcode
            elif isinstance(self.__process, subprocess.Popen):
                self.__exitcode = self.__process.poll()
            else:
                assert False, "Missing exitcode implementation for process class '{}'".format(self.__process.__class__)

        return self.__exitcode

    @exitcode.setter
    def exitcode(self, exitcode):
        self.__exitcode = exitcode

    def wait(self):
        if isinstance(self.__process, multiprocessing.Process):
            return self.__process.join()
        elif isinstance(self.__process, subprocess.Popen):
            return self.__process.wait()
        else:
            assert False, "Missing wait() implementation for process class '{}'".format(self.__process.__class__)

class GeneralRunner:
    def __init__(self, test_script):
        if not self.is_valid_test(test_script):
            raise InvalidInterestingnessTestError(test_script)

        self.test_path = os.path.abspath(test_script)

    @classmethod
    def is_valid_test(cls, test_script):
        for mode in {os.F_OK, os.X_OK}:
            if not os.access(test_script, mode):
                return False

        return True

    def launch_process(self, test_cases):
        cmd = [self.test_path]

        if isinstance(test_cases, list):
            cmd.extend(test_cases)
        else:
            cmd.append(test_cases)

        if sys.platform != "win32":
            def preexec_fn():
                pid = os.getpid()
                os.setpgid(pid, pid)
        else:
            preexec_fn = None

        return subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, preexec_fn=preexec_fn)

    def create_variant(self, test_cases):
        process = self.launch_process(test_cases)

        return Variant(process)

    @staticmethod
    def _wait_for_result_posix(variants):
        (pid, rtn) = os.wait()

        for v in variants:
            if v["proc"].pid == pid:
                v["proc"].exitcode = (rtn >> 8)
                break

    @staticmethod
    def _wait_for_result_win32(variants):
        handles = [v["proc"].handle for v in variants if v["proc"].is_alive()]

        # On Windows it is only possible to wait on max. 64 processes at once
        #TODO: Replace with loop
        handles = handles[0:64]

        #logging.warning("Waiting for {}".format(handles))

        # If all processes have already ended do not wait
        if handles:
            multiprocessing.connection.wait(descriptors)

    @classmethod
    def wait_for_results(cls, variants):
        if sys.platform == "win32":
            cls._wait_for_result_win32(variants)
        else:
            cls._wait_for_result_posix(variants)

    @staticmethod
    def _kill_variant_posix(pid):
        try:
            os.killpg(pid, signal.SIGTERM)
        except PermissionError:
            # On BSD based systems it is not allowed to kill a process group if it
            # consists of zombie processes
            # See: http://stackoverflow.com/questions/12521705/why-would-killpg-return-not-permitted-when-ownership-is-correct
            # Just do nothing in this case; everything has died and init will reap the zombies
            pass

    @staticmethod
    def _kill_variant_win32(pid):
        subprocess.run(["TASKKILL", "/T", "/PID", str(pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    @classmethod
    def kill_variants(cls, variants, no_kill):
        for v in variants:
            proc = v["proc"]

            #logging.warning("Kill {}".format(proc.handle))

            if proc.is_alive() and not no_kill:
                if sys.platform == "win32":
                    cls._kill_variant_win32(proc.pid)
                else:
                    cls._kill_variant_posix(proc.pid)

            proc.wait()

        # Performs implicit cleanup of the temporary directories
        variants.clear()

    @staticmethod
    def get_running_variants(variants):
        return [v for v in variants if v["proc"].is_alive()]

    @staticmethod
    def get_successful_variants(variants):
        return [v for v in variants if v["proc"].exitcode == 0]

    @staticmethod
    def has_successful_variant(variants):
        return any(v["proc"].exitcode == 0 for v in variants)

class PythonRunner(GeneralRunner):
    def __init__(self, test_module):
        if not self.is_valid_test(test_module):
            raise InvalidInterestingnessTestError(test_module)

        if os.path.isfile(test_module):
            (module_name, _) = os.path.splitext(os.path.basename(test_module))
            self.module_spec = importlib.util.spec_from_file_location(module_name, os.path.abspath(test_module))
        else:
            self.module_spec = importlib.util.find_spec(test_module)

    def launch_process(self, test_cases):
        process = multiprocessing.Process(target=_run_test, args=(self.module_spec, test_cases))
        process.start()

        return process

    @classmethod
    def is_valid_test(cls, test_script):
        try:
            if os.path.isfile(test_script):
                (module_name, _) = os.path.splitext(os.path.basename(test_script))
                module_spec = importlib.util.spec_from_file_location(module_name, test_script)
            else:
                module_spec = importlib.util.find_spec(test_script)

            module = importlib.util.module_from_spec(module_spec)
            module_spec.loader.exec_module(module)
        except FileNotFoundError:
            return False

        return (getattr(module, "run", None) is not None)
