import logging
import multiprocessing
import os
import signal
import subprocess
import sys

def create_variant(test_path, test_cases, no_setpgrp):
    cmd = [test_path]

    if isinstance(test_cases, list):
        cmd.extend(test_cases)
    else:
        cmd.append(test_cases)

    if not no_setpgrp and sys.platform != "win32":
        def preexec_fn():
            pid = os.getpid()
            os.setpgid(pid, pid)
    else:
        preexec_fn = None

    process = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, preexec_fn=preexec_fn)

    return process

def _wait_for_result_posix(variants):
    (pid, rtn) = os.wait()

    for v in variants:
        if v["proc"].pid == pid:
            v["proc"].returncode = (rtn >> 8)
            break

def _wait_for_result_win32(variants):
    handles = [v["proc"]._handle for v in variants if v["proc"].poll() is None]

    # On Windows it is only possible to wait on max. 64 processes at once
    #TODO: Replace with loop
    handles = handles[0:64]

    #logging.warning("Waiting for {}".format(handles))

    # If all processes have already ended do not wait
    if handles:
        multiprocessing.connection.wait(descriptors)

def wait_for_results(variants):
    if sys.platform == "win32":
        _wait_for_result_win32(variants)
    else:
        _wait_for_result_posix(variants)

def _kill_variant_posix(pid, no_setpgrp):
    if no_setpgrp:
        os.kill(pid, signal.SIGTERM)
    else:
        os.killpg(pid, signal.SIGTERM)

def _kill_variant_win32(pid, no_setpgrp):
    if no_setpgrp:
        subprocess.run(["TASKKILL", "/PID", str(pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        subprocess.run(["TASKKILL", "/T", "/PID", str(pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def kill_variants(variants, no_kill, no_setpgrp):
    for v in variants:
        proc = v["proc"]

        #logging.warning("Kill {}".format(v["proc"]._handle))

        if proc.poll() is None and not no_kill:
            if sys.platform == "win32":
                _kill_variant_win32(proc.pid, no_setpgrp)
            else:
                _kill_variant_posix(proc.pid, no_setpgrp)

        proc.wait()

    # Performs implicit cleanup of the temporary directories
    variants.clear()

def get_running_variants(variants):
    return [v for v in variants if v["proc"].poll() is None]

def get_successful_variants(variants):
    return [v for v in variants if v["proc"].poll() == 0]

def has_successful_variant(variants):
    return any(v["proc"].poll() == 0 for v in variants)
