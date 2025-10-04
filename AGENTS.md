# Repository Guidelines

## Project Structure & Module Organization
Core C sources live in `src/`, with generated indicator implementations in `src/ta_func`, common math utilities in `src/ta_common`, and the abstraction layer in `src/ta_abstract`. Public headers sit under `include/ta-lib`, while platform packaging artefacts are produced in `dist/`. Language bindings live beside the generator (`dotnet/`, `java/`, `rust/`), and maintainer tooling is collected in `scripts/`. Treat `build/` as disposable; regenerate it per toolchain.

## Build, Test, and Development Commands
- `./autogen.sh && ./configure && make -j$(sysctl -n hw.ncpu)` — bootstrap autotools, configure for your platform, and build the C library.
- `cmake -S . -B build && cmake --build build` — alternative CMake build, mirroring CI.
- `scripts/sync.py` — sync `dev` with upstream, refresh version strings, and apply repo-wide checks before committing.
- `bin/gen_code` — regenerate derived sources (run post-build so artifacts exist).
- `src/tools/ta_regtest/ta_regtest` — execute the regression suite; zero exit code means pass.

## Coding Style & Naming Conventions
C files use three-space indentation and braces on their own line. Preserve the `/* Generated */` markers and never hand-edit between `GENCODE` sentinels; adjust templates or inputs such as `ta_func_api.xml` and rerun `bin/gen_code` instead. Public APIs retain the `TA_` prefix, and new enumerations or error codes belong in `include/ta-lib/ta_*.h`. Keep whitespace clean and rely on generator output for formatting whenever possible.

## Testing Guidelines
Build the library first, then call `src/tools/ta_regtest/ta_regtest` against fresh binaries. Regenerate bindings (`bin/gen_code`) before testing language-specific surfaces. For packaging verification, `scripts/test-dist.py` exercises installer scenarios; use it before publishing assets. When adding regression data, stage it under `src/tools/ta_regtest` and name fixtures after the indicator they validate.

## Commit & Pull Request Guidelines
Work on the `dev` branch and avoid committing directly to `main`. Run `scripts/sync.py` prior to each push so shared metadata stays aligned. Follow the existing history’s tone: short, imperative subjects such as “Update dist package …” without trailing punctuation. PRs should summarize the change, cite related issues, list manual test steps or regtest results, and attach screenshots when the behaviour affects generated bindings or docs.

## Generated Bindings & Docs
Any change affecting indicator signatures must be reflected in `ta_func_api.xml`, regenerated via `bin/gen_code`, and validated in the downstream language directories. Update `docs/` alongside code when public-facing behaviour shifts, and rebuild the MkDocs site locally (`mkdocs serve`) if you touch user guides.
