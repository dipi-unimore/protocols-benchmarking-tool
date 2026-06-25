from __future__ import annotations

from rich.console import Console
from rich.table import Table

from .results import RunResult

_COMPARE_KEYS = ("message_rate_hz", "payload_size_bytes", "duration_s")
_console = Console(stderr=True)


class IncompatibleRunsError(Exception):
    pass


def validate(runs: list[RunResult]) -> None:
    if len(runs) < 2:
        return

    reference = runs[0]
    ref_vals = {k: reference.config.get(k) for k in _COMPARE_KEYS}
    mismatches: list[tuple[str, str, str, str]] = []

    for run in runs[1:]:
        for k in _COMPARE_KEYS:
            v = run.config.get(k)
            if v != ref_vals[k]:
                mismatches.append((k, reference.label, str(ref_vals[k]), f"{run.label}: {v}"))

    if not mismatches:
        return

    table = Table(title="Incompatible Runs", show_header=True, header_style="bold red")
    table.add_column("Parameter")
    table.add_column("Reference run")
    table.add_column("Reference value")
    table.add_column("Conflicting value")
    for m in mismatches:
        table.add_row(*m)

    _console.print(table)
    raise IncompatibleRunsError(
        f"Selected runs differ in: {', '.join(sorted({m[0] for m in mismatches}))}"
    )
