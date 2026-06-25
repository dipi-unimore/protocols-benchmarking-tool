from __future__ import annotations

import sys
from datetime import datetime
from pathlib import Path

import click
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn, TaskProgressColumn
from rich.table import Table

from .comparator import IncompatibleRunsError, validate
from .plotter import generate_all
from .results import RunResult, scan

_console = Console()


def _build_table(runs: list[RunResult]) -> Table:
    table = Table(show_header=True, header_style="bold cyan")
    table.add_column("#", justify="right", style="dim", width=4)
    table.add_column("Label")
    table.add_column("Protocol")
    table.add_column("Serializer")
    table.add_column("Compression")
    table.add_column("Payload (B)", justify="right")
    table.add_column("Rate (Hz)", justify="right")
    table.add_column("Duration (s)", justify="right")
    table.add_column("Status")

    for i, run in enumerate(runs, 1):
        c = run.config
        status = "[green]ok[/green]" if run.complete else "[red]incomplete[/red]"
        table.add_row(
            str(i),
            run.label,
            c.get("protocol", "?"),
            c.get("serializer", "?"),
            c.get("compression", "?"),
            str(c.get("payload_size_bytes", "?")),
            str(c.get("message_rate_hz", "?")),
            str(c.get("duration_s", "?")),
            status,
        )
    return table


def _parse_selection(raw: str, runs: list[RunResult]) -> list[RunResult]:
    selected: list[RunResult] = []
    for part in raw.split(","):
        part = part.strip()
        if not part:
            continue
        try:
            idx = int(part)
        except ValueError:
            _console.print(f"[red]Invalid index: {part!r}[/red]")
            sys.exit(1)
        if idx < 1 or idx > len(runs):
            _console.print(f"[red]Index out of range: {idx}[/red]")
            sys.exit(1)
        run = runs[idx - 1]
        if not run.complete:
            _console.print(f"[red]Run #{idx} ({run.label}) is incomplete and cannot be selected.[/red]")
            sys.exit(1)
        selected.append(run)

    if not selected:
        _console.print("[red]No runs selected.[/red]")
        sys.exit(1)
    return selected


def _subfolder_name(runs: list[RunResult]) -> str:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    c = runs[0].config
    hz = c.get("message_rate_hz", "?")
    size = c.get("payload_size_bytes", "?")
    dur = c.get("duration_s", "?")
    return f"{ts}_{hz}hz_{size}B_{dur}s"


@click.command()
@click.option("--results-dir", type=click.Path(path_type=Path), default=None,
              help="Path to results directory (overrides PBT_OUTPUT_DIR env var).")
@click.option("--out-dir", type=click.Path(path_type=Path), default=Path("plots"),
              show_default=True, help="Parent directory for plot output.")
@click.option("--list-only", is_flag=True, default=False,
              help="Print available runs table and exit without plotting.")
def main(results_dir: Path | None, out_dir: Path, list_only: bool) -> None:
    runs = scan(results_dir)
    if not runs:
        _console.print("[yellow]No runs found.[/yellow]")
        sys.exit(0)

    _console.print(_build_table(runs))

    if list_only:
        sys.exit(0)

    raw = click.prompt("Select runs to compare (e.g. 1,3)")
    selected = _parse_selection(raw, runs)

    try:
        validate(selected)
    except IncompatibleRunsError as e:
        _console.print(f"[red]{e}[/red]")
        sys.exit(1)

    session_dir = out_dir / _subfolder_name(selected)
    session_dir.mkdir(parents=True, exist_ok=True)

    total_figures = 17
    generated: list[Path] = []

    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        TaskProgressColumn(),
        console=_console,
    ) as progress:
        task = progress.add_task("Generating figures...", total=total_figures)

        def on_figure(name: str) -> None:
            progress.update(task, advance=1, description=f"[cyan]{name}[/cyan]")

        generated = generate_all(selected, session_dir, progress_cb=on_figure)

    c = selected[0].config
    _console.print(f"\n[bold green]Done.[/bold green] {len(generated)} files → [underline]{session_dir}[/underline]")
    _console.print(
        f"  Common params: rate={c.get('message_rate_hz')} Hz  "
        f"payload={c.get('payload_size_bytes')} B  "
        f"duration={c.get('duration_s')} s"
    )
