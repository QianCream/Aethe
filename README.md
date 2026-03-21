# Aethe 2

Aethe 是一个以管道语法为核心的实验性语言实现。当前实现为单文件 C++11 解释器，运行形态为 REPL，重点关注数据流表达而不是传统的嵌套调用结构。

当前工作区对应 `Aethe 2 / 2.1.0`。这一版在 `2.0.0` 的一等公民 `pipe` 之上继续推进成“管道组合子”。除了能直接写 `pipe(x) { ... }`，现在还可以用 `bind(...)`、`chain(...)`、`branch(...)`、`guard(...)` 拼出新的可调用管道。

另一个核心特点仍然保留：裸标识符就是符号值，`$name` 才是变量读取。这让 `into(score)`、`get(name)`、`where(role, admin)`、`index_by(name)` 这类写法可以直接把名字放进管道，而不需要到处补引号。

## 状态

- 单文件实现，核心代码位于 [main.cpp]
- 兼容 C++11
- 仅支持 REPL
- 不支持脚本文件执行
- 支持 `fn`、`stage`、`type`
- 支持匿名 `pipe(...) { ... }` 管道值与闭包捕获
- 支持 `bind`、`chain`、`branch`、`guard` 这组返回 `callable` 的组合子
- 支持 `when`、`match`、`while`、`for`
- 支持 `break`、`continue`、`defer`
- 支持自动首参注入、占位符 `_`、裸表达式管道
- 支持裸标识符作为管道标识符，可直接表达变量名、字段名和筛选值
- 支持 `input`、`bind`、`chain`、`branch`、`guard`、`tap`、`map`、`filter`、`each`、`reduce`、`chunk`、`zip`、`flat_map`、`group_by`、`pluck`、`where`、`index_by`、`count_by`、`sort_by`、`sort_desc_by`、`distinct_by`、`sum_by`、`evolve`、`derive`、`get`、`set`、`entries`、`pick`、`omit`、`merge`、`rename`、`contains`、`replace`、`slice`、`reverse`、`index_of`、`repeat`、`sum`、`flatten`、`take`、`skip`、`distinct`、`sort`、`sort_desc`、`find`、`all`、`any`、`type_of` 等内建能力

## 构建

```bash
g++ -std=c++11 main.cpp -o aethe
```

## 运行

```bash
./aethe
```

REPL 提示符：

- `>>>`：开始新的输入缓冲区
- `...>`：继续当前输入缓冲区

执行模型：

- 输入内容不会自动执行
- 输入 `run` 后统一解析并执行当前缓冲区
- 输入 `exit`、`quit` 或发送 `Ctrl+D` 可退出
- 程序运行时可以通过 `input()` 或 `input("prompt")` 读取一行标准输入

示例：

```text
>>> fn double(x) {
...>     return $x * 2;
...> }
...> 21 |> double |> emit;
...> run
42
>>>
```

## 文档

- [CHANGELOG.md](/Users/armand/Personal/CLion/Aethe/CHANGELOG.md)：版本更新说明，按 `1.1.0`、`1.2.0`、`1.3.0` 等阶段记录功能演进
- [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md)：零基础入门教程，从 REPL、值、管道、函数到对象与循环逐步展开
- [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)：完整语言参考，包含语法、语义、内建阶段、运行时行为
- [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)：实现代码
- [CMakeLists.txt](/Users/armand/Personal/CLion/Aethe/CMakeLists.txt)：最小 CMake 构建配置

建议阅读顺序：

1. 先读 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md)
2. 再查 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)

## 最小示例

```scala
fn double(x) {
    return $x * 2;
}

"hello" |> upper |> emit;
21 |> double |> emit;
"Hello, Aethe!" |> substring(_, 7, 3) |> emit;
10 |> _ * 3 + 5 |> emit;
[1, 2, 3, 4] |> chunk(2) |> emit;
["A", "B", "C"] |> zip([1, 2]) |> emit;
[{name: "Alice", role: admin}, {name: "Bob", role: guest}, {name: "Carol", role: admin}]
    |> where(role, admin)
    |> pluck(name)
    |> emit;
[{name: "Alice", role: admin}, {name: "Bob", role: guest}, {name: "Carol", role: admin}]
    |> count_by(role)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}, {name: "Carol", score: 95}]
    |> distinct_by(score)
    |> sort_by(name)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}, {name: "Carol", score: 91}]
    |> sum_by(score)
    |> emit;
[{name: " alice "}, {name: "bob"}]
    |> evolve(name, trim)
    |> evolve(name, upper)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}]
    |> index_by(name)
    |> emit;
let double = pipe(x) {
    return $x * 2;
};
21 |> $double |> emit;
[1, 2, 3, 4] |> map(pipe(x) { return $x * 10; }) |> emit;
[1, 2, 3, 4] |> reduce(pipe(acc, item) { return $acc + $item; }, 0) |> emit;
let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;
[1, 2, 3] |> map(bind(add, 10)) |> emit;
"Aethe" |> branch(type_of, str, bind(concat, "!")) |> emit;
5 |> guard(pipe(x) { return $x > 3; }, bind(add, 100), bind(sub, 100)) |> emit;
{name: "Alice", score: 95}
    |> derive(kind, type_of)
    |> emit;
{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
```
