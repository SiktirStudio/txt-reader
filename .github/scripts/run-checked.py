#!/usr/bin/env python3
"""Run a CI command and surface useful failures in GitHub check annotations."""
import collections
import os
import subprocess
import sys


def escape(message):
    return message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def main():
    recent = collections.deque(maxlen=1000)
    process = subprocess.Popen(sys.argv[1:], stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True,
                               encoding="utf-8", errors="replace")
    for line in process.stdout:
        print(line, end="", flush=True)
        recent.append(line.rstrip())
    code = process.wait()
    if code and os.environ.get("GITHUB_ACTIONS"):
        failures = [line for line in recent if any(word in line.lower()
                    for word in ("error", "[fail]", "failed", "not found", "fatal"))]
        for line in (failures or list(recent)[-20:])[:50]:
            print("::error::" + escape(line[:4000]), flush=True)
    return code


if __name__ == "__main__":
    sys.exit(main())
