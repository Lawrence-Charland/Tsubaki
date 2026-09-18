#include "help.h"
#include <iostream>

namespace {

constexpr const char *kHelpEn = R"(Tsubaki - a checksum utility for file integrity, comparison and duplicate detection.

Usage:
  tsubaki <command> [options] [paths...]

Commands:
  sum <algorithm> <path...>   Compute checksums for files and directories
  cmp <fileA> <fileB>         Compare two checksum lists (A/B)
  dup                         Find duplicate files from a checksum list on stdin
  help [key]                  Show this help; with a key, show its details
  help-cn [key]               Same as 'help' but in Chinese

Inputs for 'sum':
  <path>              A regular file or a directory (scanned recursively)
  stdin               Read a tsubaki-format checksum list from stdin
  stdin-plain-list    Read a plain list of paths from stdin, one per line

Algorithms:
  md4 md5 sha1 sha224 sha256 sha384 sha512 sha512_224 sha512_256
  sha3_224 sha3_256 sha3_384 sha3_512 shake128 shake256 blake2b512 blake2s256

Options:
  --exclude=PATH      Exclude paths beginning with PATH (repeatable)
  --min-size=SIZE     Only include files not smaller than SIZE
  --max-size=SIZE     Only include files not larger than SIZE
  --force-scan        Recompute hashes even if present in the input list
  --allow-symlinks    Follow directory symlinks while scanning
  --threads=N         Number of worker threads (default: hardware concurrency)
  --test              Scan and report only; do not compute checksums
  --progress          Force the progress bar on (default: only on a TTY wide enough)
  --no-progress       Force the progress bar off
  --log-level=LEVEL   Log verbosity: DEBUG, INFO, WARN, ERROR (default: INFO)
  --quiet             Deprecated. Same as --log-level=ERROR
  -v                  Deprecated. Same as --log-level=INFO
  -h, --help          Show this help in English
  --help-cn           Show this help in Chinese

SIZE units: b, k, m, g, t, p (binary, 1k = 1024)

Output:
  <hash> <path>       One line per processed file
  <NONE> <path>       The file could not be processed
  # ...               Summary report appended at the end

Exit status:
  0   success
  1   invalid arguments, unsupported algorithm, or one or more failed files
  130 interrupted by SIGINT (Ctrl+C)

Examples:
  tsubaki sum sha256 ./data
  tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1m
  cat partial.txt | tsubaki sum sha256 stdin
  printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list
  tsubaki sum sha256 ./photos | tsubaki dup
  tsubaki cmp A.txt B.txt
  tsubaki help --exclude
)";

constexpr const char *kHelpCn = R"(Tsubaki - 用于文件完整性校验、比较与重复文件检测的校验和工具。

用法：
  tsubaki <命令> [选项] [路径...]

命令：
  sum <算法> <路径...>    计算文件/目录的校验和
  cmp <文件A> <文件B>     比较两个校验和列表（A/B）
  dup                     从 stdin 的校验和列表查找重复文件
  help [键]               显示帮助；给出键时显示其详细信息
  help-cn [键]            同 help，但输出中文

sum 的输入：
  <路径>              普通文件或目录（递归扫描）
  stdin               从标准输入读取 tsubaki 格式的校验和列表
  stdin-plain-list    从标准输入读取纯路径列表，每行一个

算法：
  md4 md5 sha1 sha224 sha256 sha384 sha512 sha512_224 sha512_256
  sha3_224 sha3_256 sha3_384 sha3_512 shake128 shake256 blake2b512 blake2s256

选项：
  --exclude=PATH      排除以 PATH 开头的路径（可重复）
  --min-size=SIZE     仅包含不小于 SIZE 的文件
  --max-size=SIZE     仅包含不大于 SIZE 的文件
  --force-scan        即使输入列表中已有哈希也重新计算
  --allow-symlinks    扫描时跟随目录符号链接
  --threads=N         工作线程数（默认：硬件并发数）
  --test              仅扫描与统计，不计算校验和
  --progress          强制显示进度条（默认仅在终端足够宽时显示）
  --no-progress       强制关闭进度条
  --log-level=LEVEL   日志级别：DEBUG、INFO、WARN、ERROR（默认 INFO）
  --quiet             已弃用，等价于 --log-level=ERROR
  -v                  已弃用，等价于 --log-level=INFO
  -h, --help          显示英文帮助
  --help-cn           显示中文帮助

SIZE 单位：b、k、m、g、t、p（二进制，1k = 1024）

输出：
  <哈希> <路径>       每个已处理文件一行
  <NONE> <路径>       该文件无法处理
  # ...              末尾附带的汇总报告

退出码：
  0   成功
  1   参数错误、算法不支持，或存在处理失败的文件
  130 被 SIGINT（Ctrl+C）中断

示例：
  tsubaki sum sha256 ./data
  tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1m
  cat partial.txt | tsubaki sum sha256 stdin
  printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list
  tsubaki sum sha256 ./photos | tsubaki dup
  tsubaki cmp A.txt B.txt
  tsubaki help --exclude
)";

// 各主题的详细帮助，键可为命令、输入、选项或算法。
struct HelpTopic {
    std::string_view key;
    std::string_view en;
    std::string_view cn;
};

constexpr HelpTopic kTopics[] = {
    {"sum",
     R"(sum <algorithm> <path...>

Compute a checksum for every regular file given directly, or found by
recursively scanning a directory. Already-computed hashes in the input list are
reused unless --force-scan is set. Results go to stdout; logs and the optional
progress bar go to stderr.

Related: stdin, stdin-plain-list, --force-scan, --exclude, algorithms
)",
     R"(sum <算法> <路径...>

对直接给出的普通文件，或递归扫描目录得到的每个文件计算校验和。除非设置了
--force-scan，否则输入列表中已有的哈希会被复用。结果输出到 stdout；日志与可选
进度条输出到 stderr。

相关：stdin、stdin-plain-list、--force-scan、--exclude、algorithms
)"},
    {"help",
     R"(help [key]

Show the general help, or detailed information about a specific command, input,
option, or algorithm when a key is given.

Examples:
  tsubaki help
  tsubaki help sum
  tsubaki help --exclude
)",
     R"(help [键]

显示总帮助；当给出键时，显示某个命令、输入、选项或算法的详细信息。

示例：
  tsubaki help
  tsubaki help sum
  tsubaki help --exclude
)"},
    {"help-cn",
     R"(help-cn [key]

Same as 'help', but prints the Chinese documentation.

Examples:
  tsubaki help-cn
  tsubaki help-cn sum
)",
     R"(help-cn [键]

与 help 相同，但输出中文文档。

示例：
  tsubaki help-cn
  tsubaki help-cn sum
)"},
    {"cmp",
     R"(cmp <fileA> <fileB>

Compare two tsubaki-format checksum lists (one entry per line: <hash> <path>)
and report how they differ:

  [!] Modified              Same path, different checksum
  [D] Moved/copied/...      Same checksum under different paths
  [U] Deleted or added      Present only in A ([U][A]) or only in B ([U][B])
  [=] Matched               Same path and checksum

The two files are usually produced by 'sum'. Results go to stdout.

Example:
  tsubaki cmp A.txt B.txt > comparison.txt
)",
     R"(cmp <文件A> <文件B>

比较两个 tsubaki 格式的校验和列表（每行 "<哈希> <路径>"），并报告差异：

  [!] Modified              路径相同但哈希不同
  [D] Moved/copied/...      哈希相同但路径不同（移动/复制/重命名）
  [U] Deleted or added      仅 A 有（[U][A]）或仅 B 有（[U][B]）
  [=] Matched               路径与哈希都相同

两个文件通常由 sum 生成，结果输出到 stdout。

示例：
  tsubaki cmp A.txt B.txt > comparison.txt
)"},
    {"dup",
     R"(dup

Read a tsubaki-format checksum list from stdin and group files that share the
same checksum. Prints each duplicate group and a suggested 'rm' command.

Example:
  tsubaki sum sha256 ./photos | tsubaki dup
)",
     R"(dup

从 stdin 读取 tsubaki 格式的校验和列表，把哈希相同的文件归为一组，输出每组
重复文件以及一条建议的删除命令。

示例：
  tsubaki sum sha256 ./photos | tsubaki dup
)"},
    {"stdin",
     R"(stdin

Read a tsubaki-format checksum list from standard input, one entry per line:

  <hash> <path>

Empty lines, lines starting with '#', and lines starting with '[' are ignored.
Entries whose hash is already present are reused, which makes an interrupted
run resumable.

Example:
  cat partial.txt | tsubaki sum sha256 stdin
)",
     R"(stdin

从标准输入读取 tsubaki 格式的校验和列表，每行一条：

  <哈希> <路径>

空行、以 '#' 开头的行和以 '[' 开头的行会被忽略。若某条目的哈希已存在，则会复用
该哈希，因此中断后的任务可以续算。

示例：
  cat partial.txt | tsubaki sum sha256 stdin
)"},
    {"stdin-plain-list",
     R"(stdin-plain-list

Read a plain list of paths from standard input, one per line. Leading and
trailing whitespace is stripped and lines starting with '#' are ignored. Each
listed path is then scanned as usual.

Example:
  printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list
)",
     R"(stdin-plain-list

从标准输入读取纯路径列表，每行一个。会去除首尾空白并忽略以 '#' 开头的行。列表
中的每个路径随后会照常扫描。

示例：
  printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list
)"},
    {"algorithms",
     R"(algorithms

Available digests:

  md4 md5 sha1 sha224 sha256 sha384 sha512 sha512_224 sha512_256
  sha3_224 sha3_256 sha3_384 sha3_512 shake128 shake256 blake2b512 blake2s256

The digest is the first argument of 'sum':
  tsubaki sum sha256 ./data
)",
     R"(algorithms

可用算法：

  md4 md5 sha1 sha224 sha256 sha384 sha512 sha512_224 sha512_256
  sha3_224 sha3_256 sha3_384 sha3_512 shake128 shake256 blake2b512 blake2s256

算法是 sum 的第一个参数：
  tsubaki sum sha256 ./data
)"},
    {"--exclude",
     R"(--exclude=PATH

Exclude every path that begins with PATH. May be repeated to exclude several
subtrees. PATH is made absolute and normalized before matching.

Example:
  tsubaki sum sha256 ./data --exclude=./data/.cache
)",
     R"(--exclude=PATH

排除所有以 PATH 开头的路径。可重复指定以排除多个子树。匹配前会把 PATH 转为
绝对路径并规范化。

示例：
  tsubaki sum sha256 ./data --exclude=./data/.cache
)"},
    {"--min-size",
     R"(--min-size=SIZE

Only include files whose size is greater than or equal to SIZE. SIZE accepts
the units b, k, m, g, t, p (binary, 1k = 1024).

Example:
  tsubaki sum sha256 ./data --min-size=1m
)",
     R"(--min-size=SIZE

仅包含大小不小于 SIZE 的文件。SIZE 支持单位 b、k、m、g、t、p（二进制，
1k = 1024）。

示例：
  tsubaki sum sha256 ./data --min-size=1m
)"},
    {"--max-size",
     R"(--max-size=SIZE

Only include files whose size is less than or equal to SIZE. SIZE accepts the
units b, k, m, g, t, p (binary, 1k = 1024).

Example:
  tsubaki sum sha256 ./data --max-size=100m
)",
     R"(--max-size=SIZE

仅包含大小不大于 SIZE 的文件。SIZE 支持单位 b、k、m、g、t、p（二进制，
1k = 1024）。

示例：
  tsubaki sum sha256 ./data --max-size=100m
)"},
    {"--force-scan",
     R"(--force-scan

Recompute hashes even when the input list already contains one for a file.
Without this flag, existing hashes are reused so an interrupted run can be
resumed.
)",
     R"(--force-scan

即使输入列表中已有某文件的哈希，也重新计算。不设置该选项时，已有哈希会被复用，
以便中断后继续。
)"},
    {"--allow-symlinks",
     R"(--allow-symlinks

Follow directory symlinks while scanning directories. Without it, symlinked
directories are not descended into.
)",
     R"(--allow-symlinks

扫描目录时跟随目录符号链接。不设置时不会进入符号链接指向的目录。
)"},
    {"--threads",
     R"(--threads=N

Number of worker threads used to hash files. Defaults to the hardware
concurrency reported by the system. The value must be a positive integer.

Example:
  tsubaki sum sha256 ./data --threads=8
)",
     R"(--threads=N

用于计算哈希的工作线程数，默认为系统报告的硬件并发数。取值必须为正整数。

示例：
  tsubaki sum sha256 ./data --threads=8
)"},
    {"--test",
     R"(--test

Scan and filter only; print the summary report without computing any checksums.
)",
     R"(--test

仅扫描与过滤，不计算任何校验和，只打印汇总报告。
)"},
    {"--progress",
     R"(--progress

Force the progress bar on. By default it is shown only when stderr is a terminal
wide enough for the whole line and the log level is INFO or lower.
)",
     R"(--progress

强制显示进度条。默认仅在 stderr 为终端、终端足够宽且日志级别不高于 INFO 时显示。
)"},
    {"--no-progress",
     R"(--no-progress

Force the progress bar off.
)",
     R"(--no-progress

强制关闭进度条。
)"},
    {"--log-level",
     R"(--log-level=LEVEL

Set the minimum log verbosity: DEBUG, INFO, WARN, or ERROR. Default is INFO.
Logs are written to stderr.
)",
     R"(--log-level=LEVEL

设置最低日志级别：DEBUG、INFO、WARN 或 ERROR，默认 INFO。日志输出到 stderr。
)"},
    {"--quiet",
     R"(--quiet

Deprecated. Equivalent to --log-level=ERROR.
)",
     R"(--quiet

已弃用，等价于 --log-level=ERROR。
)"},
    {"-v",
     R"(-v

Deprecated. Equivalent to --log-level=INFO.
)",
     R"(-v

已弃用，等价于 --log-level=INFO。
)"},
    {"-h",
     R"(-h, --help

Show the general help in English. Use 'help [key]' for details about a topic.
)",
     R"(-h, --help

显示英文总帮助。使用 'help [键]' 查看某个主题的详细信息。
)"},
    {"--help",
     R"(--help

Show the general help in English. Use 'help [key]' for details about a topic.
)",
     R"(--help

显示英文总帮助。使用 'help [键]' 查看某个主题的详细信息。
)"},
    {"--help-cn",
     R"(--help-cn

Show the general help in Chinese. Use 'help-cn [key]' for details about a
topic.
)",
     R"(--help-cn

显示中文总帮助。使用 'help-cn [键]' 查看某个主题的详细信息。
)"},
    {"exit-status",
     R"(exit-status

  0    success
  1    invalid arguments, unsupported algorithm, or one or more failed files
  130  interrupted by SIGINT (Ctrl+C)
)",
     R"(exit-status

  0    成功
  1    参数错误、算法不支持，或存在处理失败的文件
  130  被 SIGINT（Ctrl+C）中断
)"},
};

} // namespace

void common::printHelp(bool chinese) {
    std::cout << (chinese ? kHelpCn : kHelpEn);
}

bool common::printHelpTopic(std::string_view topic, bool chinese) {
    for (const auto &entry : kTopics) {
        if (entry.key == topic) {
            std::cout << (chinese ? entry.cn : entry.en);
            return true;
        }
    }
    return false;
}
