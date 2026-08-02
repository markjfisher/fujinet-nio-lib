```text
# cc65 C-to-6502 Optimisation Guide

Do not translate C literally. Re-design each function for the BBC/6502 target while preserving reachable behaviour.

1. Resolve the real BBC build first
- Expand macros and inspect platform-specific definitions.
- Remove no-op behaviour such as config_nio_set_status().
- Remove branches already handled by callers.
- Use known constants, fixed limits, and structure offsets.
- C macros are not automatically available to ca65; define or include assembler constants.

2. Understand the cc65 ABI
- __fastcall__ passes the final argument in A or AX.
- Earlier arguments are pushed left-to-right using pusha or pushax.
- Extra arguments for non-fastcall functions are on c_sp.
- Save important pointers before making calls.
- Assume callees destroy A, X, Y, flags, ptr1/ptr2, and common tmp variables.

3. Avoid C stack locals
- Prefer registers for short-lived values.
- Use fixed BSS scratch for non-reentrant UI code.
- Replace local structures with shared fixed buffers.
- Keep persistent loop values in dedicated storage, not cc65 temporary variables.

4. Simplify control flow
- Implement handlers as direct dispatchers/state machines.
- Test cursor/control keys first.
- Fold ASCII case once with ORA #$20.
- Use shared return0/return1 paths.
- Use `.macpack longbranch` initially; optimise individual branches later.

5. Exploit 6502 flags and arithmetic
- Reuse carry from CMP before SBC.
- Use EOR #1 for Boolean toggles.
- Parse digits with SEC / SBC #'0' and a range CMP.
- Avoid repeated comparisons or reloads when flags already contain the answer.

6. Specialise algorithms
- Use byte offsets when proven below 256.
- Use bounded direct loops instead of generic memcpy/string helpers.
- Patch fixed request blocks instead of constructing local arrays.
- Inline trivial wrappers and one-line helpers.
- Preserve only behaviour reachable on the BBC build.

7. Share small internal helpers
- Common return paths.
- State pointer restoration/load helpers.
- Marker drawing helpers.
- Repeated save/refresh call wrappers.
- Keep helpers only when they reduce total code size.

8. Watch shared memory carefully
- Know which calls overwrite shared buffers and scratch.
- Clear records when stale fields could remain valid.
- Do not expect response buffers or zero-page temporaries to survive calls.
- Dedicated BSS is safer for values needed across calls.

9. Work iteratively
- Convert one large function.
- Expose C helpers temporarily by removing static.
- Build, test behaviour, and measure size.
- Convert the largest remaining helpers next.
- Only hunt one-byte branch/jump savings after structural savings are complete.

Primary rule:
Find the smallest BBC-specific algorithm implementing the required behaviour, not an assembly spelling of every C statement.
```
