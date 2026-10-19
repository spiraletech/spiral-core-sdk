# SAELIS B28 — Installed SDK consumer contract

- Scope: install/export `Spiral::Core`, versioned CMake package config, and independent downstream smoke test.
- Baseline: `spiraletech/spiral-core-sdk` `main` at `b7ad1c6e9a07106af6759a58bc254f46aa460d52` (October 19, 2026 inspection).
- Baseline local CTest: 2/2 passed (Linux, GCC).
- Modified local matrix: GCC 14.2 and Clang 17, Debug and Release; 2/2 project CTests and 1/1 installed consumer CTest passed in each configuration (12/12 CTest executions).
- Additional relocation smoke: installed prefix copied to a new location; independently configured consumer passed 1/1.
- Source identity: baseline public headers and tests matched GitHub blob SHA-1 via `git hash-object`.
- GitHub Windows/Linux workflow: extended with isolated install and downstream consumer checks. **Not executed at the time of this local report.** Review the branch's GitHub Actions checks before merge.
- Authority boundaries: no gameplay, canonical HOME state, scene compiler, or real-time DSP code changed. No merge or deployment authorized.
- Blockers: Windows CI and native external consumer integration remain to be verified on the branch; owner approval required for merge.
