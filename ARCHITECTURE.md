### Attention

This ARCHITECTURE was translated by AI from the Chinese version. If there are any errors, please refer to the Chinese version or the source code.

---

## Tsubaki Source Code Structure Overview

Tsubaki is a command-line tool written in C++17, primarily designed for calculating file checksums, comparing directory differences, merging checksum lists, and detecting duplicate files. The source code is organized as a single file with multiple namespaces.

### 1. Overall Organization

The program entry `main()` dispatches control to corresponding handler functions based on the first command-line argument (subcommand). These handler functions are distributed across different namespaces and share a set of common utility components:

- **`cmd` namespace**: Parses command-line arguments, converting parameters in the form `--key=value` into a queryable set of key-value pairs, and provides helper query functions.
- **`file` namespace**: Defines the core data structure `file_list` and related operations (e.g., directory traversal, reading file lists, filtering, sorting).
- **`sum` namespace**: Encapsulates hash calculation functions based on OpenSSL, supporting MD5, SHA1, SHA256, SHA512, and provides multithreaded computation with progress display.
- **`cmp` namespace**: Provides comparison functions for sorting (by path or checksum).
- **`major_func` namespace**: Contains the concrete implementations of each subcommand (`mode_sum`, `mode_mrg`, `mode_cmp`, `mode_dup`, `mode_help`).

### 2. Core Data Structure: `file_list`

The `file_list` class (located in the `file` namespace) is the primary data container throughout the program. Internally, it maintains a `std::vector<sum_and_dir>`, where the `sum_and_dir` struct records the type tag, checksum, path, size, and status flags for a single file.

`file_list` provides the following fundamental operations:

- **Loading data**: Can recursively scan directories from the filesystem or read from standard input (supporting tsubaki format or plain path lists).
- **Filtering data**: Filters files based on conditions such as `--exclude`, `--focus`, and size limits.
- **Deduplication and merging**: Deduplicates by path and can merge data from two `file_list` instances.
- **Status maintenance**: Records error messages generated during processing for later output.

### 3. Subcommand Processing Flow

Each subcommand's handler function generally follows these steps:

1. Parse the subcommand-specific arguments and validate the input source.
2. Construct a `file_list` object and load the raw data.
3. Preprocess: refresh file sizes, apply filters, and mark invalid entries.
4. Perform the core computation (possibly launching multiple threads and calling functions in the `sum` namespace).
5. Output the results (typically still the contents of `file_list`) to standard output, while writing error messages to standard error or appending them to the end of the output.
6. Return an appropriate exit code based on whether errors occurred.

### 4. Multithreading and Error Handling
- **Multithreaded computation**: `sum::calculate_dir_checksum` is called in parallel by multiple threads. Each thread processes a slice of the `file_list`, with the slicing strategy controlled by the `--contiguous` option. Atomic variables track the number of processed files and an interrupt flag, allowing safe termination with Ctrl+C.
- **Error handling**: For non-fatal errors (e.g., file cannot be opened, invalid checksum format), the program stores the error message in `file_list::error_stack` and continues execution. The exit status is determined by whether the error stack is empty at the end. Fatal errors throw exceptions, which are caught by `main` and terminate the program.

### 5. Key Design Points
- **Data-driven**: Most functionality is implemented by transforming the data within `file_list`. Adding a new subcommand only requires reusing existing operations without modifying the underlying logic.
- **Reusable parameters**: Command-line parsing supports repeated keys (e.g., multiple `--exclude`), making it easy to express complex filter conditions.
- **Dynamic buffer**: When calculating checksums, the read buffer size is dynamically adjusted based on file size, balancing memory usage and I/O performance.
- **Separate output**: Normal results go to `stdout`, log messages go to `stderr` (can be suppressed with `--quiet`), and error messages are both written to `stderr` and appended to the end of `stdout` for user convenience.

### 6. Extensibility
- Adding a new checksum algorithm only requires adding a branch in `sum::calculate_file_checksum`.
- New subcommands can be added by defining a function in the `major_func` namespace and registering it in `main`'s dispatch logic, reusing the existing `file_list` and argument parsing utilities.

The above is a basic overview of Tsubaki's source code structure, hoping it helps in understanding the code.