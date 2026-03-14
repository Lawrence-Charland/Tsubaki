### 注意

以下展示的是README的英文翻译版。若需阅读中文请移步至项目文件夹下的 `README.zh-cn.md` 。

### Attention

This README was translated by AI from the Chinese version. If there are any errors, please refer to the Chinese version or the source code.

---

This is a personal practice project, and also the first project I created since I started programming. You are welcome to use it. If you find any issues, feel free to let me know.

*Note: If you want to make any modifications to the repository, please inform me first.*

Here is the tool introduction:

---


# Tsubaki

Tsubaki is a checksum utility for verifying file integrity, comparing directories, and detecting duplicate files. It supports multiple hash algorithms (MD5, SHA1, SHA256, SHA512) and provides flexible input sources—directories, file lists, or plain path lists—as well as single-file checksum calculation. Multi-threading and dynamic buffer sizing make it suitable for large datasets.

## Features

- **Sum Mode** – Recursively compute checksums for all files in a directory, supporting direct directory scanning or reading a file list from standard input for calculation.
- **Deduplication Mode** – Identify duplicate files based on identical checksums, with a suggested deletion command.
- **Comparison Mode** – Read and compare two checksum results, categorizing files as modified, moved/copied, added/deleted, or matched.
- **Multi-threading Acceleration** – Automatically enabled for large tasks (directory calculation with total size > 1GB) to fully utilize hardware performance; configurable thread count and chunking strategy.
- **Flexible Filtering** – Focus on or exclude subdirectories, filter by file size, and allow following symbolic links.
- **Progress Reporting** – Verbose mode shows a progress bar or the currently processed file name.
- **Dynamic Buffer** – Automatically adjusts buffer size based on file size to maximize efficiency.
- **Graceful Interruption** – Press Ctrl+C to stop processing; already computed results are still written to stdout.

## Build Requirements

Tsubaki requires a C++17 compiler, OpenSSL development libraries, and pthreads (on Unix-like systems).

### Dependencies

- A computer with a Linux distribution installed
- C++17 standard library
- OpenSSL (libcrypto)
- POSIX threads
- `filesystem` (part of C++17; on older compilers you may need `-lstdc++fs`)

### Compilation

```bash
g++ -std=c++17 -O3 -o tsubaki main.cpp -lssl -lcrypto -lpthread # You can add other parameters or change the compiler
```

On some systems, you might need to add `-lstdc++fs`. If your compiler fully supports C++17 with integrated `std::filesystem`, no extra library is needed.

You can also directly run the `build` script in the project folder (requires root privileges).

#### Note

It is recommended to check the SHA256 checksum of each file (at least the **source code**) before compilation, and compare it with the content of `Checksum.sha256.txt` in the project folder.

## Usage

```
tsubaki <command> [options]
```

Commands: `sum`, `cmp`, `dup`, `help`

### Global Options

- `--quiet` – Suppress informational messages (errors are still printed to stderr).

---

## Command Syntax

### `sum <type> [paths] [options]`

Compute checksums for files.

**`<type>`** – Supported values: `md5`, `sha1`, `sha256`, `sha512`, `none`.
When using `none`, no checksum is computed; output is a list of paths (optionally with a `<NONE>` placeholder). Use `--plain-list` to output only paths.

**`[paths]`** can be:
- **Regular file** – Print its checksum (or `<NONE>`) and exit.
- **Directory** – Recursively scan all regular files in the directory, compute checksums, and output `<checksum> <relative-path>` for each file. Multiple directories can be specified **(see “Tips” section)**.
- **`"stdin"`** – Read an existing list in the format `<checksum> <path>` (lines starting with `#` are ignored). Existing checksums are retained (unless `--force-rescan` is used) to avoid recomputation.
- **`"stdin-plain-list"`** – Read plain file paths (one per line) and compute checksums for them.

#### Options for `sum`

| Option | Description |
|--------|-------------|
| `--exclude=<dir>` | Exclude files under the specified directory. Multiple occurrences are combined (union). |
| `--focus=<dir>`   | Only include files under the specified directory. Multiple occurrences are combined (union); applied after exclusions. |
| `--max-size=<size>` | Skip files larger than `<size>`. Examples: `10M`, `2G`, `1.5K`. Suffixes: `K`=KiB, `M`=MiB, `G`=GiB, `T`=TiB. |
| `--min-size=<size>` | Skip files smaller than `<size>`. |
| `--allow-symlinks` | Follow symbolic links (use with caution; may cause infinite loops). |
| `--test`          | Do not compute checksums—only evaluate file filtering rules, buffer strategy, thread allocation, and report the total number and size of files. |
| `--plain-list`    | When `<type>` is `none`, output only file paths (one per line), without the `<NONE>` placeholder. |
| `--thd-amount=<N>` | Set the number of threads. If not specified or the value is invalid, use hardware concurrency (if total size > 1G) or 1. |
| `--balance`       | In multi-threaded mode, sort files by size before interleaving allocation, so that each thread handles a more balanced total data size (helps save time). |
| `--force-rescan`  | Recompute checksums even if they already exist in the input list (used with `path="stdin"`). |
| `--buffer-size=<size>` | Use a static buffer of exactly `<size>` bytes. Default is dynamic buffer (2KB ~ 64KB). |
| `--max-buffer-size=<size>` | When using dynamic buffer, set an upper bound. Ignored if `--buffer-size` is set. |
| `-v` | Increase verbosity, showing the file being processed and the total number of processed files. |

#### Output

For multiple files, each line outputs `<checksum> <path>` (or just `<path>` if `--plain-list` is used). After the file list, a summary block is appended:

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

If errors occur while loading the file list, they appear immediately on the console; if errors occur during checksum calculation, they are listed at the end of stdout, and the program exits with status `1`.

#### Tips

- Press **Ctrl+C** to gracefully stop the calculation. Already processed results are still written to stdout. You can resume later by feeding the partial output back in, enabling **resume from breakpoint**: `cat half.txt | tsubaki sum <type> stdin`.
- Combining `--plain-list` with `none` generates a simple file list.
- For recursive directory scanning, the order of arguments after `tsubaki sum <type> <dir1>` is not sensitive. This means you can write:

```bash
tsubaki sum md5 /home/user -v --balance --exclude=/home/user/{.cache,.config} /data # Note: <type> must be immediately followed by the first directory to scan, so the program can recognize the scanning strategy.
```

---

### `cmp <fileA> <fileB> [options]`

Compare two checksum files (each line format: `<checksum> <path>`). The program reads both files line by line and categorizes files as follows:

- **[!] Modified** – Same path, different checksum.
- **[D] Moved/Copied/Merged/Renamed** – Same checksum, different paths (grouped by checksum).
- **[U] Deleted or Added** – Files present only in A or only in B.
- **[=] Matched** – Same path and same checksum.

Output is printed to stdout.
**Options:** `--quiet` suppresses informational messages.

---

### `dup [options]`

Read a list from standard input in the format `<checksum> <path>` (no prefix). Identify duplicate files (same checksum) and print duplicate groups. Also suggests a deletion command (`rm`) for all files except the first in each group.

**Options:** `--quiet` suppresses informational messages.

---

### `help`

Display concise help information.

---

## Examples

### 1. Compute SHA256 for all files under `/home/user` and `/data`, excluding cache and config directories

```bash
tsubaki sum sha256 /home/user /data --exclude=/home/user/{.cache,.config} > home_checksums.txt
```

### 2. Generate a plain path list (no checksums) for all files under `/photos`

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
tsubaki cmp A.txt B.txt > comparison.txt
```

### 5. Find duplicate files in a photo collection

```bash
tsubaki sum md5 /photos | tsubaki dup
```

### 6. Resume an interrupted calculation

```bash
# First run (interrupted)
tsubaki sum sha256 /large_data > partial.txt
# Resume later (partial.txt contains already computed entries)
cat partial.txt | tsubaki sum sha256 stdin > complete.txt
```

### 7. Multi-threaded processing with 8 threads and load balancing

```bash
tsubaki sum sha256 /large_dir --thd-amount=8 --balance > sums.txt
```

---

## Exit Status

- **0** – Success (all files processed, no errors).
- **1** – Invalid command, error while scanning files, or error while computing checksums.
  In `sum` mode, if any file cannot be read or its hash cannot be computed, error messages are appended to the output, and the program returns `1`.

---

## Notes

- **Directory Scanning** – When scanning a directory, only regular files and symbolic links pointing to regular files are recognized. If you want to exclude such symlinks, you can add a filter like `--focus=/target_dir` (where `/target_dir` is the directory to verify).
- **Permission Denied** – While traversing directories, subdirectories that cannot be accessed due to insufficient permissions are silently skipped; scanning continues elsewhere.
- **Absolute Paths** – Currently, only absolute paths are supported. You must specify files using absolute paths, and the program will always output absolute paths.
- **Symbolic Links** – Directory symlinks are not followed by default. Use `--allow-symlinks` to follow them, but beware of cyclic links.
- **Multi-threading** – When enabled, the file list is distributed among threads. The default allocation is interleaved (round-robin) to balance load; `--balance` sorts files by size before interleaving, making the total data size handled by each thread more balanced.
- **Signal Handling** – When recursively calculating a directory, `SIGINT` (Ctrl+C) is caught. Threads exit after finishing the current file; already computed results are still written. If a thread is processing a very large file, it may take a while to exit; please be patient or send `SIGKILL`.
- **Memory Usage** – The file list is fully enumerated in memory first, then filtered according to the specified criteria. For very large directory trees (millions of files), this could become a bottleneck, but typical hardware can handle it.
- **System Support** – Due to the author's limited expertise, only Linux distributions are supported.
- **Display Language** – Since the author's development environment is in English, only the English version of Tsubaki is available. If you need a Chinese version, you can translate it yourself or request it.

---

## Source Code Structure

This project is a single source file. The source code structure is described in `ARCHITECTURE.md` (English) and `ARCHITECTURE.zh-cn.md` (Simplified Chinese).

---

## License

This project is licensed under the MIT License – see the `LICENSE` file for details.

---

## Developer

- **Nickname** - Lawrence Charland