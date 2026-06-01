#!/usr/bin/env python3
"""Amalgamate occ_gordon sources into a single distributable header.

The generator expands a template that contains placeholders like:

  {{INLINE:src/occ_gordon/occ_gordon.cpp}}
  {{RAW:docs/snippet.txt}}
  {{LICENSE_BANNER}}

Local project includes are resolved recursively and emitted only once.
System includes are preserved as-is.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path


INCLUDE_RE = re.compile(r"^\s*#\s*include\s*([<\"])([^>\"]+)[>\"]\s*(?://.*)?$")
PRAGMA_ONCE_RE = re.compile(r"^\s*#\s*pragma\s+once\s*$")
PLACEHOLDER_RE = re.compile(r"\{\{([A-Z_]+)(?::([^}]+))?\}\}")


def normalize_posix(path: str) -> str:
    return Path(path).as_posix()


def relpath(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.resolve().as_posix()


@dataclass
class Amalgamator:
    project_root: Path
    include_roots: list[Path]
    skip_includes: set[str] = field(default_factory=set)
    _expanded: set[Path] = field(default_factory=set, init=False)
    _stack: list[Path] = field(default_factory=list, init=False)

    def resolve_include(self, including_file: Path, include_name: str) -> Path | None:
        relative_candidate = including_file.parent / include_name
        if relative_candidate.exists():
            return relative_candidate.resolve()

        for root in self.include_roots:
            candidate = root / include_name
            if candidate.exists():
                return candidate.resolve()

        return None

    def expand_file(self, path: Path) -> str:
        resolved = path.resolve()
        if resolved in self._stack:
            chain = " -> ".join(relpath(p, self.project_root) for p in self._stack + [resolved])
            raise RuntimeError(f"Include cycle detected: {chain}")

        if resolved in self._expanded:
            return ""

        self._expanded.add(resolved)
        self._stack.append(resolved)

        try:
            text = resolved.read_text(encoding="utf-8")
        except OSError as exc:
            raise RuntimeError(f"Failed to read {resolved}: {exc}") from exc

        if resolved.suffix == ".cpp":
            text = self._inline_cpp_definitions(text)

        output: list[str] = []
        output.append(f"// BEGIN FILE: {relpath(resolved, self.project_root)}\n")

        try:
            for line in text.splitlines(keepends=True):
                if PRAGMA_ONCE_RE.match(line):
                    continue

                match = INCLUDE_RE.match(line)
                if match:
                    include_name = match.group(2).strip()
                    if normalize_posix(include_name) in self.skip_includes:
                        continue

                    target = self.resolve_include(resolved, include_name)
                    if target is not None and self._is_project_file(target):
                        output.append(self.expand_file(target))
                        continue

                output.append(line)

            output.append(f"// END FILE: {relpath(resolved, self.project_root)}\n")
            return "".join(output)
        finally:
            self._stack.pop()

    def _prefix_inline(self, line: str) -> str:
        stripped = line.lstrip()
        if stripped.startswith("inline "):
            return line
        indent = line[: len(line) - len(stripped)]
        return f"{indent}inline {stripped}"

    def _is_cpp_variable_definition(self, line: str) -> bool:
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", "//", "/*", "*", "*/")):
            return False
        if "(" in stripped or "{" in stripped or "}" in stripped:
            return False
        if "::" not in stripped or "=" not in stripped or not stripped.endswith(";"):
            return False
        if stripped.startswith(("namespace ", "using ", "typedef ", "class ", "struct ", "enum ", "template ")):
            return False
        return True

    def _looks_like_cpp_function_start(self, line: str) -> bool:
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", "//", "/*", "*", "*/")):
            return False
        if stripped.startswith(("namespace ", "using ", "typedef ", "class ", "struct ", "enum ", "template ")):
            return False
        if "(" not in stripped or stripped.endswith(";"):
            return False
        if stripped.startswith(("if ", "for ", "while ", "switch ", "catch ")):
            return False
        return True

    def _find_cpp_function_block_end(self, lines: list[str], start: int) -> int | None:
        for idx in range(start, len(lines)):
            stripped = lines[idx].strip()
            if idx > start and stripped.endswith(";"):
                return None
            if "{" in lines[idx]:
                return idx
        return None

    def _inline_cpp_definitions(self, text: str) -> str:
        """Make .cpp definitions safe to include from multiple translation units."""

        lines = text.splitlines(keepends=True)
        result: list[str] = []
        i = 0
        brace_depth = 0
        pending_template = False

        while i < len(lines):
            line = lines[i]
            stripped = line.strip()

            if brace_depth <= 1 and stripped.startswith("template"):
                pending_template = True
                result.append(line)
                i += 1
                continue

            if brace_depth <= 1 and self._is_cpp_variable_definition(line):
                result.append(self._prefix_inline(line))
                brace_depth += line.count("{") - line.count("}")
                pending_template = False
                i += 1
                continue

            if brace_depth <= 1 and (pending_template or self._looks_like_cpp_function_start(line)):
                block_end = self._find_cpp_function_block_end(lines, i)
                if block_end is not None:
                    result.append(self._prefix_inline(lines[i]))
                    result.extend(lines[i + 1 : block_end + 1])
                    for block_idx in range(i, block_end + 1):
                        brace_depth += lines[block_idx].count("{") - lines[block_idx].count("}")
                    pending_template = False
                    i = block_end + 1
                    continue

            result.append(line)
            brace_depth += line.count("{") - line.count("}")
            if stripped and not stripped.startswith(("//", "/*", "*", "*/", "#")):
                pending_template = False
            i += 1

        return "".join(result)

    def _is_project_file(self, path: Path) -> bool:
        resolved = path.resolve()
        try:
            resolved.relative_to(self.project_root.resolve())
            return True
        except ValueError:
            return False

    def render_placeholder(self, token: str, argument: str | None) -> str:
        if token == "LICENSE_BANNER":
            return (
                "/*\n"
                " * SPDX-License-Identifier: Apache-2.0\n"
                " * Generated by tools/amalgamate.py from a template.\n"
                " * Do not edit this file directly.\n"
                " */\n"
            )

        if token == "INLINE":
            if not argument:
                raise RuntimeError("INLINE placeholder requires a path argument")
            target = (self.project_root / argument).resolve()
            if not target.exists():
                raise RuntimeError(f"INLINE target not found: {argument}")
            return self.expand_file(target)

        if token == "INLINE_CPP_TREE":
            if not argument:
                raise RuntimeError("INLINE_CPP_TREE placeholder requires a path argument")
            root = (self.project_root / argument).resolve()
            if not root.exists() or not root.is_dir():
                raise RuntimeError(f"INLINE_CPP_TREE target is not a directory: {argument}")

            cpp_files = sorted(
                (path for path in root.rglob("*.cpp") if path.is_file()),
                key=lambda path: path.resolve().relative_to(self.project_root.resolve()).as_posix(),
            )

            return "".join(self.expand_file(path) for path in cpp_files)

        if token == "RAW":
            if not argument:
                raise RuntimeError("RAW placeholder requires a path argument")
            target = (self.project_root / argument).resolve()
            if not target.exists():
                raise RuntimeError(f"RAW target not found: {argument}")
            return target.read_text(encoding="utf-8")

        raise RuntimeError(f"Unknown placeholder: {token}")

    def render_template(self, template: Path) -> str:
        raw = template.read_text(encoding="utf-8")

        def replace(match: re.Match[str]) -> str:
            token = match.group(1)
            argument = match.group(2)
            return self.render_placeholder(token, argument)

        return PLACEHOLDER_RE.sub(replace, raw)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Repository root used to resolve template placeholders and local includes.",
    )
    parser.add_argument(
        "--template",
        type=Path,
        required=True,
        help="Template file containing amalgamation placeholders.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output header path.",
    )
    parser.add_argument(
        "--include-root",
        type=Path,
        action="append",
        default=None,
        help="Additional root used when resolving local includes.",
    )
    parser.add_argument(
        "--skip-include",
        action="append",
        default=None,
        help="Include path to omit from the generated output.",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    project_root = args.project_root.resolve()
    include_roots = [project_root]
    include_roots.append(project_root / "src")
    if args.include_root:
        include_roots.extend(path.resolve() for path in args.include_root)

    skip_includes = {normalize_posix("occ_gordon/exports.h")}
    if args.skip_include:
        skip_includes.update(normalize_posix(value) for value in args.skip_include)

    amalgamator = Amalgamator(
        project_root=project_root,
        include_roots=include_roots,
        skip_includes=skip_includes,
    )

    rendered = amalgamator.render_template(args.template.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(rendered.replace("\r\n", "\n"), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
