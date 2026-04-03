# Copyright (c) 2025 Hubble Network, Inc.
# SPDX-License-Identifier: Apache-2.0

"""
Kconfig Autodoc Extension
~~~~~~~~~~~~~~~~~~~~~~~~~

Sphinx extension that generates configuration reference documentation from
Kconfig files using kconfiglib.

Usage in RST::

    .. kconfig-autodoc:: path/to/Kconfig

Configuration values (set in conf.py):

- ``kconfig_srctree``: Path to the Zephyr base directory for resolving
  ``source`` directives. Defaults to ``ZEPHYR_BASE`` env var or auto-detects
  ``../../zephyr`` relative to the documentation source directory.
"""

import logging
import os
import re
import tempfile
from pathlib import Path

import kconfiglib
from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective

logger = logging.getLogger(__name__)

_SOURCE_RE = re.compile(r"^source\s+", re.MULTILINE)


def _resolve_srctree(app):
    """Resolve the Zephyr base directory (srctree) for Kconfig source lookups."""
    configured = app.config.kconfig_srctree
    if configured and Path(configured).is_dir():
        return str(configured)

    env_val = os.environ.get("ZEPHYR_BASE")
    if env_val and Path(env_val).is_dir():
        return env_val

    # Auto-detect from docs source dir: docs/ -> SDK root -> ../../zephyr
    sdk_root = Path(app.srcdir).resolve().parent
    candidates = [
        sdk_root.parent.parent / "zephyr",
        sdk_root.parent / "zephyr",
    ]
    for candidate in candidates:
        if candidate.is_dir():
            return str(candidate)

    return None


def _load_kconfig(kconfig_path, srctree=None):
    """Parse a Kconfig file with kconfiglib.

    Sets up the environment so ``source`` directives can resolve against the
    Zephyr tree.  Falls back to replacing ``source`` with ``osource`` when the
    referenced files cannot be found.
    """
    env_backup = {}
    keys_to_set = {}

    if srctree:
        keys_to_set["srctree"] = srctree
        keys_to_set["ZEPHYR_BASE"] = srctree

    # Module variables used by Zephyr logging Kconfig template
    keys_to_set["KCONFIG_DOC_MODE"] = "1"

    try:
        for key, val in keys_to_set.items():
            env_backup[key] = os.environ.get(key)
            os.environ[key] = val

        try:
            return kconfiglib.Kconfig(str(kconfig_path), warn=False)
        except (kconfiglib.KconfigError, FileNotFoundError, OSError):
            logger.warning(
                "kconfig-autodoc: failed to parse %s with full source "
                "resolution, falling back to osource replacement",
                kconfig_path,
            )
            return _load_kconfig_with_osource(kconfig_path)
    finally:
        for key, original in env_backup.items():
            if original is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = original


def _load_kconfig_with_osource(kconfig_path):
    """Create a temp copy of the Kconfig with ``source`` replaced by
    ``osource`` so missing files are silently skipped."""
    kconfig_path = Path(kconfig_path)
    content = _SOURCE_RE.sub("osource ", kconfig_path.read_text())

    # kconfiglib parses the file fully at construction time, so the temp
    # directory can be cleaned up immediately after Kconfig() returns.
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_path = Path(tmp_dir) / kconfig_path.name
        tmp_path.write_text(content)
        return kconfiglib.Kconfig(str(tmp_path), warn=False)


def _dep_expr_str(expr):
    """Convert a kconfiglib dependency expression to a readable string."""
    if expr is None:
        return None
    s = kconfiglib.expr_str(expr)
    if s in ("y", "n", "m"):
        return None
    return s


def _default_str(sym):
    """Get the first default value for a symbol.

    Strips the condition if it matches the symbol's direct dependency
    (already shown in the "Depends on" field).
    """
    if not sym.defaults:
        return None
    node_dep_str = _dep_expr_str(sym.nodes[0].dep) if sym.nodes else None
    default, cond = sym.defaults[0]
    val = kconfiglib.expr_str(default)
    cond_str = _dep_expr_str(cond)
    if cond_str and cond_str != node_dep_str:
        return f"{val} (if {cond_str})"
    return val


def _range_str(sym):
    """Get range constraints for int/hex symbols."""
    if not sym.ranges:
        return None
    low, high, _cond = sym.ranges[0]
    return f"{kconfiglib.expr_str(low)} to {kconfiglib.expr_str(high)}"


def _collect_nodes(node):
    """Walk the Kconfig menu tree depth-first, returning a list of nodes."""
    result = []

    def _walk(n):
        while n:
            result.append(n)
            if n.list:
                _walk(n.list)
            n = n.next

    if node.list:
        _walk(node.list)
    return result


class KconfigAutoDoc(SphinxDirective):
    """Directive ``.. kconfig-autodoc:: path/to/Kconfig``

    Parses the referenced Kconfig file and generates documentation for all
    configuration options found.
    """

    has_content = False
    required_arguments = 1
    optional_arguments = 0
    option_spec = {
        "prefix": directives.unchanged,
    }

    def run(self):
        prefix = self.options.get("prefix", "CONFIG_")
        kconfig_rel = self.arguments[0]

        # Resolve path relative to the RST source file
        source_dir = Path(self.state.document.settings.env.srcdir)
        kconfig_path = (source_dir / kconfig_rel).resolve()

        if not kconfig_path.is_file():
            raise self.error(f"Kconfig file not found: {kconfig_path}")

        # Register the Kconfig file as a dependency so Sphinx rebuilds when it changes
        self.state.document.settings.env.note_dependency(str(kconfig_path))

        srctree = _resolve_srctree(self.env.app)
        kconfig = _load_kconfig(kconfig_path, srctree)

        return self._build_doc(kconfig, prefix)

    def _build_doc(self, kconfig, prefix):
        """Build the docutils node tree from parsed Kconfig."""
        result_nodes = []
        processed_choices = set()

        for knode in _collect_nodes(kconfig.top_node):
            item = knode.item

            if item is kconfig:
                continue

            if isinstance(item, kconfiglib.Choice):
                if id(item) not in processed_choices:
                    processed_choices.add(id(item))
                    result_nodes.extend(self._render_choice(item, prefix))
                continue

            if isinstance(item, kconfiglib.Symbol):
                if item.choice is not None:
                    continue
                if not item.nodes or not item.nodes[0].prompt:
                    continue
                result_nodes.extend(self._render_symbol(item, prefix))
                continue

            if item == kconfiglib.MENU and knode.prompt:
                if knode.list:
                    result_nodes.append(self._make_section(knode.prompt[0]))

        return result_nodes

    def _make_section(self, title):
        """Create a section node with the given title."""
        section = nodes.section()
        section["ids"] = [nodes.make_id(title)]
        title_node = nodes.title(text=title)
        section += title_node
        return section

    def _render_choice(self, choice, prefix):
        """Render a choice group and its member symbols."""
        prompt = choice.nodes[0].prompt[0] if choice.nodes and choice.nodes[0].prompt else "Choice"
        container = nodes.container(classes=["kconfig-choice"])
        container += nodes.rubric(text=prompt)

        choice_note = nodes.paragraph()
        choice_note += nodes.emphasis(text="Choice — select one of:")
        container += choice_note

        dl = nodes.definition_list(classes=["kconfig-options"])
        for sym in choice.syms:
            if not sym.nodes or not sym.nodes[0].prompt:
                continue
            dl += self._render_choice_symbol(sym, prefix, choice)
        container += dl

        return [container]

    def _render_choice_symbol(self, sym, prefix, choice):
        """Render a single symbol within a choice group."""
        name = f"{prefix}{sym.name}"
        prompt = sym.nodes[0].prompt[0] if sym.nodes and sym.nodes[0].prompt else ""
        is_default = any(
            isinstance(d, kconfiglib.Symbol) and d is sym
            for d, _c in choice.defaults
        )

        dli = nodes.definition_list_item()
        term = nodes.term()

        target = nodes.target("", "", ids=[nodes.make_id(name)])
        self.state.document.note_explicit_target(target)
        term += target

        term += nodes.strong(text=name)
        if is_default:
            term += nodes.inline(text=" ")
            term += nodes.emphasis(text="[default]")
        dli += term

        definition = nodes.definition()
        if prompt:
            prompt_para = nodes.paragraph()
            prompt_para += nodes.inline(text=prompt)
            definition += prompt_para

        help_text = sym.nodes[0].help if sym.nodes else None
        if help_text:
            definition += nodes.paragraph(text=help_text.strip())

        dli += definition
        return dli

    def _render_symbol(self, sym, prefix):
        """Render a standalone config symbol (not part of a choice)."""
        name = f"{prefix}{sym.name}"
        prompt = sym.nodes[0].prompt[0] if sym.nodes and sym.nodes[0].prompt else ""

        container = nodes.container(classes=["kconfig-option"])

        target = nodes.target("", "", ids=[nodes.make_id(name)])
        self.state.document.note_explicit_target(target)
        container += target
        container += nodes.rubric(text=name)

        if prompt:
            prompt_para = nodes.paragraph()
            prompt_para += nodes.emphasis(text=prompt)
            container += prompt_para

        fl = nodes.field_list()
        fl += self._make_field("Type", kconfiglib.TYPE_TO_STR.get(sym.orig_type, "unknown"))

        default = _default_str(sym)
        if default:
            fl += self._make_field("Default", default)

        range_val = _range_str(sym)
        if range_val:
            fl += self._make_field("Range", range_val)

        for knode in sym.nodes:
            dep = _dep_expr_str(knode.dep)
            if dep:
                fl += self._make_field("Depends on", dep)
                break

        if fl.children:
            container += fl

        help_text = sym.nodes[0].help if sym.nodes else None
        if help_text:
            container += nodes.paragraph(text=help_text.strip())

        return [container]

    def _make_field(self, name, value):
        """Create a field list entry."""
        field = nodes.field()
        field += nodes.field_name(text=name)
        body = nodes.field_body()
        body += nodes.paragraph(text=str(value))
        field += body
        return field


def setup(app):
    app.add_config_value("kconfig_srctree", None, "env")
    app.add_directive("kconfig-autodoc", KconfigAutoDoc)
    return {
        "version": "0.1.0",
        "parallel_read_safe": False,
        "parallel_write_safe": True,
    }
