# Phase 1 test record

Last updated: 2026-08-27

## Build environment

- Distribution: Omarchy 4.0.1 (Arch-based)
- Kernel: Linux 7.1.9-arch1-2 x86_64
- Desktop/session: Hyprland, Wayland
- Compiler: GCC 16.2.1
- Qt: 6.11.2
- FreeRDP/WinPR: 3.30.0
- CMake: 3.31.12 (workspace-local via mise)

## Automated results

| Test | Result |
| --- | --- |
| Debug configure/build | PASS |
| Unit tests | PASS |
| GUI startup on Wayland | PASS (five-second launch smoke test) |
| Direct shared-library linkage | PASS |
| ASAN/UBSAN unit tests | PASS with LeakSanitizer disabled; LSAN cannot run under the managed ptrace environment |

## Manual interoperability results

No Windows endpoint address or test credentials were available in this
environment. These entries intentionally remain `NOT TESTED`.

| Scenario | Result |
| --- | --- |
| Windows 11 local account, NLA | NOT TESTED |
| Windows Server 2025 AD account, NLA | NOT TESTED |
| Incorrect password/retry | NOT TESTED |
| Certificate name mismatch | NOT TESTED |
| Trusted certificate | NOT TESTED |
| Keyboard matrix | NOT TESTED |
| Mouse matrix | NOT TESTED |
| Ten connection cycles | NOT TESTED |
| Network loss | NOT TESTED |
| Interactive ASAN/UBSAN session | NOT TESTED |

Phase 1 is therefore not declared complete.
