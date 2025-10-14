# Recipes: Adding New Indicators

Technical indicators in TA-Lib are generated from a mix of handwritten C code and metadata definitions. This short recipe walks through the recommended workflow for introducing a new indicator (or upgrading an existing one) so that all language bindings and regression tests stay in sync.

## 1. Gather specifications

Before you touch the codebase, make sure you have:

- A written description of the indicator, including formulae and parameter defaults.
- A minimal set of reference price series and the expected outputs. These vectors are used by the regression tests to lock in behaviour.
- Any bibliographic references or web links that justify the implementation.

> ℹ️  The [ta-lib-proposal-drafts](https://github.com/TA-Lib/ta-lib-proposal-drafts) repository is the canonical spot to record this material.

## 2. Add metadata

Every indicator is described in `ta_func_api.xml`. Copy the closest existing entry and update:

- `<group>` for documentation categorisation.
- `<output>` and `<optInput>` blocks to match parameters and outputs.
- `<flags>` when the indicator has special traits (e.g. overlapping, candle pattern, requires volume).

While editing XML, keep it alphabetically sorted and respect existing comment style—this makes rebases on upstream easier.

Next, register the indicator name in `ta_func_list.txt`. This file controls discovery order, documentation, and generator output.

## 3. Implement the core C function

Create or edit `src/ta_func/ta_<NAME>.c` and provide the actual numerical implementation. Each indicator exports three key entry points:

- `TA_<NAME>` is the public API.
- `TA_<NAME>_BatchStateInit` / `_BatchStateFree` (if batching is supported).
- `TA_<NAME>_StateInit` / `_State` / `_StateFree` / `_StateSaveLoad` for the stateful variant.

Re-use helpers under `src/_common/` whenever possible. Adding targeted unit helpers next to the new file keeps the regression step clean.

## 4. Regenerate bindings and tables

Once the XML and C file exist, regenerate every derived asset:

```bash
cmake --build build --target gen_code
```

This target refreshes:

- C lookup tables (`table_a.c`, `ta_group_idx.c`, ...)
- Public headers (`include/ta_func.h`)
- SWIG wrappers
- Java and .NET bindings

If you prefer a standalone binary, you can also run `./build/bin/gen_code` directly after the build step above.

## 5. Rebuild and run the regression suite

```bash
cmake --build build
./build/bin/ta_regtest
./build/bin/ta_regtest -p   # optional profiling pass
```

The regression harness loads canonical quote history and compares outputs against the expectations encoded in the metadata. When the new indicator needs bespoke reference data, add a dataset in `src/tools/ta_regtest/test_data.c` (or a sibling include) and wire it into the relevant test helper.

If any failure mentions "value out of tolerance" inspect the generated reference arrays and the indicator implementation—the tooling prints the tolerance window that must be satisfied.

## 6. Update documentation and changelog

- Add a short entry to `docs/functions.md` (or the relevant section) stating the indicator name and parameters.
- Mention the addition in the project changelog if it is user-facing.
- For indicators that depend on external libraries or special compilation flags, capture the requirements under `docs/install.md`.

## 7. Optional: extend wrappers

Language bindings (Java, .NET, Python via SWIG, Rust, etc.) automatically pick up new indicators after `gen_code` runs. If the indicator exposes unusual options or data types, double-check the generated code and adjust wrapper-specific documentation or examples.

---

Following this recipe keeps the generator in sync and prevents hand-written edits from being overwritten the next time `gen_code` runs. Remember: never modify generated files manually—always update the XML + source inputs and let the tooling do the rest.
