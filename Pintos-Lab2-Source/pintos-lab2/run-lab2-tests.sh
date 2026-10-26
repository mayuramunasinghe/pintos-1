#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export PATH="$project_dir/src/utils:$PATH"
mkdir -p "$project_dir/test-evidence"
command -v qemu-system-i386 >/dev/null || { echo 'Install qemu-system-x86 first.' >&2; exit 1; }
cd "$project_dir/src/userprog"
make clean
make 2>&1 | tee "$project_dir/test-evidence/build.log"
cd build
make check 2>&1 | tee "$project_dir/test-evidence/test-run.log"
cp results "$project_dir/test-evidence/results.txt"
