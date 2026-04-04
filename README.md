# Aethe 3 (Version 3.1.0)

Aethe 2 是一门**以数据流（Data-Flow）和管道语义（Pipeline Semantics）为核心**的实验性可编程动态语言。

相较于传统的嵌套函数调用结构（如 `step3(step2(step1(data)))`），Aethe 把“管道传送”提升为语言的第一等公民表达式。它的设计初衷是摆脱语法噪音，让代码逻辑像加工流水线一样顺滑、直观地自左向右推进。

本解释器为纯粹的**单文件 C++11 实现**，没有外部依赖，自带一个极客风的全终端自适应 IDE。

## 🎯 核心设计哲学

- **一切皆流转 (`|>` 管道化)**：深度融入 `|>`。你可以把纯数字、集合列阵、甚至是裸条件全部扔进管廊中传递加工，且支持自动首参注入与 `_` 切片截流。
- **Railway Safe Pipeline (`|?`)**：3.0 旗舰特性。用 `|?` 代替 `|>` 即可启用铁轨式容灾传递——管道中任何环节抛出的运行时异常都会被自动捕获封装为 `Err(msg)` 并沿轨道滑行至末端兜底，搭配 `Ok`/`Err` 包裹与 `match` 解构，让长链管道拥有 Rust/Elixir 级别的容错能力。
- **裸标识符（Bare Identifiers）**：在 Aethe 中，不存在到处写带引号的 `"field"`。未用 `$` 标定的裸单词天然就是“符号值”。这在进行字典取值或管道集联时带来了 EDSL 般的清爽体验。
- **安全与控制（Control & Return）**：拥有媲美 C++ 的严密赋值回写（覆盖引用）和强大的 `match` 防击穿守护匹配；同时内置 RAII 风格的 `defer` 后端清理池以保护资源。
- **一流的闭合体系**：包含普通函数 `fn` (或称 `flow`)、面向管道的天然 `$it` 接收器 `stream`（兼容 `stage`）、以及即拼即用的匿名 Lambda 模块 `pipe`，加上 `branch`/`guard` 高阶组合子，完美统治复杂抽象。
- **惰性流执行 (Lazy Stream)**：`range(...)` 现在返回惰性 `stream`，配合 `map/filter/skip/take/head/...` 按需拉取，不再先把大数组一次性铺满内存。
- **并行映射 `pmap`**：新增 `pmap(callable, ...)`，可直接并行处理大数组并保持结果顺序。

---

## 🛠️ 安装与构建

由于整个运行环境与解释器核心高度浓缩于一份源文件之中，你的 C++ 编译器就是最好的封包器：

```bash
# 本地完整并行版（pmap 真并行）
g++ -std=c++11 -DAETHE_ENABLE_THREADS=1 -pthread main.cpp -o aethe

# 在线编译器兼容版（无需 pthread，pmap 自动顺序降级）
g++ -std=c++11 main.cpp -o aethe
```

启动 Aethe 有三种模式：

1. **终端 IDE（默认）**: `./aethe`
   - 自带全屏自适应视图、行号及语法高亮。
   - `Ctrl-R` (下嵌视窗直接运行流)。
   - 支持自由复制、剪切、及智能 `Bracketed Paste`，并全兼容 Linux/MacOS 的主流快捷键。
2. **REPL 命令行交互**: `./aethe --repl`
   - 带有缓冲区的沉浸环境。输入代码不立即阻塞，键入 `run` 即可整合并一键求值。
3. **脚本单次执行**: `./aethe --run example.ae`
   - （推荐脚本扩展名为 `.ae`）

---

## 📖 语言概览体验 (The Aethe Way)

一瞥 Aethe 的美学与流水线特性：

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
