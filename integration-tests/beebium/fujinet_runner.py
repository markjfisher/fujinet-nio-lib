from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Optional

_STDBUF = shutil.which("stdbuf")


class IsolatedFujinet:
    def __init__(
        self,
        binary: str | os.PathLike,
        *,
        pty_path: Optional[str] = None,
        extra_config: str = "",
        run_dir: Optional[str | os.PathLike] = None,
    ):
        self.binary = Path(binary)
        self.run_dir = Path(run_dir) if run_dir else Path(tempfile.mkdtemp(prefix="fn-lib-e2e-"))
        self.pty_path = pty_path or str(self.run_dir / "pty")
        self.extra_config = extra_config
        self._proc: Optional[subprocess.Popen] = None
        self._log = None

    def _write_config(self) -> None:
        data_dir = self.run_dir / "fujinet-data"
        data_dir.mkdir(parents=True, exist_ok=True)
        cfg = f"channel:\n  pty_path: {self.pty_path}\n"
        if self.extra_config:
            cfg += self.extra_config if self.extra_config.endswith("\n") else self.extra_config + "\n"
        (data_dir / "fujinet.yaml").write_text(cfg)

    def start(self, timeout: float = 10.0) -> "IsolatedFujinet":
        self._write_config()
        self._log = open(self.run_dir / "fujinet.log", "wb")
        cmd = [str(self.binary)]
        if _STDBUF:
            cmd = [_STDBUF, "-oL", "-eL", *cmd]
        self._proc = subprocess.Popen(
            cmd,
            cwd=str(self.run_dir),
            stdin=subprocess.DEVNULL,
            stdout=self._log,
            stderr=subprocess.STDOUT,
        )
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if os.path.islink(self.pty_path) or os.path.exists(self.pty_path):
                return self
            if self._proc.poll() is not None:
                raise RuntimeError(
                    f"fujinet exited early (rc={self._proc.returncode}); log:\n{self.log_text()[-2000:]}"
                )
            time.sleep(0.1)
        raise TimeoutError(
            f"fujinet did not create pty {self.pty_path!r} within {timeout}s; log:\n{self.log_text()[-2000:]}"
        )

    def log_text(self) -> str:
        try:
            return (self.run_dir / "fujinet.log").read_text(errors="replace")
        except OSError:
            return ""

    def wait_for_log(self, needle: str, timeout: float = 8.0, poll: float = 0.1) -> bool:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if needle in self.log_text():
                return True
            time.sleep(poll)
        return False

    def stop(self) -> None:
        if self._proc is not None and self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait(timeout=5)
        if self._log is not None:
            self._log.close()
            self._log = None

    def cleanup(self, keep: bool = False) -> None:
        self.stop()
        if not keep and self.run_dir.exists():
            shutil.rmtree(self.run_dir, ignore_errors=True)
