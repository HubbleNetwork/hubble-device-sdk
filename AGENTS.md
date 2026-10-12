<!--
Copyright (c) 2026 Hubble Network, Inc.

SPDX-License-Identifier: Apache-2.0
-->

# Hubble Device SDK: instructions for AI coding agents

This file is for coding agents, and for the humans driving them, working in this repository.
It is a digest of `CONTRIBUTING.md`, `docs/architecture/project-organization.rst` and the
checks in `.github/workflows/`. Where it is silent or disagrees with those, they win. Read the
relevant page before working in an area you have not touched before; do not rely on this digest
or on training data alone.

## Rules for agents

- Every commit must carry a `Signed-off-by:` line with the full name of the human responsible
  for the work (gitlint rule `UC1` in `tools/ci/gitlint/rules.py`). Never add, edit or copy one
  yourself: only a human can certify the DCO, and the submitter adds their own
  (`git commit -s`). Never add `Co-authored-by:`. Do not put "Generated with ...", session
  links or any other mention of AI tools in commit messages, PR bodies, issues or comments. The
  `Assisted-by:` trailer below is the only place AI involvement is recorded.
- Add one `Assisted-by: <Agent>:<model-version>` trailer, for example
  `Assisted-by: Claude:claude-opus-4.6`.
- The human reviews and tests every change before it is submitted. State what you did not do
  (not built, not run, not run on hardware, no reproducer) instead of implying it was done.
  Radio, FEM and satellite timing changes in `port/*/boards/` cannot be validated on
  `native_sim`; say so.
- Commit messages, code comments, Kconfig help texts and docs describe the tree as it is: no
  references to prompts, plans, sessions, "the rework" or "as requested". Write tersely and
  concretely. No emoji.
- Change what the task needs, which can mean refactoring the code a fix touches, but keep
  unrelated reformatting, renames and cleanups out of the change. A wider cleanup is a separate
  PR.
- Never commit device keys, provisioning secrets, generated `key.c`/`time.c`, `sdkconfig`,
  build output (`build/`, `twister-out/`) or local reports. Stage explicit paths
  (`git add <file>`); never `git add -A`, `git add .` or `commit -a`.

## Build and test

- The repository is a west manifest repository whose `self.path` is
  `modules/lib/hubblenetwork-sdk`. From the workspace root (two levels above
  `modules/lib/`): `west init -l modules/lib/hubblenetwork-sdk && west update`, then
  `west blobs fetch hubblenetwork-sdk hal_silabs hal_nordic`. `west.yml` tracks upstream
  Zephyr, `west-zephyr-lts.yml` the Zephyr LTS release and `west-ncs.yml` the nRF Connect SDK;
  pick one with `west config manifest.file <file>` and `west update`.
- Zephyr tests: `west twister -T tests/zephyr --quarantine-list tools/ci/twister-quarantine.yaml`
  (the LTS job uses `tools/ci/twister-quarantine-lts.yaml`). Narrow with `-T <test-dir>` or
  `-s <test-dir>/<scenario-id>`; `-p native_sim` runs on the host; `-i` prints failing logs.
- Zephyr samples: `west twister -T samples/zephyr --build-only --quarantine-list
  tools/ci/twister-quarantine.yaml`. A single build: `west build -b <board> samples/zephyr/<name>`.
- Coverage (CI job `codecov.yaml`): `west twister -p native_sim -p unit_testing --coverage
  --coverage-tool gcovr -T tests/zephyr -x=CONFIG_COVERAGE=y`.
- FreeRTOS/TI samples: `python3 tools/ci/freertos_sample_build.py --platform ti samples/freertos/ti`
  (needs the TI SDK and SysConfig; see `.github/workflows/build-samples-ti.yaml`). SiLabs and
  ESP-IDF samples are built by `build-samples-freertos-silabs.yaml` and
  `build-samples-esp-idf.yaml` (manifest `tools/ci/esp-idf-build-manifest.yaml`).
- Docs: `pip install -r docs/requirements.txt`, then `make -C docs doxygen html` (needs
  doxygen and graphviz).
- A change to `src/` or `include/` affects every port. Build at least Zephyr tests plus one
  non-Zephyr port, or state which ports were not built.
- Every commit in a series must build and pass its tests on its own (bisectability).

## Checks to run before pushing

CI runs these in `.github/workflows/compliance.yaml`; each failure costs a round trip. Fix the
cause, never work around a check.

- `git clang-format --extensions c,h --diff origin/main` must produce no diff. Apply
  `.clang-format` to your own hunks only (`git clang-format`); never run `clang-format -i` on
  whole files. Vendor-generated radio configs carry their own `.clang-format`.
- `gitlint --commits origin/main..` (config in `.gitlint`, custom rules in `tools/ci/gitlint/`).
- `ruff check` and `ruff format --diff` for any Python (`.ruff.toml`).
- The PR description must not be empty.
- Optional static checks: `./tools/coccicheck [FILE|DIR]` runs the semantic patches in
  `tools/coccinelle/` (needs `spatch`). CodeQL runs in CI (`codeql.yaml`).

## Commits

    area: subarea: imperative summary (72 columns max, no trailing period)

    Body wrapped at 100 columns or less: what the change does, why this
    approach, which assumptions were made and how it was tested. Never empty.

    Fixes #123

    Assisted-by: Claude:claude-opus-4.6
    Signed-off-by: Full Name <email@example.com>

- `area:` is the prefix the file's recent history uses: `git log --format=%s -20 -- <path>`.
  Examples from the tree: `port: nordic:`, `port: freertos: ti:`, `zephyr: nrf52:`, `sat:`,
  `sat: packet:`, `ble:`, `crypto: psa:`, `samples: zephyr/sat-dual-stack:`,
  `docs: esp-idf:`, `ci:`, `tools:`. Never `WIP`.
- Trailers (`Assisted-by`, `Signed-off-by`) form one block as the last paragraph. `Fixes #N`
  goes in its own paragraph directly before that block.
- The `Signed-off-by:` line must match the commit `Author:` name and e-mail; never remove an
  existing sign-off. Verify with `git log -1 --format='%an <%ae>%n%b'`.
- One logical change per commit. No fixup, squash or merge commits in a PR: fold review fixes
  into the commit that owns the lines. Without an interactive terminal:
  `git commit --fixup=<sha>`, then
  `GIT_SEQUENCE_EDITOR=true GIT_EDITOR=true git rebase -i --autosquash <base>`.
- Update a branch with `git rebase origin/main`, never by merging `main` into it. Before
  pushing, review `git show --stat origin/main..HEAD`.

## Pull requests

- Develop on a fork and open the PR against `main`. Do not push feature branches to the shared
  repository; `main` and `release/*` are the only long-lived branches.
- A PR is one self-contained change. Reviewers are assigned by area (`reviewer.yaml`): Zephyr,
  FreeRTOS, ESP-IDF, docs, CI, tools and common code (`src/`, `include/`). When a change spans
  several areas and splits cleanly, split it. Larger design changes start as an RFC under
  `docs/rfc/`.
- The PR description summarizes the change and its rationale, lists the platforms and boards it
  was built and run on, and carries `Fixes #N` when it addresses an issue. It is rendered as
  Markdown: unwrap paragraphs instead of pasting hard-wrapped commit text. Keep title and
  description in sync with the commits after every force-push.
- API additions, deprecations and behavior changes go in the release notes for the next version
  under `docs/releases/`, in the same PR. Versioning follows semver (`docs/releases/index.rst`);
  the version macros live in `include/hubble/version.h`.
- Issues use the templates in `.github/ISSUE_TEMPLATE/` (bug report, enhancement).
- Security: never file a public issue or PR describing an undisclosed vulnerability, and never
  reference an unpublished advisory in a commit, PR or issue. Report to security@hubble.com or
  through the repository's security advisories. See `.github/SECURITY.md` and
  `docs/security/vulnerability-process.rst`.

## Code rules agents get wrong most often

Complete list: `CONTRIBUTING.md` (Code Style) and
`docs/architecture/project-organization.rst`.

- Layering: dependencies flow downward only. `include/hubble` (public API) -> `src` (common
  code) -> `include/hubble/port` (port contracts) -> `port/<rtos>` -> `port/<rtos>/boards`.
  Code in `src/` never includes RTOS, vendor SDK or board headers. When common code needs a
  platform service, add a contract in `include/hubble/port/` and implement it in every port
  (`zephyr`, `freertos`, `esp-idf`, `sisdk`); do not add RTOS conditionals to `src/`.
- Board files answer how the hardware transmits (driver setup, channel and power mapping, FEM,
  vendor blobs); protocol behavior stays in `src/`.
- Naming: public symbols use `hubble_*`; SDK-private helpers shared between `src/` files are
  `hubble_internal_*`, declared in `src/hubble_priv.h`, and are never called from ports or
  applications; port contracts called by common code use `hubble_*_port_*`. File-local
  `static` functions and variables start with `_`. Units go in names (`timeout_ms`,
  `interval_s`). Public headers do not expose RTOS or board types.
- Linux kernel style as configured in `.clang-format`: tabs, 100-column lines, `/* */`
  comments only, `char *ptr`, `lower_snake_case`, `UPPER_SNAKE_CASE` for macros and enum
  values.
- Explicit comparisons: `if (err != 0)`, `if (ptr == NULL)`; check pointers for NULL at API
  boundaries. Errors are negative errno values; check every return value that carries one and
  propagate it instead of logging and continuing.
- Fixed-width types (`uint8_t`, `int32_t`) for numeric data, `size_t` for sizes and lengths;
  `U` suffix on unsigned constants. Key material is wiped with `hubble_crypto_zeroize()`.
- No dynamic allocation in `src/` or port code; use static storage.
- Configuration is `CONFIG_HUBBLE_*`. Zephyr defines it in `port/zephyr/Kconfig*`, ESP-IDF in
  `port/esp-idf/hubblenetwork-sdk/Kconfig`, FreeRTOS through `port/freertos/config.h` and
  `hubblenetwork-sdk.mk`, SiLabs through `port/sisdk/hubble_config.h`. A new option must be
  wired into every port that compiles the affected source. Build files select sources for whole
  features; `src/` uses `#ifdef CONFIG_HUBBLE_*` only for small decisions inside a selected
  feature. Zephyr-only helpers such as `IS_ENABLED()` are not available in `src/`.
- Public functions in `include/hubble/` have Doxygen: brief, `@param` per argument and
  `@retval <value> <text>` per discrete return value, success first. Update
  `docs/api/*.rst` when adding a header or API group.
- New files start with the copyright and SPDX header in the file's comment syntax:
  `Copyright (c) <year> Hubble Network, Inc.` and `SPDX-License-Identifier: Apache-2.0`. Never
  rewrite existing license headers. Third-party code keeps its own.
- Binary libraries are not committed. Zephyr blobs are listed in `zephyr/module.yml` with their
  SHA-256 and fetched by `west blobs fetch`; the ESP32 PHY blob is user-supplied.
- Zephyr tests live in `tests/zephyr/` with `testcase.yaml`; samples in `samples/<rtos>/` with
  `sample.yaml` and a `README.md`. Unit tests that run on `native_sim` belong in
  `tests/zephyr/unit/`. Behavior changes in BLE advertisement or satellite packet encoding must
  update the test vectors in `tests/zephyr/unit/ble-advertise/` or `tests/zephyr/unit/sat-packet/`.
- Documentation in `docs/` is reStructuredText (Sphinx); keep sample `README.md` files and the
  quickstart and integration guides in step with build or setup changes.

## Finding things

- Public API: `include/hubble/` (`hubble.h`, `ble.h`, `sat.h`, `sat/`); port contracts:
  `include/hubble/port/`; common code: `src/` (crypto providers in `src/crypto/`); ports:
  `port/zephyr/`, `port/freertos/`, `port/esp-idf/`, `port/sisdk/`; board radio support:
  `port/<rtos>/boards/<vendor>/`; samples: `samples/<rtos>/`; tests: `tests/zephyr/`; CI
  helpers and host tools: `tools/`.
- Packaging metadata: `zephyr/module.yml` (Zephyr module), `idf_component.yml` (ESP-IDF
  component), `hubble_device_sdk.slce` and `port/sisdk/*.slcc` (Simplicity Studio),
  `.meta/` and `.metadata/` (TI SysConfig).
- The recent history of a path is the best guide to its local conventions:
  `git log --oneline -20 -- <path>`.
- Documentation source is `docs/`, published at https://hubblenetwork.github.io/hubble-device-sdk/main/index.html
