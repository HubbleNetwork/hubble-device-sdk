"""pytest configuration for the riglink BLE test harness.

Without --riglink-port, builds and launches the tests/riglink app on
native_sim and connects riglink to its pseudotty. With --riglink-port,
connects to an already-flashed device (e.g. an nrf52840dk).
"""
import os
import re
import select
import subprocess
import sys
import threading
import time

import pytest
import riglink

_HERE = os.path.dirname(__file__)
_APP_DIR = os.path.abspath(os.path.join(_HERE, ".."))
_TOOLS = os.path.abspath(os.path.join(_HERE, "..", "..", "..", "tools"))
if _TOOLS not in sys.path:
    sys.path.insert(0, _TOOLS)

_PTY_RE = re.compile(r"connected to pseudotty:\s*(\S+)")
_BAUD = 115200
# The firmware uses riglink's shell backend, so commands are dispatched as
# subcommands of this root command; shell_root routes outbound calls to it.
_SHELL_ROOT = "rig"


# --riglink-port / --riglink-baud are registered by the always-installed
# riglink pytest plugin (riglink.pytest_plugin), so we deliberately do NOT
# re-register them -- doing so collides with the plugin and aborts collection.
# We only read them. The plugin makes --riglink-port an append-action option
# (dest "riglink_port"), so getoption() returns a list; we take the first port.

def _riglink_port(config):
    """First configured serial port, or None when running on native_sim.

    Falls back to None if the plugin is somehow absent so the no-port
    (native_sim) auto-build path still works.
    """
    try:
        val = config.getoption("--riglink-port")
    except (ValueError, KeyError):
        return None
    if not val:
        return None
    if isinstance(val, (list, tuple)):
        return val[0] if val else None
    return val


def _riglink_baud(config):
    try:
        val = config.getoption("--riglink-baud")
    except (ValueError, KeyError):
        return _BAUD
    return val if val else _BAUD


class _NativeSim:
    """Builds, launches, and connects to the tests/riglink app on native_sim."""

    def __init__(self):
        self._build_dir = os.path.join(_APP_DIR, "build-native_sim")
        self.proc = None
        self.dev = None
        self._drain = None

    def build(self):
        subprocess.run(
            ["west", "build", "-b", "native_sim/native/64", "-d", self._build_dir,
             _APP_DIR],
            check=True,
        )

    def launch(self):
        exe = os.path.join(self._build_dir, "zephyr", "zephyr.exe")
        self.proc = subprocess.Popen(
            [exe], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1,
        )
        # native_sim opens uart0 (console) first, then uart1 (riglink's PTY),
        # emitting one "connected to pseudotty:" line per UART. We want the
        # riglink UART, which is the last one reported, so keep scanning until
        # a brief quiet period after the latest match (mirrors riglink's own
        # reference harness) rather than breaking on the first line.
        # The 0.25s quiet window and 0.05s poll interval are heuristic timing
        # values (not protocol-derived) and may be tuned if startup is slower.
        deadline = time.monotonic() + 10.0
        pty = None
        last_match_at = None
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.proc.stdout], [], [], 0.05)
            if not ready:
                if (pty is not None and last_match_at is not None
                        and time.monotonic() - last_match_at > 0.25):
                    break
                if self.proc.poll() is not None:
                    break
                continue
            line = self.proc.stdout.readline()
            if not line:
                if self.proc.poll() is not None:
                    break
                continue
            m = _PTY_RE.search(line)
            if m:
                pty = m.group(1)
                last_match_at = time.monotonic()
        if not pty:
            self.teardown()
            pytest.skip("native_sim did not report a pseudotty")
        # Keep draining stdout so the OS pipe never fills and backpressures
        # (which would stall the simulator). riglink drives I/O over the PTY,
        # so the captured stdout is only diagnostic console output.
        self._drain = threading.Thread(target=self._drain_stdout, daemon=True)
        self._drain.start()
        time.sleep(0.2)  # let the PTY settle
        self.dev = riglink.connect(pty, _BAUD, shell_root=_SHELL_ROOT)

    def _drain_stdout(self):
        try:
            for _ in iter(self.proc.stdout.readline, ""):
                pass  # discard; loop ends at EOF when the process exits
        except Exception:
            pass  # swallow errors raised by pipe close during teardown

    def teardown(self):
        if self.dev is not None:
            self.dev.close()
            self.dev = None
        if self.proc is not None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None
        # Daemon thread ends on stdout EOF once the process is gone; no join.
        self._drain = None


@pytest.fixture(scope="session")
def _device(request):
    port = _riglink_port(request.config)
    if port:
        baud = _riglink_baud(request.config)
        dev = riglink.connect(port, baud, shell_root=_SHELL_ROOT)
        yield dev
        dev.close()
    else:
        sim = _NativeSim()
        sim.build()
        sim.launch()
        yield sim.dev
        sim.teardown()


@pytest.fixture
def dev(_device):
    # Every test sets key/uptime/seq explicitly, so tests are self-contained;
    # just clear any buffered bytes from a prior test.
    _device.clear_buffers()
    return _device
