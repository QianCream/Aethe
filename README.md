# Aethe

Aethe 是一个以管道语法为核心的实验性语言实现。当前实现为单文件 C++11 解释器，运行形态为 REPL，重点关注数据流表达而不是传统的嵌套调用结构。

## 状态

- 单文件实现，核心代码位于 [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)
- 兼容 C++11
- 仅支持 REPL
- 不支持脚本文件执行
- 支持 `fn`、`stage`、`type`
- 支持 `when`、`match`、`while`、`for`
- 支持 `break`、`continue`、`defer`
- 支持自动首参注入、占位符 `_`、裸表达式管道

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

- [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)：完整语言参考，包含语法、语义、内建阶段、运行时行为
- [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)：实现代码
- [CMakeLists.txt](/Users/armand/Personal/CLion/Aethe/CMakeLists.txt)：最小 CMake 构建配置

## 最小示例

```scala
fn double(x) {
    return $x * 2;
}

"hello" |> upper |> emit;
21 |> double |> emit;
"Hello, Aethe!" |> substring(_, 7, 3) |> emit;
10 |> _ * 3 + 5 |> emit;
```
