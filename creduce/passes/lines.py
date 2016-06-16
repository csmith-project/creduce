import logging
import shutil
import subprocess
import tempfile

from .delta import DeltaPass

class LinesDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return shutil.which("topformflat") is not None

    @classmethod
    def new(cls, test_case, arg):
        return {"start": 1}

    @classmethod
    def advance(cls, test_case, arg, state):
        new_state = state.copy()
        pos = new_state["index"]
        new_state["index"] -= new_state["chunk"]

        logging.debug("***ADVANCE*** from {} to {} with chunk {}".format(pos, new_state["index"], new_state["chunk"]))

        return new_state

    @classmethod
    def advance_on_success(cls, test_case, arg, state):
        return state

    @classmethod
    def transform(cls, test_case, arg, state):
        new_state = state.copy()

        if "start" in new_state:
            del new_state["start"]

            logging.debug("***TRANSFORM START***")

            with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
                with open(test_case, "r") as in_file:
                    try:
                        proc = subprocess.run(["topformflat", arg], stdin=in_file, stdout=subprocess.PIPE, universal_newlines=True)
                    except subprocess.SubprocessError:
                        return (DeltaPass.Result.error, new_state)

                for l in proc.stdout.splitlines(keepends=True):
                    if not l.isspace():
                        tmp_file.write(l)

            shutil.move(tmp_file.name, test_case)

            with open(test_case, "r") as in_file:
                data = in_file.readlines()

            new_state["index"] = len(data)
            new_state["chunk"] = len(data)
            return (DeltaPass.Result.ok, new_state)
        else:
            logging.debug("***TRANSFORM REGULAR chunk {} at {}***".format(new_state["chunk"], new_state["index"]));

            with open(test_case, "r") as in_file:
                data = in_file.readlines()

            while True:
                new_state["index"] = min(new_state["index"], len(data))

                if new_state["index"] >= 0 and len(data) > 0 and new_state["chunk"] > 0:
                    start = max(0, new_state["index"] - new_state["chunk"])
                    chunk = new_state["chunk"]
                    old_len = len(data)
                    data = data[0:start] + data[start + chunk:]

                    logging.debug("went from {} lines to {} with chunk {}".format(old_len, len(data), new_state["chunk"]))

                    with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
                        tmp_file.writelines(data)

                    shutil.move(tmp_file.name, test_case)
                    break
                else:
                    if new_state["chunk"] <= 1:
                        return (DeltaPass.Result.stop, new_state)

                    new_state["chunk"] = int(float(new_state["chunk"]) / 2.0)
                    new_state["index"] = len(data)

                    logging.debug("granularity reduced to {}".format(new_state["chunk"]))

            return (DeltaPass.Result.ok, new_state)
