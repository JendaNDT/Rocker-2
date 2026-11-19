"""Kill the real writer at the OS boundary, then inspect the files from a new process."""
import json
import pathlib
import random
import subprocess
import sys
import tempfile
import time

binary, trial = sys.argv[1:]


def kill_writer(directory, mode, delay):
    with subprocess.Popen([binary, mode, str(directory)], stdout=subprocess.PIPE) as child:
        try:
            if child.stdout.readline().strip() != b"READY":
                raise RuntimeError("Writer failed to start")
            time.sleep(delay)
        finally:
            child.kill()
            child.wait()


with tempfile.TemporaryDirectory(prefix="rocker-e2-") as temp:
    project = pathlib.Path(temp)
    if trial == "A08":
        kill_writer(project, "record", 20.05)
        subprocess.run([binary, "recover", str(project)], check=True)
    elif trial == "A22":
        kill_writer(project, "midi", 0)
        subprocess.run([binary, "recoverMidi", str(project)], check=True)
    elif trial == "A33":
        kill_writer(project, "loop", 0)
        subprocess.run([binary, "recoverLoop", str(project)], check=True)
    elif trial == "A09":
        jitter = random.Random(112)
        for cycle in range(1000):
            kill_writer(project, "save", jitter.uniform(0, 0.004))
            song = json.loads((project / "skladba.json").read_text(encoding="utf-8-sig"))
            if song["nazev"] not in ("A", "B") or song["stopy"][0]["nazev"] != song["nazev"]:
                raise RuntimeError(f"Inconsistent save after kill {cycle}")
    else:
        raise ValueError(trial)
