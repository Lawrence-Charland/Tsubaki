# Tsubaki

一个用于文件完整性校验的命令行校验和工具，使用现代 C++20 编写。

[English](README.md)

## 功能特性

- **校验和计算** – 对文件与目录计算校验和，支持 OpenSSL 提供的全部摘要算法
  （MD4/MD5、SHA-1、SHA-2、SHA-3、SHAKE、BLAKE2）。
- **递归扫描** – 递归扫描目录，可选择跟随符号链接并容忍权限不足。
- **多种输入** – 支持普通文件、目录，以及从标准输入读取的 tsubaki 格式校验和
  列表或纯路径列表。
- **过滤** – 支持排除路径前缀，并按文件大小上下限过滤。
- **多线程** – 使用按硬件并发数配置的线程池并行计算，任务通过无锁队列分发，
  读取缓冲区按文件大小动态调整。
- **可续算** – 除非指定 `--force-scan`，输入列表中已有的哈希会被直接复用，因此
  中断后的任务可以续算。
- **进度条** – 当 stderr 为终端时显示实时进度，包含文件数量与已处理字节数。
- **输出清晰** – 结果输出到 stdout，日志输出到 stderr，末尾附带汇总报告。

## 环境要求

- 支持 C++20 的编译器（GCC 13+、Clang 16+、Apple Clang 15+ 或 MSVC 19.29+）
- CMake 3.16+
- OpenSSL 开发库
- Linux、macOS 或 Windows

## 构建

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

可执行文件位于 `build/tsubaki`。

### Windows 上的 OpenSSL

本项目不附带 OpenSSL，需要先自行安装。任选下面一种方式，然后用 CMake 配置项目。

**方式一：vcpkg（推荐）**

```sh
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install openssl:x64-windows

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

**方式二：预编译安装包**

安装 Win64 OpenSSL 3.x（例如来自
<https://slproweb.com/products/Win32OpenSSL.html>），然后指定安装路径：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64"
cmake --build build --config Release
```

**方式三：Chocolatey**

```sh
choco install openssl

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64"
cmake --build build --config Release
```

运行时需要能访问 `libcrypto-3-x64.dll`：把 OpenSSL 的 `bin` 目录加入 `PATH`，或将该
DLL 复制到 `tsubaki.exe` 同目录。

### CMake 选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | – | 计算校验和推荐使用 `Release` |
| `BUILD_TESTING` | `ON` | 构建测试（需要 GoogleTest） |
| `TSUBAKI_SANITIZE` | `OFF` | 开启 AddressSanitizer 与 UBSan |

## 用法

```text
tsubaki <命令> [选项] [路径...]
```

### 命令

| 命令 | 说明 |
| --- | --- |
| `sum <算法> <路径...>` | 计算文件与目录的校验和 |
| `cmp <文件A> <文件B>` | 比较两个校验和列表并报告差异 |
| `dup` | 从 stdin 的校验和列表查找重复文件 |
| `help [键]` | 显示英文帮助；给出 `键` 时显示该主题的详细信息 |
| `help-cn [键]` | 同 `help`，但输出中文 |

`键` 可以是命令、输入、选项或算法，例如 `tsubaki help sum`、
`tsubaki help --exclude`、`tsubaki help-cn stdin`。

### 比较与去重

`cmp` 与 `dup` 都消费 tsubaki 格式的校验和列表（每行一条 `<哈希> <路径>`），
例如 `sum` 的输出。

`dup` 从 stdin 读取列表，把哈希相同的记录分组输出，并在末尾给出一条建议的
`rm` 命令用于删除多余副本：

```sh
tsubaki sum sha256 ./photos | tsubaki dup
```

`cmp` 接收两个列表文件并报告：

| 标记 | 含义 |
| --- | --- |
| `[!]` | Modified：路径相同但哈希不同 |
| `[D]` | Moved/copied/renamed：哈希相同但路径不同 |
| `[U][A]` / `[U][B]` | 仅 A 有（删除）/ 仅 B 有（新增） |
| `[=]` | Matched：路径与哈希都相同 |

```sh
tsubaki cmp A.txt B.txt > comparison.txt
```

### `sum` 的输入

| 输入 | 说明 |
| --- | --- |
| `<路径>` | 普通文件，或将被递归扫描的目录 |
| `stdin` | 从标准输入读取 tsubaki 格式的校验和列表 |
| `stdin-plain-list` | 从标准输入读取纯路径列表，每行一个 |

### 算法

`md4`、`md5`、`sha1`、`sha224`、`sha256`、`sha384`、`sha512`、`sha512_224`、
`sha512_256`、`sha3_224`、`sha3_256`、`sha3_384`、`sha3_512`、`shake128`、
`shake256`、`blake2b512`、`blake2s256`。

### 选项

| 选项 | 说明 |
| --- | --- |
| `--exclude=PATH` | 排除以 `PATH` 开头的路径（可重复） |
| `--min-size=SIZE` | 仅包含不小于 `SIZE` 的文件 |
| `--max-size=SIZE` | 仅包含不大于 `SIZE` 的文件 |
| `--force-scan` | 即使输入列表中已有哈希也重新计算 |
| `--allow-symlinks` | 扫描时跟随目录符号链接 |
| `--threads=N` | 工作线程数（默认：硬件并发数） |
| `--test` | 仅扫描与统计，不计算校验和 |
| `--progress` | 强制显示进度条 |
| `--no-progress` | 强制关闭进度条 |
| `--log-level=LEVEL` | `DEBUG`、`INFO`、`WARN` 或 `ERROR`（默认 `INFO`） |
| `--quiet` | 已弃用，等价于 `--log-level=ERROR` |
| `-v` | 已弃用，等价于 `--log-level=INFO` |
| `-h`、`--help` | 显示英文总帮助 |
| `--help-cn` | 显示中文总帮助 |

`SIZE` 可带单位后缀：`b`、`k`、`m`、`g`、`t`、`p`（二进制，即 `1k` = 1024 字节）。

### 输出

每个已处理的文件在 stdout 输出一行：

```text
<哈希> <路径>
<NONE> <路径>          # 该文件无法处理
```

末尾附带汇总报告：

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

若运行过程中按 `Ctrl+C`（`SIGINT`）中断，报告仍会输出，并额外增加一行：

```text
# Interrupted: yes
```

### 退出码

| 退出码 | 含义 |
| --- | --- |
| `0` | 成功 |
| `1` | 参数错误、算法不支持，或存在处理失败的文件 |
| `130` | 被 `SIGINT`（`Ctrl+C`）中断 |

## 示例

```sh
# 计算单个文件
tsubaki sum sha256 ./archive.tar.gz

# 递归计算目录，排除子目录并忽略过小的文件
tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1k

# 输出结果，并在中断后从部分结果续算
tsubaki sum sha256 ./data > partial.txt
cat partial.txt | tsubaki sum sha256 stdin > complete.txt

# 从标准输入读取纯路径列表
printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list

# 只查看扫描/过滤结果，不计算哈希
tsubaki sum sha256 ./data --test

# 查找重复文件，以及比较两个校验和列表
tsubaki sum sha256 ./photos | tsubaki dup
tsubaki cmp before.txt after.txt

# 中文帮助，或查看单个主题的详细信息
tsubaki help-cn
tsubaki help --exclude
```

## 测试

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

测试基于 GoogleTest：优先通过 `find_package(GTest)` 查找，未安装时由 CMake
自动下载。

## 目录结构

```text
core/
  common/   日志级别配置、校验和列表解析、帮助文本
  cmp/      `cmp` 命令：比较两个校验和列表
  dup/      `dup` 命令：查找重复文件
  sum/      `sum` 命令：扫描、过滤与哈希计算
plugins/
  arg_parser.*   命令行解析
  encoder.*      OpenSSL 摘要查找与文件哈希
  file_list.*    文件元数据容器
  lock_free_queue.h  有界无锁 MPMC 队列（Vyukov）
  logger.*       分级日志
  thread_pool.*  用于哈希计算的线程池，基于无锁队列实现
  trie.*         `--exclude` 使用的路径前缀匹配
tests/      GoogleTest 测试套件
```

## 说明与限制

- 仅对普通文件计算校验和；除非指定 `--allow-symlinks`，否则不跟随目录符号链接。
- 无法访问的子目录会被跳过并给出警告。
- 输出中的路径会被规范化为绝对路径。
- `SIGINT`（`Ctrl+C`）会优雅地停止线程池；已完成的哈希仍会输出，汇总报告仍会打印
  并附带 `# Interrupted: yes`，进程以退出码 `130` 结束，结果可用于续算。
- 主线程等待单个文件的哈希最多 300ms；超时的文件会被暂存，待首轮遍历结束后再
  回读，避免慢文件阻塞快文件的结果输出。
- 进度条仅在 stderr 为终端、终端足够宽且日志级别不高于 `INFO` 时绘制；输出被
  重定向、日志被静默或终端过窄（换行会破坏 `\r` 原地刷新）时自动禁用。
- 文件列表在过滤前会完整载入内存，超大目录树会成为瓶颈。

## 许可证

基于 MIT 许可证发布，详见 [LICENSE](LICENSE)。

## 作者

- **Lawrence Charland**
