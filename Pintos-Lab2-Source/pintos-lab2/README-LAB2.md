# Pintos Lab 2 - User Programs

Implemented from the uploaded Pintos project, retaining its Lab 1 work.

## Contents and verification

- `p2.md`: completed design template; fill in your name, student ID, and email.
- `src/`: full Pintos sources, including original tests.
- `run-lab2-tests.sh`: clean build and test command with saved logs.
- `LAB2-CHANGES.patch`: changes relative to the uploaded source, excluding the design document and packaging helpers.

Verified on 25 October 2026: all 80 supplied Lab 2 tests passed with QEMU 8.2.2. The 439 supplied test files are unchanged. Lab 1 regression tests and unavailable hidden tests are not covered by this result.

## Run on Linux

Required tools: GCC and binutils capable of 32-bit compilation, Make, Perl, and `qemu-system-i386`. On Ubuntu, the relevant packages normally include `build-essential`, `gcc-multilib`, `perl`, and `qemu-system-x86`.

From this folder:

```bash
bash run-lab2-tests.sh
```

Take screenshots showing the pass/fail lines and final summary if your instructor requires captures from your own computer.

## Implementation map

- `src/userprog/process.c/.h`: arguments, load notification, wait, status ownership, termination, executable write denial.
- `src/userprog/syscall.c/.h`: 13 system calls, pointer checking, file descriptors, file-system synchronization.
- `src/userprog/pagedir.c/.h`: writable-page inspection helper.
- `src/threads/thread.c/.h`: process fields and their initialization.
- `src/Make.config`: modern compiler/linker compatibility.
- `src/filesys/filesys.h`: extern device declaration for a single definition.

The design uses one global file-system lock and page-by-page validation. It is intended for Project 2's one-thread-per-process, no-eviction model. Commands are bounded to 4095 characters and the initial argument stack must fit one page.

Review the source and design before submission. ChatGPT/Codex assisted with implementation, debugging, verification, and documentation; the design document records this assistance. Identity fields are intentionally left for you to complete.
