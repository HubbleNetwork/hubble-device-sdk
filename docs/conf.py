# Hubble Network SDK build configuration file.
#
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import os
from pathlib import Path

# -- Project information -----------------------------------------------------

project = "Hubble Network SDK"
copyright = "2025, Hubble Network, Inc"
author = "HubbleNetwork"
release = "0.1"

sys.path.insert(0,  "./_extensions")

# -- Kconfig autodoc ---------------------------------------------------------
# Resolve Zephyr base for Kconfig source directives
kconfig_srctree = os.environ.get(
    "ZEPHYR_BASE",
    str(Path(__file__).resolve().parent.parent.parent / "zephyr")
)

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
        "sphinx_rtd_theme",
        "sphinx_sitemap",
        "sphinx.ext.autodoc",
        "sphinx.ext.extlinks",
        "sphinx.ext.graphviz",
        "sphinx_tabs.tabs",
        "sphinxcontrib.jquery",
        "sphinx_togglebutton",
        "sphinx_copybutton",
        "breathe",
        "kconfig_autodoc",
]

templates_path = ["templates"]
exclude_patterns = []

# Setup the breathe extension
breathe_projects = {
    "HubbleNetworkSDK": "./_doxygen/xml"
}
breathe_default_project = "HubbleNetworkSDK"

# Tell sphinx what the primary language being documented is.
primary_domain = "c"

# Tell sphinx what the pygments highlight language should be.
pygments_sytle = "sphinx"
highlight_language = "none"

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_title = "Hubble Network SDK Documentation"
html_logo = "static/images/hubble-logo.svg"
html_favicon = "static/images/hubble.png"
html_split_index = True
html_show_sourcelink = False
html_show_sphinx = False
html_domain_indices = False
html_static_path = ["static"]
# html_baseurl is required for sphinx-sitemap
# Defaults to localhost for local builds
# Set SPHINX_BASEURL environment variable to override (e.g., for production/CI)
html_baseurl = os.environ.get(
    "SPHINX_BASEURL",
    "http://localhost:8000/"
)
html_theme_options = {
    "collapse_navigation": False,
    'navigation_depth': 3,
    }

# -- Multi-version support ---------------------------------------------------
# DOCS_BRANCH and GITHUB_REPOSITORY_NAME are set by CI.
_docs_branch = os.environ.get("DOCS_BRANCH", "")
_repo_name = os.environ.get("GITHUB_REPOSITORY_NAME", "")

html_context = {
    "is_release": _docs_branch.startswith("release"),
    "docs_base_path": f"/{_repo_name}/" if _repo_name else "/",
}

def setup(app):
    # theme customizations
    app.add_css_file("css/custom.css")
