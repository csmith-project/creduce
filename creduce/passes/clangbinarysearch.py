import logging
import math
import os
import re
import shutil
import subprocess
import tempfile

from . import AbstractPass
from ..utils import compat

class ClangBinarySearchPass(AbstractPass):
    def check_prerequisites(self):
        return shutil.which("clang_delta") is not None

    def new(self, test_case):
        instances = self.__count_instances(test_case)

        logging.debug("initial granularity = {}".format(instances))

        return {"chunk": instances, "instances": instances, "index": 1}

    def advance(self, test_case, state):
        new_state = state.copy()
        new_state["index"] = state["index"] + state["chunk"]

        logging.debug("ADVANCE: index = {}, chunk = {}".format(new_state["index"], new_state["chunk"]))

        return new_state

    def advance_on_success(self, test_case, state):
        return state.copy()

    def __count_instances(self, test_case):
        cmd = ["clang_delta", "--query-instances={}".format(self.arg), test_case]

        try:
            proc = compat.subprocess_run(cmd, universal_newlines=True, stdout=subprocess.PIPE)
        except subprocess.SubprocessError:
            return 0

        m = re.match("Available transformation instances: ([0-9]+)$", proc.stdout)

        if m is None:
            return 0
        else:
            return int(m.group(1))

    @staticmethod
    def __rechunk(state):
        if state["chunk"] < 10:
            return False

        def round_to_inf(num):
            return math.floor(num + 0.5)

        state["chunk"] = round_to_inf(state["chunk"] / 2.0)
        state["index"] = 1

        logging.debug("granularity = {}".format(state["chunk"]))

        return True

    def transform(self, test_case, state):
        new_state = state.copy()

        if new_state["index"] > new_state["instances"] and not self.__rechunk(new_state):
            return (self.Result.stop, new_state)

        # Only loops if we need to rechunk
        while True:
            logging.debug("TRANSFORM: index = {}, chunk = {}, instances = {}".format(new_state["index"], new_state["chunk"], new_state["instances"]))

            end = min(new_state["instances"], new_state["index"] + new_state["chunk"])

            with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
                cmd = ["clang_delta", "--transformation={}".format(self.arg), "--counter={}".format(new_state["index"]), "--to-counter={}".format(end), test_case]

                logging.debug(" ".join(cmd))

                try:
                    proc = compat.subprocess_run(cmd, universal_newlines=True, stdout=tmp_file)
                except subprocess.SubprocessError:
                    return (self.Result.error, new_state)

            if proc.returncode == 0:
                shutil.move(tmp_file.name, test_case)
                return (self.Result.ok, new_state)
            elif proc.returncode == 1:
                os.unlink(tmp_file.name)

                logging.debug("out of instances!")

                if not self.__rechunk(new_state):
                    return (self.Result.stop, new_state)
                else:
                    # Try again with new chunk size
                    continue
            else:
                os.unlink(tmp_file.name)
                return (self.Result.stop if proc.returncode == 255 else self.Result.error, new_state)
