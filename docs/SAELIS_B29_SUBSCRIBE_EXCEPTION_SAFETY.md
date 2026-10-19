# SAELIS B29 — EventBus subscription exception safety

## Baseline and scope

- Native remote base: `spiraletech/spiral-core-sdk` feature branch `saelis/b28-installed-sdk-consumer-20261019` at `78277cb2c2cb5c0097c6b830c1aa0371568c70d3`.
- This is a narrowly scoped forward-port of one safety improvement from the isolated B27 research package; it does **not** integrate or validate the entire B27 history.
- No HOME canonical state, HAKUI gameplay state, KATA3 scene logic, or VST DSP callback changed.

## Defect and correction

The native `EventBus::subscribe` incremented `next_id_` before handler construction and insertion. A throwing handler copy constructor consumed a subscription ID without registering a callback. The handler was also constructed under the subscription mutex, allowing user-defined copy/move constructors to reenter the bus and deadlock.

B29 constructs the type-erased handler and an owning pending slot before locking, then inserts under the lock and advances the token only after successful insertion. On failure, the pending callback is destroyed after lock unwinding. Existing successful ID ordering and the public API are unchanged.

## Verified local evidence (Linux only)

- Git blob hashes for the native B28 baseline sources were checked against GitHub before modification.
- Test-first reproduction: `spiral.core` failed on `next == first + 1` after a handler-copy exception; `spiral.contract` passed.
- Post-fix local GCC 14.2 and Clang 17, Debug/Release, `-Wall -Wextra -Werror`: 8/8 project CTest executions passed.
- Independent installed-package consumer: 4/4 CTest executions passed.
- Clang UBSan and ASan+UBSan Debug: 4/4 project CTest executions passed.
- Total post-fix matrix: **16/16** CTest executions passed. A separate post-fix Debug retest passed 2/2.
- The added regression checks both failed-subscription ID stability and reentrant handler construction.

## Boundaries and remaining risks

`EventBus` remains synchronous, lock-using, and allocation-capable. It is **not** safe to invoke on VST real-time audio threads. B29 does not address reentrant handler copies during `publish` snapshotting, handler destruction during `unsubscribe`, or subscription-ID exhaustion. Those require separate design/review.

## Integration and alert handoff

- B29 is stacked on the unmerged B28 installed-package feature branch. Do not merge B29 independently of B28.
- Before merge, review B28 and B29, verify exact-commit Windows/Linux GitHub Actions CI, and evaluate the remaining `publish`/`unsubscribe` reentrancy boundaries.
- Approval gate: `B29_STACKED_ON_B28_REVIEW` — owner review/merge authorization is still required. Do not send duplicate blocker alerts for this unchanged gate.
