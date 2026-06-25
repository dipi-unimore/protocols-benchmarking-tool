from __future__ import annotations

import json
import os
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import pandas as pd


@dataclass
class RunResult:
    folder: Path
    label: str
    config: dict[str, Any]
    summary: dict[str, Any] | None
    _packets: pd.DataFrame | None = field(default=None, repr=False, compare=False)

    @property
    def complete(self) -> bool:
        return self.summary is not None and (self.folder / "packets.csv").exists()

    def load_packets(self) -> pd.DataFrame:
        if self._packets is None:
            self._packets = pd.read_csv(self.folder / "packets.csv")
        return self._packets

    @property
    def legend(self) -> str:
        c = self.config
        return f"{c.get('protocol','?')}/{c.get('serializer','?')}/{c.get('compression','?')}"


def _resolve_results_dir(results_dir: Path | None) -> Path:
    if results_dir is not None:
        return results_dir
    env = os.environ.get("PBT_OUTPUT_DIR")
    if env:
        return Path(env)
    return Path(__file__).parent.parent.parent.parent / "results"


def scan(results_dir: Path | None = None) -> list[RunResult]:
    root = _resolve_results_dir(results_dir)
    if not root.exists():
        return []

    runs: list[RunResult] = []
    for entry in sorted(root.iterdir()):
        if not entry.is_dir():
            continue
        cfg_path = entry / "config.json"
        sum_path = entry / "summary.json"
        pkt_path = entry / "packets.csv"

        if not cfg_path.exists():
            continue

        config = json.loads(cfg_path.read_text())
        summary = json.loads(sum_path.read_text()) if sum_path.exists() else None
        complete = summary is not None and pkt_path.exists()

        if not complete:
            summary = None

        runs.append(RunResult(
            folder=entry,
            label=entry.name,
            config=config,
            summary=summary,
        ))

    runs.sort(key=lambda r: (
        r.config.get("protocol", ""),
        r.config.get("serializer", ""),
        r.config.get("compression", ""),
    ))
    return runs
