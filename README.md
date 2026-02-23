### 注意：以下展示的是README的英文翻译版。若需阅读中文请移步至项目文件夹下的README.zh_cn.md。

This is a personal practice project, and also the first project I've created since I started coding. You are welcome to use it. **I am not a professional developer**, so the code may reveal some of my inexperience, and I hope you can understand. This project is reasonably robust, but not production‑ready. **Feel free to play with it as a toy.**

Due to my limited knowledge, this project **supports only Linux distributions**. Without further ado, the help information is as follows:

# Tsubaki

Tsubaki is a checksum utility for verifying file integrity, comparing directories, and detecting duplicates. It supports multiple hash algorithms (MD5, SHA1, SHA256, SHA512) and offers flexible input sources – directories, file lists, or plain path lists. Multi‑threaded processing and dynamic buffer sizing make it suitable for large data sets.

## Features

- **Sum mode** – Recursively compute checksums for all files in a directory, or for a file list read from standard input.
- **Duplicate mode** – Identify duplicate files based on identical checksums and suggest a deletion command.
- **Compare mode** – Read and compare two checksum lists, classifying files as modified, moved/copied, added/deleted, or matched.
- **Multi‑threading** – Automatically enabled for large workloads (total size > 1 GB); thread count and chunking strategy are configurable.
- **Flexible filtering** – Include or exclude subtrees, filter by file size, optionally follow symbolic links to directories.
- **Progress reporting** – Verbose modes show a progress bar or the current file being processed.
- **Dynamic buffer** – Adjusts the I/O buffer size based on file size for better efficiency.
- **Graceful interruption** – Press Ctrl+C to stop processing; already computed results are still written to stdout.

## Building

Tsubaki requires a C++17 compiler, the OpenSSL development libraries, and pthreads (on Unix-like systems).

### Dependencies

- A computer with a Linux distribution installed
- C++17 standard library
- OpenSSL (libcrypto)
- POSIX threads
- `filesystem` (part of C++17; on older compilers you may need `-lstdc++fs`)

### Compile

```bash
g++ -std=c++17 -O3 -o tsubaki main.cpp -lssl -lcrypto -lpthread
```

On some systems you might need to add `-lstdc++fs`. If your compiler fully supports C++17 with integrated `std::filesystem`, the extra library is not required.

## Usage

```
tsubaki <command> [options]
```

Commands: `sum`, `mrg`, `cmp`, `dup`, `help`

### Global Options

- `--quiet` – Suppress informational messages (errors still go to stderr).
- `-v`, `-vv` – Increase verbosity (only effective in `sum` mode).  
  `-v` shows a progress bar; `-vv` shows the current file name.

---

## Command Syntax

### `sum <type> <path> [options]`

Compute checksums for files.

**`<type>`** – one of: `md5`, `sha1`, `sha256`, `sha512`, `none`.  
With `none` no checksum is calculated; the output is a list of paths (optionally with `<NONE>` placeholders). Use `--plain-list` to output only paths.

**`<path>`** can be:
- a **regular file** – prints its checksum (or `<NONE>`) and exits.
- a **directory** – recursively lists all regular files, computes checksums, and outputs `<checksum> <relative-path>` for each.
- **`"stdin"`** – reads an existing list in the format `<checksum> <path>` (lines starting with `#` are ignored). Existing checksums are kept (unless `--force-rescan` is used) to avoid recomputation.
- **`"stdin-plain-list"`** – reads plain file paths (one per line) and computes checksums for them.

#### Options for `sum`

| Option | Description |
|--------|-------------|
| `--exclude=<dir>` | Exclude files under the given directory (can be repeated, intersection). |
| `--focus=<dir>`   | Only include files under the given directory (union if repeated); applied after excludes. |
| `--max-size=<size>` | Skip files larger than `<size>`. Examples: `10M`, `2G`, `1.5K`. Suffixes: `K`=KiB, `M`=MiB, `G`=GiB, `T`=TiB. |
| `--min-size=<size>` | Skip files smaller than `<size>`. |
| `--allow-symlinks` | Follow symbolic links (use with caution; may create infinite loops). |
| `--test`          | Do not compute checksums – only load and filter the file list, and show the planned total files and size. |
| `--plain-list`    | When `<type>` is `none`, output only file paths (one per line) without `<NONE>` placeholders. |
| `--disable-multi-thd` | Disable multi‑threading. By default, multi‑threading is enabled when total size > 1 GiB. |
| `--thd-amount=<N>` | Set the number of threads (default: hardware concurrency). |
| `--contiguous`    | When multi‑threading, split the file list into contiguous chunks instead of interleaved. |
| `--force-rescan`  | Recompute checksums even for files that already have a checksum in the input list (used with `path="stdin"`). |
| `--buffer-size=<size>` | Use a static buffer of exactly `<size>` bytes. Default is dynamic (2 KB ~ 64 KB). |
| `--max-buffer-size=<size>` | Limit the maximum buffer size when using dynamic sizing (ignored if `--buffer-size` is set). |

#### Output

For multiple files, each line is `<checksum> <path>` (or just `<path>` with `--plain-list`). After the file list, a summary block is appended:

```
#
#----------General Report----------
#Total:          1234
#Succeed:        1230
#Failed:         2
#Kept:           2
#Unprocessed:    0
#Time Finished:  2025-03-15-10-30-45
#Duration:       12.34 secs
#Command:        tsubaki sum sha256 /home/user ...
#Error messages:
#Error: Cannot open file: /home/user/secret1.txt
#Error: Cannot open file: /home/user/secret2.txt
```

If any errors occurred during checksum calculation, they are listed at the end. The program exits with status `1` if at least one such error was reported.

#### Tips

- Press **Ctrl+C** to stop calculation gracefully. Already processed results are still written to stdout. You can later resume by feeding the half‑finished output back with `cat half.txt | tsubaki sum <type> stdin` (resume support).
- Use `--plain-list` together with `none` to generate a simple file list for backup purposes.

---

### `mrg <fileA> <fileB> [options]`

Merge two existing checksum lists (each line: `<checksum> <path>`) into one list with lines prefixed by `A ` or `B `. The result is printed to stdout and is typically used as input for `cmp`.

**Options:** `--quiet` suppresses informational messages.

---

### `cmp [options]`

Read from stdin lines with `A ` or `B ` prefixes (typically the output of `mrg` mode) in the format:
```
A <checksum> <path>
B <checksum> <path>
```
(lines starting with `#` are ignored). Compare the two sets and group files into categories:

- **[!] Modified** – same path, different checksum.
- **[D] Moved/copied/merged/renamed** – same checksum, different paths (grouped by checksum).
- **[U] Deleted or added** – files present only in A or only in B.
- **[=] Matched** – same path and same checksum.

The output is printed to stdout.  
**Options:** `--quiet` suppresses informational messages.

---

### `dup [options]`

Read from stdin a list of `<checksum> <path>` (without prefix). Identify duplicate files (identical checksum) and print groups of duplicates. A suggested deletion command (`rm`) for all but the first file in each group is also printed.

**Options:** `--quiet` suppresses informational messages.

---

### `help`

Display a concise help message.

---

## Examples

### 1. Compute SHA256 for all files under `/home/user`, excluding cache and config directories

```bash
tsubaki sum sha256 /home/user --exclude=/home/user/{.cache,.config} > home_checksums.txt
```

### 2. Generate a plain list of all files in `/photos` (no checksums)

```bash
tsubaki sum none /photos --plain-list > photo_list.txt
```

### 3. Compute MD5 for each path listed in `filelist.txt`

```bash
cat filelist.txt | tsubaki sum md5 stdin-plain-list > file_checksums.txt
```

### 4. Compare two directories

```bash
tsubaki sum sha256 /dirA > A.txt
tsubaki sum sha256 /dirB > B.txt
tsubaki mrg A.txt B.txt | tsubaki cmp > comparison.txt
```

### 5. Find duplicate files in a photo collection

```bash
tsubaki sum md5 /photos | tsubaki dup
```

### 6. Resume interrupted calculation

```bash
# First run (interrupted)
tsubaki sum sha256 /bigdata > partial.txt
# Later continue (partial.txt contains already computed entries)
cat partial.txt | tsubaki sum sha256 stdin > complete.txt
```

### 7. Use multi‑threading with 8 threads and contiguous chunking

```bash
tsubaki sum sha256 /large_dir --thd-amount=8 --contiguous > sums.txt
```

---

## Exit Status

- **0** – Success (all files processed, and no errors occurred).
- **1** – An error occurred (invalid arguments, I/O error, or at least one checksum calculation failed).  
  In `sum` mode, if any file could not be read or its hash could not be computed, error messages are appended to the output and the program returns `1`.
  (Errors during file list loading are printed immediately and do not affect the exit status.)

---

## Notes

- **Directory scanning** – When scanning a directory, only regular files and symbolic links pointing to regular files are included. If you want to exclude such symlinks, you can add a filter like `--focus=/target_dir` (where `/target_dir` is the directory to be verified).
- **Symbolic links** – By default they are not followed. Use `--allow-symlinks` to follow them, but beware of cyclic links.
- **File size filtering** – Sizes are checked **after** loading the file list. For directories, this means all files are first enumerated, then filtered. For large directories with many files, this can consume memory.
- **Multi‑threading** – When enabled, the file list is split among threads. The default splitting is interleaved (round‑robin) to balance load; `--contiguous` splits into contiguous blocks. The latter may cause imbalance if file sizes vary widely.
- **Signal handling** – `SIGINT` (Ctrl+C) is caught. Threads finish their current file and then exit. Already computed results are still written. If a thread is processing a very large file, it may take a while to exit; please be patient or send `SIGKILL` if necessary.
- **Memory usage** – The file list is stored entirely in memory. For very large directory trees (millions of files), this may become a bottleneck, but typical hardware can handle it.

---

## License

Tsubaki is a personal project and has no copyright; you may do anything with it.

---

## Developer and Contact

- **Nickname** – Lawrence Charland
- **Primary email** – 1470595583@qq.com
- **Backup email** – 3818604312@qq.com

## Attention

This README was translated by AI from the Chinese version. If there are any errors, please refer to the Chinese version or the source code.
