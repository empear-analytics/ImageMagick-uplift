"""One stdio session with the CodeScene MCP server, held open until it has answered.

cs-mcp 1.5.12 stops reading as soon as its stdin closes and gives the requests
still running five seconds to finish ("timed out draining in-flight
responses"). Piping every request in and closing stdin at once therefore
loses any file that takes longer than that to score - MagickWand/magick-image.c
came back silent in two full sweeps. This keeps stdin open until every
request id has an answer, or until the deadline passes.
"""

from __future__ import annotations

import json
import os
import subprocess
import tempfile
import threading
from dataclasses import dataclass


@dataclass
class Session:
    """What one server invocation printed."""

    stdout: str
    stderr: str
    returncode: int
    timed_out: bool


def _answered_id(line: str) -> object:
    """The id of a JSON-RPC response on this line, or None."""
    try:
        msg = json.loads(line)
    except ValueError:
        return None
    return msg.get("id") if isinstance(msg, dict) else None


def _read_until_answered(proc: subprocess.Popen, waiting: set) -> list[str]:
    """Every stdout line, closing stdin once no id in waiting is outstanding."""
    lines = []
    for line in proc.stdout:
        lines.append(line)
        waiting.discard(_answered_id(line))
        if not waiting and not proc.stdin.closed:
            proc.stdin.close()
    return lines


def exchange(exe: str, requests: list[str], wanted: set, timeout: float) -> Session:
    """Send requests to exe and wait for an answer to every id in wanted."""
    env = dict(os.environ, CS_DISABLE_VERSION_CHECK="1")
    with tempfile.TemporaryFile(mode="w+", encoding="utf-8", errors="replace") as err:
        proc = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err,
                                text=True, encoding="utf-8", errors="replace", env=env)
        timer = threading.Timer(timeout, proc.kill)
        timer.start()
        try:
            proc.stdin.write("\n".join(requests) + "\n")
            proc.stdin.flush()
            lines = _read_until_answered(proc, set(wanted))
        finally:
            timed_out = not timer.is_alive()
            timer.cancel()
            if not proc.stdin.closed:
                proc.stdin.close()
            proc.wait()
        err.seek(0)
        return Session("".join(lines), err.read(), proc.returncode, timed_out)
