# Tsubaki

Tsubaki 是一个校验和工具，用于验证文件完整性、比较目录和检测重复文件。它支持多种哈希算法（MD5、SHA1、SHA256、SHA512），并提供灵活的输入源——目录、文件列表或纯路径列表。多线程处理和智能缓冲区大小调整使其适用于大数据集。

## 特性

- **求和模式** – 递归计算目录中所有文件的校验和，或从标准输入读取文件列表进行计算。
- **合并模式** – 合并两个校验和列表（例如来自两个不同目录），并为每行添加 `A` 或 `B` 前缀，以便后续比较。
- **比较模式** – 读取合并后的列表，将文件分类为已修改、已移动/复制、已添加/删除或匹配。
- **重复模式** – 基于相同的校验和识别重复文件，并建议删除命令。
- **多线程** – 对于大型任务自动启用；可配置线程数和分块策略。
- **灵活过滤** – 包含或排除子目录，按文件大小过滤。
- **进度报告** – 详细模式显示进度条或当前正在处理的文件名。
- **优雅中断** – 按 Ctrl+C 可停止处理；已计算的结果仍会写入标准输出。

## 构建

Tsubaki 需要 C++17 编译器、OpenSSL 开发库和 pthreads（在类 Unix 系统上）。

### 依赖项

- 一台安装了Linux发行版的电脑
- C++17 标准库
- OpenSSL（libcrypto）
- POSIX 线程
- `filesystem`（C++17 的一部分；在较旧的编译器上可能需要 `-lstdc++fs`）

### 编译

```bash
g++ -std=c++17 -O3 -o tsubaki main.cpp -lssl -lcrypto -lpthread
```

在某些系统上，你可能需要添加 `-lstdc++fs`。如果你的编译器完全支持带有集成 `std::filesystem` 的 C++17，则不需要额外库。

## 用法

```
tsubaki <command> [options]
```

命令：`sum`, `mrg`, `cmp`, `dup`, `help`

### 全局选项

- `--quiet` – 禁止信息性消息（错误仍会输出到 stderr）。
- `-v`, `-vv` – 增加详细程度（仅对 `sum` 模式有效）。  
  `-v` 显示进度条；`-vv` 显示当前正在处理的文件名。

---

## 命令语法

### `sum <type> <path> [options]`

计算文件的校验和。

**`<type>`** – 可选值：`md5`、`sha1`、`sha256`、`sha512`、`none`。  
使用 `none` 时不计算校验和；输出为路径列表（可选择包含 `<NONE>` 占位符）。使用 `--plain-list` 仅输出路径。

**`<path>`** 可以是：
- **普通文件** – 打印其校验和（或 `<NONE>`）并退出。
- **目录** – 递归列出所有普通文件，计算校验和，并为每个文件输出 `<checksum> <relative-path>`。
- **`"stdin"`** – 读取现有列表，格式为 `<checksum> <path>`（以 `#` 开头的行将被忽略）。保留已有的校验和（除非使用 `--force-rescan`）以避免重新计算。
- **`"stdin-plain-list"`** – 读取纯文件路径（每行一个）并为其计算校验和。

#### `sum` 的选项

| 选项 | 描述 |
|------|------|
| `--exclude=<dir>` | 排除指定目录下的文件（可重复）。 |
| `--focus=<dir>`   | 仅包含指定目录下的文件（若重复则取并集）；在排除之后应用。 |
| `--max-size=<size>` | 跳过大于 `<size>` 的文件。示例：`10M`、`2G`、`1.5K`。后缀：`K`=KiB、`M`=MiB、`G`=GiB、`T`=TiB。 |
| `--min-size=<size>` | 跳过小于 `<size>` 的文件。 |
| `--allow-symlinks` | 跟随符号链接（谨慎使用；可能导致无限循环）。 |
| `--test`          | 不计算校验和——仅加载并过滤文件列表。 |
| `--plain-list`    | 当 `<type>` 为 `none` 时，仅输出文件路径（每行一个），不包含 `<NONE>` 占位符。 |
| `--disable-multi-thd` | 禁用多线程。默认情况下，当总大小 > 1 GiB 时启用多线程。 |
| `--thd-amount=<N>` | 设置线程数（默认：硬件并发数）。 |
| `--contiguous`    | 多线程时，将文件列表划分为连续块，而不是交错分配。 |
| `--force-rescan`  | 即使输入列表中的文件已有校验和，也重新计算（与 `path="stdin"` 一起使用）。 |
| `--buffer-size=<size>` | 使用静态缓冲区，大小精确为 `<size>` 字节。默认使用动态缓冲区（≤64 KB）。 |
| `--max-buffer-size=<size>` | 使用动态大小时限制最大缓冲区大小（若设置了 `--buffer-size` 则忽略此选项）。 |

#### 输出

对于多个文件，每行输出 `<checksum> <path>`（若使用 `--plain-list` 则仅为 `<path>`）。文件列表后附加一个摘要块：

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

如果发生任何错误，它们会列在末尾。如果至少报告了一个错误，程序将以状态 `1` 退出。

#### 提示

- 按 **Ctrl+C** 可优雅地停止计算。已处理的结果仍会写入标准输出。之后可以通过将半成品输出重新输入来继续：`cat half.txt | tsubaki sum <type> stdin`。
- 将 `--plain-list` 与 `none` 结合使用，可生成用于备份的简单文件列表。

---

### `mrg <fileA> <fileB> [options]`

合并两个现有的校验和列表（每行格式：`<checksum> <path>`）为一个列表，每行添加 `A ` 或 `B ` 前缀。结果打印到标准输出，基本只会用作 `cmp` 的输入。

**选项：** `--quiet` 禁止信息性消息。

---

### `cmp [options]`

从标准输入读取带有 `A ` 或 `B ` 前缀的行（一般是`mrg`模式的输出），格式如下：
```
A <checksum> <path>
B <checksum> <path>
```
（以 `#` 开头的行将被忽略）。比较两组文件并将其分类：

- **[!] 已修改** – 路径相同，校验和不同。
- **[D] 已移动/复制/合并/重命名** – 校验和相同，路径不同（按校验和分组）。
- **[U] 已删除或添加** – 仅出现在 A 或仅出现在 B 中的文件。
- **[=] 匹配** – 路径和校验和均相同。

输出打印到标准输出。  
**选项：** `--quiet` 禁止信息性消息。

---

### `dup [options]`

从标准输入读取一个列表，格式为 `<checksum> <path>`（无前缀）。识别重复文件（校验和相同）并打印重复组。还会为每组中除第一个文件外的所有文件建议删除命令（`rm`）。

**选项：** `--quiet` 禁止信息性消息。

---

### `help`

显示简洁的帮助信息（与本 README 开头所示相同）。

---

## 示例

### 1. 计算 `/home/user` 下所有文件的 SHA256，排除缓存和配置目录

```bash
tsubaki sum sha256 /home/user \
    --exclude=/home/user/.cache \
    --exclude=/home/user/.config \
    > home_checksums.txt
```

### 2. 生成 `/photos` 中所有文件的纯路径列表（无校验和）

```bash
tsubaki sum none /photos --plain-list > photo_list.txt
```

### 3. 为 `filelist.txt` 中列出的每个路径计算 MD5

```bash
cat filelist.txt | tsubaki sum md5 stdin-plain-list > file_checksums.txt
```

### 4. 比较两个目录

```bash
tsubaki sum sha256 /dirA > A.txt
tsubaki sum sha256 /dirB > B.txt
tsubaki mrg A.txt B.txt | tsubaki cmp > comparison.txt
```

### 5. 查找照片集中的重复文件

```bash
tsubaki sum md5 /photos | tsubaki dup
```

### 6. 恢复中断的计算

```bash
# 第一次运行（中断）
tsubaki sum sha256 /bigdata > partial.txt
# 稍后继续（partial.txt 包含已计算的条目）
cat partial.txt | tsubaki sum sha256 stdin --force-rescan > complete.txt
```

### 7. 使用 8 个线程和连续分块进行多线程处理

```bash
tsubaki sum sha256 /large_dir --thd-amount=8 --contiguous > sums.txt
```

---

## 退出状态

- **0** – 成功（所有文件均已处理，未发生错误）。
- **1** – 发生错误（无效参数、I/O 错误或至少一个校验和计算失败）。
  在 `sum` 模式下，如果有任何文件无法读取或其哈希无法计算，错误消息会附加到输出中，程序返回 `1`。

---

## 注意事项

- **符号链接** – 默认不跟随。使用 `--allow-symlinks` 可跟随，但需注意循环链接的风险。
- **文件大小过滤** – 大小检查是在**加载文件列表之后**进行的。对于目录，这意味着会先枚举所有文件，然后再过滤。对于包含大量文件的大目录，这可能会消耗内存。
- **多线程** – 启用时，文件列表会在线程间分配。默认分配方式是交错（轮询）以平衡负载；`--contiguous` 则划分为连续块。后者在文件大小差异较大时可能导致负载不均衡。
- **信号处理** – 会捕获 `SIGINT`（Ctrl+C）。线程完成当前文件后退出。已计算的结果仍会写入。
- **内存使用** – 文件列表完全存储在内存中。对于非常大的目录树（数百万个文件），这可能成为瓶颈。

---

## 许可证

Tsubaki 是个人项目，没有任何版权，你可以对它做任何事情。

---

## 开发者及联系方式

- **昵称** - Lawrence Charland
- **主邮箱** - 1470595583@qq.com
- **备用邮箱** - 3818604312@qq.com