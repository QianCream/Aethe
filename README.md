# Aethe 4 (Version 4.0.0)

Aethe 4 是一门**以数据流（Data-Flow）和管道语义（Pipeline Semantics）为核心**的实验性可编程语言。

相较于传统的嵌套函数调用结构（如 `step3(step2(step1(data)))`），Aethe 把“管道传送”提升为语言的第一等公民表达式。它的设计初衷是摆脱语法噪音，让代码逻辑像加工流水线一样顺滑、直观地自左向右推进。

当前实现为纯粹的**单文件 C++11 编译器 + 字节码虚拟机**，没有外部依赖，自带一个极客风的全终端自适应 IDE。核心执行路径为：

```text
source -> lexer/parser -> custom bytecode -> VM
```

字节码在进入 VM 前会先跑一轮轻量优化，目前以常量折叠、冗余跳转消除和简单 peephole 压缩为主。

为了控制复杂度，当前版本只保留围绕管道与基础控制流的一组核心语法；超出该子集的高级特性会在编译阶段直接报错，不再回退到解释执行。运行时也不再提供语言层文件读取能力，宿主 CLI 也只接受 IDE 缓冲区或 `stdin` 源码。

注意：下文部分示例和设计说明保留了历史版本特性，用于展示语言演化方向；当前保证可编译的主子集集中在 `flow` / `stream` / `pipe`、`when` / `while`、数组字典、调用、`|>` / `|?` 与基础内建。诸如 `type`、`match`、`for`、`defer` 等段落现在应视为历史说明。

## 🎯 核心设计哲学

- **一切皆流转 (`|>` 管道化)**：把值沿流水线向右推送，优先表达“数据怎么流动”，而不是“括号怎样嵌套”。
- **Railway Safe Pipeline (`|?`)**：容错管道仍然是一等能力，错误会被包装后沿流水线继续传播，避免长链手写样板判断。
- **裸标识符（Bare Identifiers）**：字段名、标签名、筛选值直接写成裸单词，尽量减少引号与样板语法。
- **一流闭包体系**：`flow` / `stream` / `pipe` 负责不同粒度的复用和组合，统一落到编译后的自定义字节码上执行。
- **沙盒优先**：语言层不再提供文件读取，宿主侧也不再接受脚本文件路径，避免把运行模型重新拖回宿主环境依赖。

---

## 🛠️ 安装与构建

由于整个运行环境与编译器/VM 核心高度浓缩于一份源文件之中，你的 C++ 编译器就是最好的封包器：

```bash
# 本地完整并行版（pmap 真并行）
g++ -std=c++11 -DAETHE_ENABLE_THREADS=1 -pthread main.cpp -o aethe

# 在线编译器兼容版（无需 pthread，pmap 自动顺序降级）
g++ -std=c++11 main.cpp -o aethe
```

如果你走 CMake，当前仓库也已经把编译优化直接写在 `CMakeLists.txt` 里：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`Release` 会启用更激进的优化选项；如果工具链支持，也会自动打开 IPO/LTO。

启动 Aethe 有三种模式：

1. **终端 IDE（默认）**: `./aethe`
   - 自带全屏自适应视图、行号和下方输出窗口。
   - `Ctrl-R` (下嵌视窗直接运行流)。
   - 保留最基础的编辑与运行能力，外围交互能力已刻意收窄。
2. **标准输入单次执行**: `cat example.ae | ./aethe --stdin`
   - 也可以直接 `cat example.ae | ./aethe`，非交互模式下会默认从 `stdin` 读取源码并编译执行。
3. **导出字节码**: `cat example.ae | ./aethe --dump-bytecode`
   - 打印当前源码编译出的自定义字节码，方便调试编译结果与 VM 指令流。

---

## 📖 语言概览体验 (The Aethe Way)

一瞥 Aethe 的美学与流水线特性。下面第 1/2 节示例属于当前编译子集；第 3 节及之后的段落保留为历史设计展示，不保证当前版本可直接编译：

### 1. 流畅的管道加工与裸字符
不需要嵌套函数，也不需要写那些冗余的引号：
```scala
let users = [
    {name: "Alice", role: admin, score: 95},
    {name: "Bob", role: guest, score: 88},
    {name: "Carol", role: admin, score: 91}
];

// 核心操作：查找 role 为 admin 的人，并提取她们的名字成数组
$users |> where(role, admin) |> pluck(name) |> emit;    
// 结果: [Alice, Carol]

// 统计各个 role 的人员数量，直接生成字典
$users |> count_by(role) |> emit;
```

### 2. 匿名闭包与动态挂载
无需命名，通过 `pipe` 或 `bind` 拼接你的逻辑：
```scala
// map + 匿名管道捕获
[1, 2, 3, 4] |> map(pipe(x) { return $x * 10; }) |> emit; 

// 运用高阶组合子串联不同加工函数
let loud = chain(trim, upper, bind(concat, "!!!"));
"  hello  " |> $loud |> emit; // 输出：HELLO!!!
```

### 3. 可退化与进化的结构对象
摆脱 C++ 极度重装甲的类继承，建立天然依附方法的纯粹结构模型：
```scala
type User(name, score) {
    fn badge() {
        when $self.score >= 90 { return "A"; } 
        else { return "B"; }
    }
}

let alice = User("Alice", 85);
$alice.score += 7;          // 高级左值穿透，深度更改原状态
$alice.badge() |> emit;     // 产出 "A" 
```

### 4. Railway Safe Pipeline `|?` (3.0 NEW)
用 `|?` 代替 `|>` 即可开启容灾管道——任何运行时异常都会被自动捕获为 `Err()` 并沿轨道跳过后续阶段：
```scala
let result = user_input
    |? parse_json
    |? pluck(score)
    |? pipe(x) { return $x / 0; }   // Runtime error auto-caught!
    |? pipe(x) { return $x + 100; }; // Skipped because above is Err

match $result {
    case Ok(value) { "Got: " + str($value) |> emit; }
    case Err(msg)  { "Safe: " + str($msg) |> emit; }
}
```

### 5. Match + Ok/Err Destructuring
```scala
match $score {
    case _ when $it >= 90 { "Excellent" |> emit; }
    case _ when $it >= 60 { "Pass" |> emit; }
    else { "Fail" |> emit; }
}
```

### 6. Lazy + Parallel (3.1 NEW)
```scala
// 惰性：不会先构造 1000000 项数组
range(1000000) |> map(bind(add, 1)) |> take(5) |> emit;
// [1, 2, 3, 4, 5]

// stream 关键字（兼容旧 stage）
stream boost(n) {
    return $it + $n;
}
10 |> boost(5) |> emit; // 15

// 并行映射 pmap
range(10000) |> pmap(bind(mul, 2)) |> take(5) |> emit;
// [0, 2, 4, 6, 8]
```

---

## 📚 文档指南

若要彻底玩转这门语言，推荐遵循以下阅读索引：

1. **[TUTORIAL.md](./TUTORIAL.md)**：零基础入门教程。新接触者务必从这里启程，领略从 REPL 敲打出第一个管道指令，到掌握类与对象机制的全栈演进。
2. **[REFERENCE.md](./REFERENCE.md)**：Aethe 系统白皮书级别参考。它包含了语法设计模型、语义界定、超 **50+ 内建处理流水线功能（map、reduce、evolve、derive 等）** 的详尽入参/返回值规范，更囊括了横向跨度至 C++ 端的执行原理与内存行为机制极深分析。
3. **[CHANGELOG.md](./CHANGELOG.md)**：所有版本的历程演进记录与版本迭代路线。

---

## 🚫 边界与限制

当前的试验性实施受制于如下边界限制：
- 当前运行状态不存在静态类型系统探测（无 Type hints）。
- 尚未接轨基于原生源层面的模块（Modules）与跨文件引出导入的支持体系。
- 系统极客 IDE 的底层显示渲染与操作高度依赖 TTY / ANSI 标准操作转义控制码，若挂载于缺失该标准的虚无极简控制台环境中存在发生乱码及光标跑位的情况。
