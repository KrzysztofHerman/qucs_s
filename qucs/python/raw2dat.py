"""Convert an ngspice raw file into a Qucs `.dat` dataset.

The converter is intentionally lightweight so it can be used directly from
Python or as a command-line script::

    python raw2dat.py input.raw output.dat

Only the core ngspice raw features are supported (real/complex values,
ASCII/Binary storage). The first variable in the raw file is treated as the
independent axis; all following variables are exported as dependents.
"""
from __future__ import annotations

import argparse
import io
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import List, Sequence

_VERSION_FILE = Path(__file__).resolve().parent.parent / "VERSION"


@dataclass
class RawVariable:
    name: str
    type_name: str


@dataclass
class RawData:
    variables: List[RawVariable]
    points: List[List[complex]]
    is_complex: bool


_FLOAT_RE = re.compile(r"[-+]?(?:\d+\.\d*|\d*\.\d+|\d+)(?:[eE][-+]?\d+)?")


def _read_header(stream: io.BufferedReader) -> tuple[List[RawVariable], int, bool, str]:
    """Read the text header of the raw file.

    Returns a tuple with the variable metadata, number of points, whether the
    dataset is complex and the section marker ("Values" or "Binary").
    """
    variables: List[RawVariable] = []
    nvars = 0
    npoints = 0
    is_complex = False
    section: str | None = None

    while True:
        line = stream.readline()
        if not line:
            break
        text = line.decode(errors="ignore").strip()
        lower = text.lower()
        if lower.startswith("flags:"):
            is_complex = "complex" in lower
        elif lower.startswith("no. variables:"):
            nvars = int(text.split()[2])
        elif lower.startswith("no. points:"):
            npoints = int(text.split()[2])
        elif text == "Variables:":
            for _ in range(nvars):
                var_line = stream.readline().decode(errors="ignore").strip()
                parts = var_line.split()
                if len(parts) >= 3:
                    variables.append(RawVariable(parts[1], parts[2]))
            continue
        elif text in {"Values:", "Binary:"}:
            section = text.rstrip(":")
            break
    if section is None:
        raise ValueError("Unable to locate data section in raw file")
    if not variables or npoints == 0:
        raise ValueError("Raw header is missing variable or point information")
    return variables, npoints, is_complex, section


def _parse_ascii(stream: io.BufferedReader, variables: Sequence[RawVariable], npoints: int, is_complex: bool) -> List[List[complex]]:
    payload = stream.read().decode(errors="ignore")
    numbers = [float(match.group()) for match in _FLOAT_RE.finditer(payload)]
    expected = npoints * (1 + (2 if is_complex else 1) * len(variables))
    if len(numbers) < expected:
        raise ValueError(f"Unexpected end of data: expected {expected} numeric entries, got {len(numbers)}")

    points: List[List[complex]] = []
    cursor = 0
    stride = 1 + (2 if is_complex else 1) * len(variables)
    for _ in range(npoints):
        slice_ = numbers[cursor:cursor + stride]
        cursor += stride
        values = slice_[1:]
        point: List[complex] = []
        for idx, _ in enumerate(variables):
            if is_complex:
                real = values[2 * idx]
                imag = values[2 * idx + 1]
                point.append(complex(real, imag))
            else:
                point.append(complex(values[idx], 0.0))
        points.append(point)
    return points


def _parse_binary(stream: io.BufferedReader, variables: Sequence[RawVariable], npoints: int, is_complex: bool) -> List[List[complex]]:
    count_per_point = len(variables) * (2 if is_complex else 1)
    fmt = f"<{count_per_point}d"
    chunk_size = struct.calcsize(fmt)

    points: List[List[complex]] = []
    for _ in range(npoints):
        raw_chunk = stream.read(chunk_size)
        if len(raw_chunk) != chunk_size:
            raise ValueError("Binary data truncated before all points were read")
        values = struct.unpack(fmt, raw_chunk)
        point: List[complex] = []
        if is_complex:
            for idx in range(0, len(values), 2):
                point.append(complex(values[idx], values[idx + 1]))
        else:
            for val in values:
                point.append(complex(val, 0.0))
        points.append(point)
    return points


def read_raw(path: Path) -> RawData:
    with path.open("rb") as stream:
        variables, npoints, is_complex, section = _read_header(stream)
        if section == "Values":
            points = _parse_ascii(stream, variables, npoints, is_complex)
        else:
            points = _parse_binary(stream, variables, npoints, is_complex)
    return RawData(variables=variables, points=points, is_complex=is_complex)


def _format_complex(value: complex) -> str:
    imag = abs(value.imag)
    sign = "+" if value.imag >= 0 else "-"
    return f"{value.real:.12e}{sign}j{imag:.12e}"


def write_dat(data: RawData, destination: Path) -> None:
    try:
        version = _VERSION_FILE.read_text(encoding="utf-8").strip()
    except OSError:
        version = "unknown"

    lines = [f"<Qucs Dataset {version}>\n"]

    indep_name = data.variables[0].name
    lines.append(f"<indep {indep_name} {len(data.points)}>\n")
    for point in data.points:
        lines.append(f"{point[0].real:.12e}\n")
    lines.append("</indep>\n")

    indep_label = indep_name
    for var_index, var in enumerate(data.variables[1:], start=1):
        lines.append(f"<dep {var.name} {indep_label}>\n")
        for point in data.points:
            value = point[var_index]
            if data.is_complex:
                lines.append(f"{_format_complex(value)}\n")
            else:
                lines.append(f"{value.real:.12e}\n")
        lines.append("</dep>\n")

    destination.write_text("".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Convert ngspice raw files to Qucs datasets")
    parser.add_argument("rawfile", type=Path, help="Input ngspice raw file")
    parser.add_argument("output", type=Path, nargs="?", help="Output Qucs .dat file")
    args = parser.parse_args()

    output = args.output
    if output is None:
        output = args.rawfile.with_suffix(args.rawfile.suffix + ".dat")

    raw_data = read_raw(args.rawfile)
    write_dat(raw_data, output)


if __name__ == "__main__":
    main()
