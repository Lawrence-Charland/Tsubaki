# Tsubaki

A command-line checksum utility for file integrity verification, written in modern C++20.

[中文文档](README.zh-cn.md)

## Features

- **Checksums** – Compute checksums for files and directories using any digest
  supported by OpenSSL (MD4/MD5, SHA-1, SHA-2, SHA-3, SHAKE, BLAKE2).
- **Recursive scanning** – Scan directories recursively, with optional symlink
  following and permission-denied tolerance.
- **Multiple inputs** – Accept regular files, directories, a tsubaki-format
  checksum list on stdin, or a plain path list on stdin.
- **Filtering** – Exclude path prefixes and filter by minimum/maximum file size.
- **Multi-threading** – Hash files in parallel with a thread pool sized to the
  hardware; tasks are dispatched through a lock-free queue, and read buffers
  are sized dynamically per file.
- **Resumable** – Already-computed hashes from the input list are reused unless
  `--force-scan` is given, so an interrupted run can be resumed.
- **Progress bar** – Shows live progress on stderr when it is a terminal,
  including file counts and processed bytes.
- **Clean output** – Results go to stdout, logs to stderr, with a summary report
  appended at the end.

## Requirements

- A C++20 compiler (GCC 13+, Clang 16+, Apple Clang 15+, or MSVC 19.29+)
- CMake 3.16+
- OpenSSL development files
- Linux, macOS, or Windows

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The executable is produced at `build/tsubaki`.

### OpenSSL on Windows

OpenSSL is not bundled and must be installed first. Pick one of the options
below, then configure the project with CMake.

**Option 1 – vcpkg (recommended)**

```sh
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install openssl:x64-windows

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

**Option 2 – prebuilt installer**

Install a Win64 OpenSSL 3.x package (for example from
<https://slproweb.com/products/Win32OpenSSL.html>), then point CMake at it:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64"
cmake --build build --config Release
```

**Option 3 – Chocolatey**

```sh
choco install openssl

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64"
cmake --build build --config Release
```

At runtime `libcrypto-3-x64.dll` must be reachable: add the OpenSSL `bin`
directory to `PATH`, or copy the DLL next to `tsubaki.exe`.

### Options

| CMake option | Default | Description |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | – | `Release` is recommended for hashing performance |
| `BUILD_TESTING` | `ON` | Build the test suite (requires GoogleTest) |
| `TSUBAKI_SANITIZE` | `OFF` | Build with AddressSanitizer and UBSan |

## Usage

```text
tsubaki <command> [options] [paths...]
```

### Commands

| Command | Description |
| --- | --- |
| `sum <algorithm> <path...>` | Compute checksums for files and directories |
| `cmp <fileA> <fileB>` | Compare two checksum lists and report differences |
| `dup` | Find duplicate files from a checksum list on stdin |
| `help [key]` | Show help in English; with `key`, show that topic's details |
| `help-cn [key]` | Same as `help` but in Chinese |

`key` may be a command, input, option, or algorithm, for example
`tsubaki help sum`, `tsubaki help --exclude`, or `tsubaki help-cn stdin`.

### Comparing and de-duplicating

Both `cmp` and `dup` consume tsubaki-format checksum lists (one `<hash> <path>`
entry per line), such as the output of `sum`.

`dup` reads the list from stdin, groups entries that share a checksum, prints
each group, and ends with a suggested `rm` command for the extra copies:

```sh
tsubaki sum sha256 ./photos | tsubaki dup
```

`cmp` takes two list files and reports:

| Tag | Meaning |
| --- | --- |
| `[!]` | Modified: same path, different checksum |
| `[D]` | Moved/copied/renamed: same checksum under different paths |
| `[U][A]` / `[U][B]` | Deleted (only in A) / added (only in B) |
| `[=]` | Matched: same path and checksum |

```sh
tsubaki cmp A.txt B.txt > comparison.txt
```

### Inputs for `sum`

| Input | Description |
| --- | --- |
| `<path>` | A regular file, or a directory scanned recursively |
| `stdin` | Read a tsubaki-format checksum list from stdin |
| `stdin-plain-list` | Read a plain list of paths from stdin, one per line |

### Algorithms

`md4`, `md5`, `sha1`, `sha224`, `sha256`, `sha384`, `sha512`, `sha512_224`,
`sha512_256`, `sha3_224`, `sha3_256`, `sha3_384`, `sha3_512`, `shake128`,
`shake256`, `blake2b512`, `blake2s256`.

### Options

| Option | Description |
| --- | --- |
| `--exclude=PATH` | Exclude paths beginning with `PATH` (repeatable) |
| `--min-size=SIZE` | Only include files not smaller than `SIZE` |
| `--max-size=SIZE` | Only include files not larger than `SIZE` |
| `--force-scan` | Recompute hashes even if present in the input list |
| `--allow-symlinks` | Follow directory symlinks while scanning |
| `--threads=N` | Number of worker threads (default: hardware concurrency) |
| `--test` | Scan and report only; do not compute checksums |
| `--progress` | Force the progress bar on |
| `--no-progress` | Force the progress bar off |
| `--log-level=LEVEL` | `DEBUG`, `INFO`, `WARN`, or `ERROR` (default `INFO`) |
| `--quiet` | Deprecated. Same as `--log-level=ERROR` |
| `-v` | Deprecated. Same as `--log-level=INFO` |
| `-h`, `--help` | Show the general help in English |
| `--help-cn` | Show the general help in Chinese |

`SIZE` accepts an optional unit suffix: `b`, `k`, `m`, `g`, `t`, `p`
(binary, so `1k` = 1024 bytes).

### Output

Each processed file produces one line on stdout:

```text
<hash> <path>
<NONE> <path>          # the file could not be processed
```

A summary report is appended at the end:

```text
#
# ----------General Report----------
# Total: 3
# Succeed: 3
# Failed: 0
# Unprocessed: 0
# Time started: 2026-09-17 12:00:00
# Time finished: 2026-09-17 12:00:01
# Duration: 1.23s
# Command: tsubaki sum sha256 ./data
```

If the run is interrupted with `Ctrl+C` (`SIGINT`), the report is still printed
and an extra line is added:

```text
# Interrupted: yes
```

### Exit status

| Code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Invalid arguments, unsupported algorithm, or one or more failed files |
| `130` | Interrupted by `SIGINT` (`Ctrl+C`) |

## Examples

```sh
# Checksum a single file
tsubaki sum sha256 ./archive.tar.gz

# Recursively checksum a directory, skipping a subdirectory and tiny files
tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1k

# Write results, then resume an interrupted run from the partial output
tsubaki sum sha256 ./data > partial.txt
cat partial.txt | tsubaki sum sha256 stdin > complete.txt

# Read a plain path list from stdin
printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list

# Inspect the scan/filter pipeline without hashing
tsubaki sum sha256 ./data --test

# Find duplicates, then compare two checksum lists
tsubaki sum sha256 ./photos | tsubaki dup
tsubaki cmp before.txt after.txt

# Chinese help, or details about a single topic
tsubaki help-cn
tsubaki help --exclude
```

## Testing

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The suite uses GoogleTest, located via `find_package(GTest)` or fetched
automatically by CMake if it is not installed.

## Project layout

```text
core/
  common/   Argument-driven logging setup, checksum-list parsing, help text
  cmp/      `cmp` command: compare two checksum lists
  dup/      `dup` command: find duplicate files
  sum/      `sum` command: scanning, filtering and hash computation
plugins/
  arg_parser.*   Command-line parsing
  encoder.*      OpenSSL digest lookup and file hashing
  file_list.*    File metadata container
  lock_free_queue.h  Bounded lock-free MPMC queue (Vyukov)
  logger.*       Leveled logger
  thread_pool.*  Worker pool used for hashing, backed by the lock-free queue
  trie.*         Path-prefix matcher used by --exclude
tests/      GoogleTest suite
```

## Notes and limitations

- Only regular files are checksummed; directory symlinks are not followed unless
  `--allow-symlinks` is set.
- Subdirectories that cannot be accessed are skipped with a warning.
- Paths are normalized to absolute paths in the output.
- `SIGINT` (`Ctrl+C`) stops the worker pool gracefully. Files already hashed are
  still printed, the summary report is still emitted with `# Interrupted: yes`,
  and the process exits with code `130`; the output is suitable for resuming.
- The main thread waits at most 300 ms for each file's hash; files that take
  longer are deferred to a second pass after the first traversal, so slow files
  do not block the results of faster ones.
- The progress bar is drawn on stderr only when stderr is a terminal wide enough
  to fit the whole line and the log level is `INFO` or lower; it is disabled
  automatically when output is redirected, logging is quieted, or the terminal is
  too narrow (wrapping would break the in-place `\r` refresh).
- The file list is fully enumerated in memory before filtering, which is a
  bottleneck for very large trees.

## License

Released under the MIT License. See [LICENSE](LICENSE) for details.

## Author

- **Lawrence Charland**
