# Aethe 2 语言参考

本文档描述 `Aethe 3 / 3.1.0` 的词法、语法、语义、运行时行为与所有内建能力。按参考手册的方式组织，适合在你知道某个概念但忘了细节时来查。

如果你还没读过 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md)，建议先读教程，再来查参考。

---

## 目录

1. [实现范围](#1-实现范围)
2. [词法约定](#2-词法约定)
3. [类型与值](#3-类型与值)
4. [真值规则](#4-真值规则)
5. [变量与裸标识符](#5-变量与裸标识符)
6. [表达式](#6-表达式)
7. [管道语义](#7-管道语义)
8. [语句](#8-语句)
9. [可调用对象](#9-可调用对象)
10. [对象系统](#10-对象系统)
11. [运行模型](#11-运行模型)
12. [内建能力](#12-内建能力)
13. [内建名称索引](#13-内建名称索引)
14. [错误行为](#14-错误行为)
15. [示例](#15-示例)
16. [未实现特性](#16-未实现特性)
17. [内存模型与对象生命周期](#17-内存模型与对象生命周期)
18. [作用域与名字解析](#18-作用域与名字解析)
19. [求值顺序与副作用](#19-求值顺序与副作用)
20. [实现定义与未定义行为](#20-实现定义与未定义行为)

---

## 1. 实现范围

Aethe 当前是一个单文件 C++11 解释器，源码在 [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)。

已实现：

- 函数 `fn` / `flow`、管道阶段 `stage`、匿名管道值 `pipe(...) { ... }`
- 对象构造与方法 `type`
- 条件 `when`、多分支 `match`、循环 `while` / `for`、延迟执行 `defer`
- 赋值与复合赋值 `$x = ...`、`$x += ...`
- 自动首参注入、占位符 `_`、裸表达式管道
- 裸标识符作为符号值——变量名、字段名、筛选值都直接写进管道
- 管道组合子 `bind`、`chain`、`branch`、`guard`
- 丰富的字符串、容器、集合式管道与类型转换内建

未实现：

- 静态类型系统 / 类型注解
- 模块与导入系统

运行方式：

- 默认启动终端 IDE
- `--repl` 进入旧 REPL
- `--run <file.ae>` 单次执行脚本文件

---

## 2. 词法约定

### 2.1 空白符

空格、制表符、换行符和回车符用于分隔记号。除字符串内部外，空白不影响语义。

### 2.2 注释

只支持单行注释：

```scala
// 这是注释
"hello" |> emit; // 这也是注释
```

从 `//` 开始到行尾。不支持 `/* ... */`。

### 2.3 标识符

标识符用于函数名、stage 名、类型名、变量名、字段名、裸标识符值。

规则：

- 首字符：字母或下划线
- 后续字符：字母、数字或下划线

```scala
name
user_score
_tmp
Value42
```

### 2.4 关键字

```text
fn  flow  stage  stream  type  pipe
when  else  match  case
while  for  in
let  give  return
defer  break  continue
true  false  nil
```

### 2.5 数字字面量

只支持十进制整数：

```scala
0
42
1024
```

不支持浮点数、十六进制。负数通过一元 `-` 运算生成，`-3` 实际上是 `-(3)`。

### 2.6 字符串字面量

双引号包围：

```scala
"hello"
"line1\nline2"
```

支持的转义序列：

| 转义 | 含义 |
|------|------|
| `\n` | 换行 |
| `\t` | 制表符 |
| `\"` | 双引号 |
| `\\` | 反斜杠 |

非法转义会触发词法错误。

### 2.7 符号

```text
|>
( ) { } [ ]
, : ;
$ . _
+ - * / %
! == != > >= < <=
&& ||
= += -= *= /= %=
```

---

## 3. 类型与值

Aethe 是动态类型语言。每个值在运行时携带类型标签。

### 3.1 `int`

整数。

```scala
10
-3
0
```

### 3.2 `bool`

布尔值，只有 `true` 和 `false`。

```scala
true
false
```

### 3.3 `string`

字符串。

```scala
"hello"
"Aethe"
""
```

### 3.4 `nil`

空值。表示"没有值"。

```scala
nil
```

常见来源：`drop` 的返回值、找不到元素时的返回值、`input()` 遇到 EOF 时的返回值。

### 3.5 `array`

有序值序列。元素可以不同类型。

```scala
[1, 2, 3]
["a", "b", "c"]
[1, "hello", true, nil]
[]
```

### 3.6 `dict`

键值映射。键在运行时统一按字符串存储。

```scala
{name: "Alice", score: 95}
{"lang": "Aethe"}
{}
```

键可以写成裸标识符 `name` 或字符串字面量 `"name"`。推荐用裸标识符。

### 3.7 `object`

由 `type` 构造产生的实例值。包含类型名、字段表、方法。

```scala
User("Alice", 95)
```

### 3.8 `callable`

可调用值。来源包括：

- `fn` / `flow` 定义的函数名
- `stage` 定义的阶段名
- 匿名 `pipe(...) { ... }`
- `bind`、`chain`、`branch`、`guard` 生成的组合子
- 内建可调用对象（`range`、`str`、`int`、`bool`、`type_of`、`input`、`read_file`）
- 存有 callable 的变量

---

## 4. 真值规则

以下值为假：

| 值 | 假 |
|----|-----|
| `false` | 假 |
| `nil` | 假 |
| `0` | 假 |
| `""` | 假 |
| `[]` | 假 |
| `{}` | 假 |

其余值均为真。

真值规则影响 `when`、`while`、`choose`、逻辑运算 `&&` / `||`。

---

## 5. 变量与裸标识符 (Variables & Bare Identifiers)

Aethe 在语言底层最反传统、也是最独特的设计在于：**明确区分了“变量读取符号”与“裸标识符”**。这极大简化了在字典/对象/管道流中指定字段、属性时的字符串包围冗余，提供了接近 Lisp 中 Symbol（符号）或 Ruby 里的 `:symbol` 体验。

### 5.1 裸标识符 (Bare Identifiers / Symbols)

在 C++ 或绝大多数语言中，写下一个并没有加引号的单词（如 `name`）时，编译器会尝试将它解析为变量名查表。但在 Aethe 中，**裸标识符默认是一个符号式的字面量字符串值，而不是对变量的求值读取**。

```scala
name          // 符号值，在 AST 中求值得出字符串 "name"
score         // 符号值，等价于字符串 "score"
admin         // 符号值，等价于字符串 "admin"
```

这带来了极大的便利：当你调用基于属性与键查找的函数时，再也不必要填写累赘的引号。它使得领域特定语言（EDSL）般的表述成为了可能：

```scala
100 |> into(score);                         // 将 100 导入至变量名为 score 的坑位中
{name: "Alice"} |> get(name) |> emit;       // 查字典，name直接视作键 "name"
$users |> where(role, admin) |> emit;       // 甚至 admin 也能作为对比的值字串
$users |> evolve(name, upper) |> emit;      // 告知系统更新 "name" 下辖的数据域
{score: 95} |> rename(score, total) |> emit; // score 和 total 全是裸字串参数
```

### 5.2 强制显式状态读取 (Explicit Variable Evaluation)

为对冲上述设计，由于纯英文单词不再引发变量读取，那要如何“读取”一个之前存下的变量值呢？Aethe 与 Bash Shell/PHP 等语言借鉴了相同的语法范式——**凡欲获取变量的真实内在状态（Dereference the value），必须加前置符 `$`。**

```scala
let score = 100;
score |> emit;     // 打印出字符串 "score"
$score |> emit;    // 读取变量 score 中的实体值，打印出 100
```

这种双层视角的区分彻底消除了 C++ 当中“用局部变量掩盖全局变量”的隐患：在 Aethe 中，键名和变量绝不冲突。

### 5.3 变量推注写入与声明

你可以运用两套同构平行手段来改写或新立变量的归属权：
```scala
let score = 100;           // (1) 陈述式语法糖，声明并赋值
100 |> into(score);        // (2) 纯正管道数据流注水方式，本质与之完全等效
```

### 5.4 复合形左值赋值体系

Aethe 完全内置了形同 C/C++ 一般完备强大的连缀左值（L-Value）赋值架构：

```scala
$score = 120;             // 最单纯直接覆写
$score += 5;              // 原地加法反哺
$user.name = "Bob";       // 对象深层成员直接篡改
$rows[0].score += 9;      // 多维数组兼字典解引用深溯叠加
```
**关键差异连结**：
每一句对赋值表达式的运算完毕后，**它自身都会产出此番刚刚写透的最新值作为这一块表达式群落的返回值**——这意味着它能够被嵌套至更复杂的函数判别或运算之中连贯作业（例如 `when ($a = foo()) != nil { ... }`），彻底看齐了 C++ 对于赋值返回值设定的巧思。

---

## 6. 表达式计算单元 (Expressions)

Aethe 的表达式设计从简明主义进发，剔除了 C++ 繁琐的强制形变宏（Casting），采用更平滑的动态解析与安全护城河。

### 6.1 主元表达式体系 (Primary Expressions)

构成指令的最小原子的不可分割部件：
- **静态原语集**：纯数字 `42`，文本串 `"hello"`，真假开关 `true` / `false`，彻底消亡的虚无空洞 `nil`。
- **复合字面结构组装体**：数组字面量 `[1, 2, 3]` 以及结构图谱字典字面量 `{name: "Alice"}`。在解析时期即可直接构造大包体分配。
- **符号值与探知探头**：前述无引号之裸标识符 `name`、读值探针 `$name` 以及下节阐明的占位路由分发中心 `_`。
- **动态运行块生成**：匿名闭包制造工坊 `pipe(x) { return $x * 2; }` 及运算重封包裹 `(...)`。

### 6.2 强力调用算式 (Call Expressions)

```scala
callee(arg1, arg2, ...)
```

在 Aethe 中不存在对于函数体调用的重负——`callee` 位置几乎可以兼容吸纳任何内含 `callable` 的左式：
- 定义好的常任 `fn` 与 `stage`
- 挂在对象头上的对象方法 `$user.badge()`
- 直接指向存在内存变量内的函数体 `$double(21)`
- 犹如魔法般甚至支持即刻生成立即调用（也就是 JS 界常用的 IIFE 化身）： `pipe(x){return $x;}(21)`
- 以及一切内建名字词 `range(5)` 等。

### 6.3 闭包工坊 `pipe` 字面产生式

它是一种能挂接并在行文当中直接凭空吐出全新作用域 `callable` 函数体的表达式：
```scala
pipe(param1, param2, ...) {
    ...   // 内部仍靠 $param1 引读参数
}
```
**深度闭包捕获机制 (Value Capture Policy)**：当执行该句之时，它不是像全域函数般只知外界大视野；而是“冻结”此一时刻周边天地里能够读取的所有 `$var` 到该内部独立空间且是实施浅层数值化拷贝拦截。完全贴合 C++ 中 `[=]` 值捕获 Lambda 机制。

### 6.4 索引穿透层 (Index Access)

```scala
expr[index]
```
一种深层检索语法的快捷外包糖。实际后台对应执行的是通用容器获取接口 `get(index)` / `get(key)`！
- 对列装线性序列（`array`, `string`）：限定 `index` 为整型脚标（从 0 开计）：`[10, 20, 30][1]`
- 对键值配对表象（`dict`, `object`）：限定采用字串作为锁定挂钩，允许裸字词或是外部传入的字符串形式：`{name: "Alice"}[name]`

并允许疯狂的嵌套连溯机制直达无边层级：`$matrix[0][1]`。

### 6.5 结构体跨越访问 (Member Access / Dot Syntax)

```scala
expr.member
expr.method(...)
```
在面向对象的应用层特供通道语法。`dict` 与 `object` 全线畅通。
通过它去读取内部私有存储或是诱发深层的方法驱动：不仅能作为只读接口，**更是一等左值（L-value）——可通过 `$obj.field = value` 反向更改成员深处。**

### 6.6 算术逻辑单/双元算子操作表

**一元算子**：对变量自身直接动武
| 符号 | 原理表述 | C++ 对照域 |
|--------|------|------|
| `!expr` | 真理反转化。触发判断 Truthy 机制得出对立 `bool` | `!x` 逻辑非 |
| `-expr` | 向位负极化翻转（对数据要求极严苛只能是纯净 `int` ） | `-x` 数学负 |

**二元混合博弈算子**：
| 派系归属 | 参战算符旗号 | 特征界定与规约 |
|------|--------|------|
| 数理算术 | `+` `-` `*` `/` `%` | 主干严格执行纯化 `int` 的限制操作。若在加号 `+` 中有一侧为字串即触发了重载魔性行为：化为全盘类型推平 `string` 并执行超大字汇拼接挂合 |
| 并列天平和 | `==` `!=` `>` `>=` `<` `<=` | `==` 和 `!=` 广纳百川可做一切体素（包含对象阵列）的全深层次配对查重对比。大于等于一类的刻度校验则死锁只可容受 `int` 入场，一切越界将直接爆裂触发运行时警示 |
| 短路逻辑 | `&&` `||` | 短路断路器原滋原味：当 `&&` 见首个为假、抑或 `||` 见首测为真即刻终止截停并在该节点即时结束推算返回最后一次求得之定断成果 |

### 6.7 复合赋值阵点流

```scala
$name += expr
$record.field /= expr
```
对左值 `L-Value` 进行再分配与反攻并融合前一期留存数值。除 `/=` 与 `%=` 兼具对于零容忍机制（除 0 当场判处极刑封杀）外，表现方式全与 C/C++ 老派祖传同等。

### 6.8 严格界定之优先级分流轴 (Operator Precedence)

Aethe 对此排序极为敏感并严卡下列表格定调（高至低），用以保障没有括号护体时全域解读的绝对单向确立不生歧义：

1. 函数指令呼叫 / 括号解链 / 成员查访：`f()`、`a[i]`、`a.b`
2. 最顶配单一制裁：`!`、`-`
3. 高规格乘除模剥：`*`、`/`、`%`
4. 基底加减拼接：`+`、`-`
5. 划界尺度刻写：`>`、`>=`、`<`、`<=`
6. 双生重迭天秤：`==`、`!=`
7. 逻辑合流联查：`&&`
8. 逻辑横跨抉择：`||`
9. **总制高流水统划**: 大管道引流 `|>`
10. 最迟缓收网写入：全系制霸赋值 `=`、`+=`、`-=`、`*=`、`/=`、`%=`

---

## 7. 管道骨干语义流 (Pipeline Semantics)

管道 `|>` 并不只是一块提供左送右转的花瓶修饰，更是撑起 Aethe 全部大树生态和思想精髓的主动脉——由于它是该语言体系中的最特权阶级表达式层。这就和 Bash 中的 `|` 或 F# 及 Elixir 语言机制遥相呼应。

### 7.1 拓扑推演总模型

```scala
source |> step1 |> step2 |> step3;
```
每跨越一处 `|>` ，大总管会将左手侧整个表达式运算殆尽后的确切沉淀值化作主脉血注入右手的领地并再度重铸新态计算。这就规避了 C++ 里多重镶嵌调用的“恶心括号地狱”：`step3(step2(step1(source)))` 转变为了完全符合人类直觉从前至后梳理的工业化车间操作线。

### 7.2 缺省占位之自动首参隐插 (Automatic First-Argument Injection)

这套核心装配规程决定了即便无端无号，数据也不会走调流失：当你探定右侧操作盘中并未设置出哪怕半个人工开孔符 `_` 时，**左面来者的滚滚数据流会被强硬且全自动地安塞推置成右列阶段接收方参数队列的第绝对一顺位入参。**

```scala
21 |> double;            // 完美转换 → double(21)
10 |> add(20);           // 原参后退补位 → add(10, 20)
"hello" |> upper;        // 无缝适配 → upper("hello")
```
这种设计让日常大部分组合指令极度轻快简洁，连一丁点符号噪音都消散于无形。

### 7.3 定位引流靶眼 —— 占位符 `_` 

既然上文有强制插前机制，那么如果要将数据发送至第二个亦或更深处的特定接收腹地该如何操作？这时 **下划线路由占位符 `_`** 将正式主导：由于有了 `_` 充当信号接收器，先前的盲目加插第一首选排位规则即刻废止，而是转入精确追踪空投指引。

```scala
"Hello, Aethe!" |> substring(_, 7, 5);    // 精准插位第一 → substring("Hello, Aethe!", 7, 5)
// 倘若是想把管路输出送进其它次位也可以！
$obj |> set(score, _)                     // 智能规避首位将流分发至后续
```

### 7.4 多维度多发齐射引流 (Multi-Targeting Reuse)

若在一个独立执行阶段同时排布多个天线针 `_` ，这个源入点不会因分流而分裂衰竭。其实是等于做出了指向同一个常态值缓存区的多次深浅不一的**同构指引引用获取读法**，这就等比例缩减了声明额外冗长中转变量去中继倒置的时间消耗：

```scala
5 |> add(_, _);    // → 后台解包为 add(5, 5) ，产出返回 10
```

### 7.5 超能变种：裸表达式动态重铸目标 (Naked Expression Targets)

右边部分没有严格要求只有单一调兵号牌函数名接管——事实上只要是带有 `_` 引导存在的合法计算簇表达式，全额通吃包揽接下！这就是将一整个带有繁杂微操行为指令算作超级车间流转的一绝：

```scala
10 |> _ * 3 + 5 |> emit;    // 自动置换包裹，得返算 35 ，并且不脱离单线串联
```

### 7.6 极严格的 `_` 绝对语义隔离边界 (Strict Boundaries of Placemarkers)

这是这门语言的死亡底红线纪纲！
`_` 在 Aethe 中的全等底线定义只存在一种：“这有且仅当只代表当前跨越这一步 `|>` 流送入界那瞬间的具体定长不变静态输入源值代理引用”。

- 它**绝非**是一台可以随意定义出界匿名 Callable 函数的方法制造机。
- 它更加**不合法不能、无权脱离它依附的这独属一整座流水 `|>` 小舞台上存在**！

```scala
// 合规，皆在此道内发生且为管道内向内承接对象
10 |> _ * 3 + 5 |> emit;
"hello" |> substring(_, 0, 3) |> emit;

// 非法报错！由于脱离承载 `|>` 的庇护网流之外了
_ * 3;                     // 不会有神龙输送数据至此
let f = _ * 2;             // 这不能用来代替创造 callable
let g = map(_, add);       // 不行！这种外抛操作严重触礁停摆
```

遇到需要建立可反复呼号持久性利用的抽象组合工具组，务请左转至专属厂房建立起你的 `pipe(...) { ... }` 构建机制或者是依赖闭包特性的系列产品调用。

### 7.7 入网把关类型流变判据表 

**情形归纳推断总表览**：
- 当该执行右半段指令舱 **不包含任何** `_` 标位时。该指令只认同且唯一认同以下身份作为领航接收方：
  1. 提前布防确立了的 `stage` 乃至 `fn` 总名册头衔。
  2. 已经加持填装完全了带有固定偏转参数包裹状态之下的预挂载指令如 `stage(...)`。
  3. 解包提拔引流自身体质内部中夹带出的 callable 职级对象如 `$double` 。
  4. 当场现拉搭建挂帅起的匿名小作坊流水台如 `pipe(x) { ... }`。

- 但是！若有 **任意的一个** `_` 横空出现，则宣告解放这所有的固执与局限，在它的护航内此侧可以是这套架构下最包罗万象五花八门的复合混合数学运算或是无休连绵逻辑推理判定。

### 7.8 Railway Safe Pipeline `|?` (3.0)

Aethe 3.0 引入了一套全新的**铁轨式安全管道操作符 `|?`**。这是受到 Rust `Result<T,E>` / Elixir `{:ok, val} | {:error, reason}` / F# Railway-Oriented Programming 启发而诞生的决定性新特性。

#### 7.8.1 Ok / Err 包裹类型

Aethe 新增了两种原生内建值包裹类型：

| 类型 | 构造 | 语义 |
|---|---|---|
| `ok` | `Ok(value)` | 表示一次成功计算的结果载荷 |
| `err` | `Err(message)` | 表示一次失败/异常的错误信封 |

```scala
let good = Ok(42);       // type_of → "ok", toString → "Ok(42)"
let bad  = Err("oops");  // type_of → "err", toString → "Err(oops)"
```

#### 7.8.2 `|?` 操作符求值规则

`|?` 与 `|>` 可以在同一条管道链中自由混用。其核心求值契约如下：

```
source |? stage1 |? stage2 |? stage3 |> final_consumer;
```

**每当跨越一处 `|?` 时**：

1. **Err 拦截短路**：如果左侧传入的值是 `Err(msg)`，则**直接跳过本阶段**，将 `Err` 原封不动地向下传递。
2. **Ok 自动解箱**：如果左侧传入的值是 `Ok(v)`，则**自动提取内部载荷 `v`** 作为本阶段的输入。
3. **裸值直通**：如果左侧传入的既不是 `Ok` 也不是 `Err`（例如首个 `|?` 的源头），则直接作为输入传递。
4. **成功自动封箱**：如果阶段执行成功且返回值不是 `Ok`/`Err`，则自动封装为 `Ok(result)`。
5. **异常自动捕获 (Try-Catch)**：如果阶段执行中抛出了运行时异常（如除零、类型错误等），异常消息会被**自动捕获并封装为 `Err(message)`**，而非击穿整条管道。

```scala
// 完整示例：安全的数据处理管道
"42"
    |? pipe(x) { return int($x); }           // Ok(42)
    |? pipe(x) { return $x * 2; }            // Ok(84)
    |? pipe(x) { return $x / 0; }            // Err("Runtime Error: division by zero")
    |? pipe(x) { return $x + 999; }          // SKIPPED (input is Err)
    |> emit;
// Output: Err(Runtime Error: division by zero)
```

#### 7.8.3 Match 解构 Ok / Err

`match` 语句原生支持对 `Ok(binding)` 和 `Err(binding)` 进行模式匹配与解构绑定：

```scala
let result = 100 |? pipe(x) { return $x / 2; };

match $result {
    case Ok(value) {
        // $value 被绑定为解包后的内部载荷 (50)
        "Success: " + str($value) |> emit;
    }
    case Err(msg) {
        // $msg 被绑定为错误消息字符串
        "Failure: " + str($msg) |> emit;
    }
    else {
        "Unexpected" |> emit;
    }
}
```

#### 7.8.4 辅助内建函数

| 函数 | 参数 | 返回值 | 说明 |
|---|---|---|---|
| `Ok(val)` | 任意值 | `ok` 包裹 | 手动构造成功包裹 |
| `Err(val)` | 任意值 | `err` 包裹 | 手动构造错误包裹 |
| `is_ok(val)` | 任意值 | `bool` | 当 val 为 `Ok` 包裹时返回 `true` |
| `is_err(val)` | 任意值 | `bool` | 当 val 为 `Err` 包裹时返回 `true` |
| `unwrap(val)` | `Ok`/`Err` 包裹 | 内部载荷 | 强制拆箱，提取内部值。非包裹值直接返回自身 |

#### 7.8.5 C++ 实现层对照

在 C++ 底层概念比对中：

- `Ok(v)` / `Err(v)` 类似于 `std::variant<OkTag, ErrTag>` 持有一个 `shared_ptr<Value>` 载荷。
- `|?` 管道的 try-catch 拦截机制等价于在每个阶段外套一层 `try { ... } catch (const std::runtime_error& e) { value = Err(e.what()); }`。
- `match case Ok(x)` 的解构匹配类似于 `std::visit` + `if (std::holds_alternative<OkTag>(v))` 的组合语义。

---


## 8. 语句

### 8.1 表达式语句 (Expression Statements)

```scala
expr;
```

最常见的形式就是管道语句结束。在 Aethe 中，单纯计算值而不用变量接收也是合法的，这与 C++ 的表达式计算语句（如调用 `foo();`）完全一致。注意，裸值必须加分号才能做表达式语句。

```scala
"hello" |> upper |> emit;  // 完整的语句
42;                        // 合法语义，但在解释器中该值求算后抛弃
```

### 8.2 变量声明 `let`

```scala
let name = expr;
```

分配并在当前最近内圈闭作用域（Block Scope）中挂载一个新的命名变量，并作初始化求值赋值。
**工作机制**：
- 它类似于 C++ 的 `auto` 关键字定型，基于值语义与右侧绑定建立。
- 语义在底层上严格等价于 `expr |> into(name);` 形式的操作。这体现了语言“一切皆流转赋予”的设计哲学。

### 8.3 赋值语句

```scala
$name = expr;
$name += expr;
$object.field = expr;
$items[index] = expr;
```

用于覆盖已声明变量的值或对复合容器项进行深操作。
左值解析支持链式解引用（类似 C++ 的成员解引用与指针运算合并 `$obj.score[0] = ...`）。对嵌套目标的赋值在运行时会由解释器自顶向下的深溯并返回向上的写穿透行为修改内存结构体，例如 `$rows[0].score += 9;` 会在原字典基础上施加更新覆盖，而非产生丢失联结的临时副产物。

### 8.4 条件语句 `when`

```scala
when condition {
    ...
} else {
    ...
}
```

控制流抉择支，直接对应于 C++ 的 `if-else`。
- `condition` 无需用圆括号包裹，但其大括号 `{}` 是**强制硬性要求**，杜绝了由于省写大括号产生的经典的悬空 else (`dangling else`) 问题。
- `condition` 表达式接受任何类型，在求值前触发 [真值规则（Truthy/Falsy）](#4-真值规则) 动态校验，免除 C++ 里对不同指针是否必须显式与 `nullptr` 比较的啰嗦限定。

### 8.5 模式匹配 `match`

```scala
match expr {
    case value { ... }
    case _ when condition { ... }
    case _ { ... }
    else { ... }
}
```

高度加强版的 `switch-case`，自带模式验证与数据流绑定提取功能：
- **目标缓存**：首部的 `expr` 参数仅在入口发生**且只发生唯一一次求值**（借以防御副作用重复爆炸）。
- **`case value`**：采用深度判断 `==` 核验两者等同。由于 Aethe 动态性，其允许各种不同类别混合比较。
- **守卫条件（Guard Clause）**：`case pattern when condition` 的形式在 C++17 的 `if constexpr` 或者 C# 语法栈上极为吃香，在此模式一旦达成第一步配对后即刻执行二次关卡 `when`。
- **隐含绑定 `$it`**：每一个成功的条件域在入口第一步被强制将源头 `expr` 最新化身寄存进该 block 层内的系统变量 `$it` 以便使用，且在出块后析构，无变量泄露之风险。
- **下漏阻断**：匹配无 C++ 老旧的 `fall-through` 弊病，一旦命中一项在处理并跳出大结构后便全阶段终止，不需要 `break`。

### 8.6 循环结构 `while`

```scala
while condition {
    ...
}
```

基于前置状态检验的古典控制回路，与 C++ 的 `while` 语义分毫不差。支持 `condition` 在每次环回跳跃时原点重新求算真理映射。

### 8.7 迭代器循环 `for-in`

Aethe 放弃了类似于 C++ 远故的 `for(int i=0; i<N; i++)` 造次方法，全面转型类似 C++11 基于范围的循环 `Range-based for loop`。

单变量解包：
```scala
for item in expr { ... }
```

双变量协同解包：
```scala
for index, item in expr { ... }
```

- **数据源求值机制**：在此命令启动瞬时，`expr` 一样遭到封锁性唯一一次初态捕获，这意味着试图修改被迭代队列（比如循环中删除元素）绝不造成索引失联等恐怖内存问题。
- **动态行为推算表**：
  | 输入类型 | 单变量 `item` 引力行为 | 双变量 `index, item` 映射行为 |
  |---------|-----------|-----------|
  | `array` | 对源值作独立只读拷贝遍历 | 迭代器索引 + `item` 值 |
  | `string` | 拆作各个字符 `string` 遍历 | 字节层步进索引 + `char` 字串 |
  | `dict` / `object` | 将自身以复合体结构 `{key: ..., value: ...}` 释放 | 解构成 `key` （字串）+ 字典原属值 |

### 8.8 循环截断 `break` 与 接续 `continue`

执行原理与指令定序皆与 C++ 对齐：

```scala
break;
continue;
```
- `break` 具备最末层环剥逃逸能力，切断就近包裹着它的封闭 `for` / `while` 环域回路跳出；在非循环领域误用强制拉闸抛出运行时阻断级错误。
- `continue` 放弃现域残余行动，令程序指针骤降至下个新圈跳跃起步校验期。

### 8.9 作用域清理 `defer`

```scala
defer {
    // 关闭资源、回写状态等安全清理动作
}
```
等同于极其灵巧化的 **C++ RAII（Resource Acquisition Is Initialization）模型**构建或者 Go 语言的同名特设：
- 会使得指定在 `defer` 代码块内地方法自动被封存并在系统控制流跳将脱离“**直系最近包围环境词法作用域大括号界层**”（不仅仅是函数，甚至囊括 `when` 的局部块）这千钧一发之际立即后备执行！
- 一域内多重声明 `defer` 则强制以先进后出排队制（LIFO 栈式堆叠）原则销毁倒出。

### 8.10 数据返回阀门 `return` / `give`

```scala
return expr;
return;
expr |> give;
```
终极数据弹射窗，向召唤指令者逆向返回并带脱当前全运行流（函数、管道流或匿名调用）。
- 同等映射 C++ 中函数的剥蚀回调功能，且针对 `give` 专门定做：其直接作为一端吸收阶段挂在管道后方完美衔接受理从而免除了打破平滑连贯管链布局写回命令流的情况。
- 缺失对象将自动填充为虚无体 `nil` 返回。只能依托 `fn`/`flow`/`stage`/`pipe` 或是方法封控作用域内启动应用。

---

## 9. 可调用对象 (Callable Objects)

Aethe 实现了一个基于一级对象 (First-Class Citizen) 挂载策略的广义函数体总成系统：一切代码体动作均是被封装包装成纯数据类型的 `callable` ，可以放入变量流窜、用在数组与参数交互——这点上等同于 C++11 的 `std::function` 模型。

### 9.1 普通函数 `fn`

定义常规格局和传统对仗思维模式的标准型函数：

```scala
fn name(param1, param2, ...) {
    return $param1 + $param2;
}
```
**特征**：
- 它对于其接受列表 `paramX` 不提供任何偏心侧向对待机制。适合普通泛式的辅助工具处理搭建。

### 9.2 流式函数 `flow`

```scala
flow name(param1, param2, ...) { ... }
```
**特征**：
功能本质等态为 `fn`，作为它的互演化化名。唯一的不同即语义标识侧重点为“专门操纵多源组合流的数据调度节点”。

### 9.3 连贯阶段函数 `stream`（兼容 `stage`）

为迎合 Aethe 最极致灵魂机制“流水接力管道”的深度嵌合产物：

```scala
stream shout(suffix) {
    return $it |> upper |> concat($suffix);
}
"hello" |> shout("!!!") |> emit;    // HELLO!!!
```
**特征**：
- **暗桩绑定机制**：凡经 `stream`（或旧写法 `stage`）宣告构架出的 callable 身骨，不问你提供任何普通参数，都会自动接收一个**不在入参名表中暴露、但在体内无处不在的主系统游走变量引用 `$it`**！
- `$it` 稳固代指：一旦它挂接入管道，上游产出必定原汁原味地填装在 `$it` 中以供加工接手，省去了显位接引参数书写的丑陋步骤。

### 9.4 匿名捕获对象 `pipe`

```scala
let closure = pipe(x) { return $x * $outside_var; };
```
一如 C++ 当中所盛行的匿名 Lambda 表达式：
- 属于生成时表达式计算并非全局定义语句。由于是生成表达式，所以此等闭包必须在构建时进行周边环境关联（将能窥视的全部现存自由变量进行数值拷贝 `By Value Capture`）。在此后调取即使外界原始参照覆灭修改，闭包深埋固化的状态一如既往不会波动分毫。

### 9.5 组合子构物 (Combinators)

`bind`、`chain`、`branch`、`guard` 本质全数返回类型为 `callable`。但并不是手造代码墙，而是对上述四式 callable 用宏观积木层级进行了再次拼接捆绑（犹如 C++ 中的 `std::bind` 等）。详情请阅 [12.2 组合子体系](#122-组合子)。

### 9.6 全向选择参照架构图

为了明确这套错综相接系统的实际使用边界位，请参考此表格设计规范：

| 实施场景 | 运用武器推荐 | 为什么？ |
|------|------|------|
| 普通离散计算，各参项地位平等无高低贵贱 | `fn` | 标准函数体量清晰没有隐藏动作 |
| 置入长串管廊内，作为对上流冲击承接中枢 | `stream`（兼容 `stage`） | 直接内置解包利用 `$it` 流力节省代码笔墨 |
| 用于提供微小临时的判定、单项折损回调传替 | `pipe` | 现写并能深邃记录环境快照参数值不脱落 |
| 为原版缺失部件的函数前向植入后续固定位参 | `bind` | 函数柯里化与状态延时的正统解法 |
| 把零散步骤统化整合作为超级单线管道 | `chain` | 无需声明变量反复传导即可打造大型接龙闭环 |
| 并发开进横向铺开各路验证处理流，取统一并集 | `branch` | 最快速构造多元扇出分流大集合的超强工具 |
| 给管廊增加拦截水闸与安全护栏 | `guard` | 高优雅替代生硬冗长 when 结构的防守后门 |

---

## 10. 对象系统 (Object System)

Aethe 所依托的对象模型是一个极简但又极具生命力的原型系统概念结构。它规避了 C++ 庞大的类层级继承（Inheritance）与虚函数多态（Polymorphism），完全转交由运行时内存数据打包组合完成面向数据本身聚合的机制（相当于拥有附属运作方法的纯 `Struct` ）。

### 10.1 构造型态声明 `type`

```scala
type TypeName(field1, field2, ...) {
    fn method1(...) { ... }
    flow method2(...) { ... }
}
```

- **类名同源构造**：结构宣告圆括号上盘附的各项参数名 `field1, field2` 是在系统运行时自动作为对应该对象实例化实体上的私有财产公有字段名被确立的（且不提供也不必须提前作类型设标）。
- **完全解耦封装**：所有在类型作用块 `{}` 大图景中诞生的块级操作实体只能作为它的函数和方法挂载工具存在，允许利用常规 `fn` 甚至 `flow` 打包，**不支持也不允许体内额外再次独立宣告属性项。**
- **自指回溯**：处于一切该结构专属操作方法的深层运行体之中，都会默认悬挂上这一个隐性对象关联锚定词—— `$self`。类似于 C++ 的 `this` 指针但是免却其指向推导。借它可畅通无阻地寻访宿主躯干里的挂名数据域界抑或相邻方法流。

### 10.2 内存实例化调用 (Instantiation)

```scala
let active_user = User("Alice", 95);
```
没有诸如 `new` 这等内存直接强暴接管语素。因为通过 `type` 所组装的词条在 Aethe 中既是类型符号又是天然的出厂组装函数。传值进去便能启动类似于工厂模式般的直接量产运作。而所有的这些皆依附底端的浅层引用（类似于 `std::shared_ptr` 管理的生命历程）生存。

### 10.3 读存字段及调用通信 (Member Operations)

```scala
$active_user.name = "Bob";               // 写状态
$active_user.name |> emit;               // 读属性：此时为 Bob
$active_user.badge() |> print;           // 对等发送并引诱计算方法的计算指令启动
```
- **方法安全防线**：Aethe 中的此系统对于方法享有独家调用豁免（也就是无法直接作为 `$obj.method` 这种不附加圆括号被抽查，只可执行计算动作）。
- **对象向字典劣化**：对象实体可以退化。当你向针对泛容器加工的字典操作管道指令体系施展给到该 `object` 上时（如使用 `pick`, `omit`, `rename` 等指令时），它将平铺展开解印并化回最源初的单纯 `dict` 数据模型处理并丢失一切内部原绑附函数体。这提供了一种高阶数据清洗和快照保存机制！

### 10.4 联合运用蓝本

```scala
type User(name, score) {
    fn badge() {
        // 利用 $self 向内置属性挂钩推演结果
        when $self.score >= 90 {
            return "A";
        } else {
            return "B";
        }
    }
    
    // 方法内部自由调度相互协作以及变量捕获覆盖
    fn promote(bonus) {
        $self.score += $bonus;
    }
}

// 快速铺陈出线组
let user = User("Alice", 85);
$user.badge() |> emit;    // B
$user.promote(15);        // 激活升权处理，将修改状态贯穿生命周期
$user.badge() |> emit;    // A
```

---

## 11. 运行模型

### 11.1 终端 IDE（默认）

```bash
./aethe
```

进入全屏编辑器。`Ctrl-R` 运行当前代码，`Ctrl-S` 保存，`Ctrl-O` 打开文件，`Ctrl-Q` 退出。

### 11.2 REPL

```bash
./aethe --repl
```

缓冲执行模型：

- 输入的代码先进入缓冲区，不会立刻执行
- 输入 `run` 后统一解析与执行
- 执行完成后清空缓冲区
- 已定义的函数、stage、类型在解释器实例中保留

提示符：

| 提示符 | 含义 |
|--------|------|
| `>>>` | 缓冲区为空 |
| `...>` | 缓冲区非空，继续输入 |

退出：`exit`、`quit`、`Ctrl+D`。

### 11.3 脚本模式

```bash
./aethe --run example.ae
```

单次执行，不进入交互界面。

### 11.4 标准输入

程序执行期间调用 `input()` 或 `input(prompt)` 会从标准输入读取一行。EOF 时返回 `nil`。

---

## 12. 内建能力

### 12.0 约定

本节的写法约定：

- `input |> name(...)` 表示管道阶段形式
- `name(...)` 表示普通调用形式
- `callable` 参数统一支持三类值：stage 名（如 `upper`）、普通 callable 名（如 `type_of`）、`pipe` / 组合子生成的值（如 `bind(add, 10)`）
- 类型不匹配或参数数量不匹配时会触发运行时错误

---

### 12.1 通用可调用对象

以普通调用形式使用，不是管道阶段。

#### `range`

```scala
range(end)              // lazy stream [0, end)
range(start, end)       // lazy stream [start, end)
range(start, nil)       // lazy infinite stream
range(start, nil, step) // lazy infinite stream with step
```

**参数**：
- `start` (`int`，可选)：区间的起始值，默认为 0，包含在区间内。
- `end` (`int`，必填)：区间的结束值，不包含在区间内。

**返回机制**：
生成一个惰性 `stream`。只有在终结阶段（如 `take`、`head`、`emit` 等）真正拉取时才会逐项计算，避免大范围区间提前分配内存。`range(start, nil)` / `range(start, nil, step)` 可表示无限流。

```scala
range(5) |> emit;       // [0, 1, 2, 3, 4]
range(2, 5) |> emit;    // [2, 3, 4]
```

#### `str`

```scala
str(x)
```

**参数**：
- `x`（任意类型，必填）：待转换的任意类型值。

**返回机制**：
将给定参数强制转换为其对应的标准字符串表示形式，返回类型为纯文本面貌的 `string`。其常用于动态生成打出内容或字符串序列化输出保障。

```scala
str(123) |> emit;     // 123
str(true) |> emit;    // true
```

#### `int`

```scala
int(x)
```

**参数**：
- `x`（`bool`、`string` 或 `int`，必填）：待接受验证转换的目标本体。

**返回机制**：
对目标值实施同态算力转整型运算，必定反馈给系统纯化血统的 `int` 格式：对布尔值 `bool` 则分别打作 0 （对于 `false`）或是 1 （指代 `true`），若针对富含数字面貌的 `string` 会主动开启内置解析脱除字串光环并还原数值本身。如遇彻底不可解析读数时（比如纯英文词条引发转数字）发生极度反噬强制阻断反馈运行时错误。

```scala
int("42") |> emit;    // 42
int(true) |> emit;    // 1
```

#### `bool`

```scala
bool(x)
```

**参数**：
- `x`（任意类型，必填）：亟须接受甄别校验和质变的任意目标。

**返回机制**：
依托真理推断规则模型全盘验正：强行剥离它一切原本携带的伪装数据特性，仅仅探视保存极简主义色彩里的终极状态走向：只交手 `bool` 实体——即抛送赤裸的 `true` 或者 `false`。

```scala
bool("") |> emit;     // false
bool(1) |> emit;      // true
```

#### `type_of`

```scala
type_of(x)
```

**参数**：
- `x`（任意类型，必填）：待查水表深究运行时真身的客体。

**返回机制**：
对底层溯源操作。将包含此目标下层源头所依托构型类的核心 C++ 运行时结构名称映射到语言级字眼中输出（为文本字符串）：定案之字词必定出自此处穷尽罗列的合法小标词 —— `"int"`、`"bool"`、`"string"`、`"nil"`、`"array"`、`"dict"`、`"object"`、`"callable"` 以正名。

也可以作为最基本的探针型部件接入流式数据管道验证：

```scala
"hello" |> type_of |> emit;    // string
```

#### `input`

```scala
input()
input(prompt)
```

**参数**：
- `prompt` (`string`，可选)：充当开启会话或要求作答前向受众提示语的静态字串。

**返回机制**：
霸主级独裁管辖截留接口。阻塞当下流水线的自然执行状态，立刻横扫激活系统后台并执行抢抓用户对于命令台控制终端所做的一行敲击回应行动。在捕捞收网阶段摘掉多余换行符且包裹为 `string` 主串重投怀抱；倘若遭遇断水断粮绝户袭击（按键遭遇 EOF 信手中断拒绝交互）只能退避并返回极空无物状态元灵之 `nil` 兜底保驾。

```scala
let name = input("name> ");
```

#### `read_file`

```scala
read_file(path)
```

**参数**：
- `path` (`string`，必填)：基于本地主轴的特定目标文件访问的相对或绝对路径名称。

**返回机制**：
作为高空突防特权接口向着宿主系统盘执行直升机空降读取动作。秉持文本模式横暴拉取其目标文件内的海量全尺寸文字库，统一合并送上为一段史诗规模极其修长的纯文本 `string` 卷轴。（假如档案不在服务区迷雾隐藏、或是本身存在读取拒绝壁垒限制，无能为力时爆发极重创型运行时错误报错让系统强退出戏）。

```scala
read_file("data.txt") |> emit;
```

---

### 12.2 组合子

这四个内置特殊对象用于将其他可调用对象以更高的抽象层面组合封装，既可以普通调用返回新的 callable，也可以直接在管道阶段进行即时归约应用（Higher-Order functions）。

#### `bind`

```scala
bind(callable, arg1, arg2, ...)       // 普通调用返回新 callable
input |> bind(callable, arg1, ...);   // 即时管道应用
```

**参数**：
- `callable` (`callable`，必填)：原始的目标函数、阶段或 pipe，要求接受 $1 \dots N$ 个参数。
- `arg1, arg2, ...`（任意类型，可选）：预先绑定的后续静态参数。

**返回机制**：
生成一个“单输入”的新 Callable 闭包。当该新 callable 收到主输入 `$it` 时，会自动调用 `callable($it, arg1, arg2, ...)` 并返回结果。

```scala
[1, 2, 3] |> map(bind(add, 10)) |> emit;              // [11, 12, 13]
[{name: "Alice"}, {name: "Bob"}] |> map(bind(get, name)) |> emit;  // [Alice, Bob]
```

#### `chain`

```scala
chain(callable1, callable2, ...)       // 返回新 callable
input |> chain(callable1, ...);        // 即时应用
```

**参数**：
- `callable1, callable2, ...` (`callable`，必填)：依次连续执行的处理阶段，按从左至右组合。

**返回机制**：
生成包含一系列线型串联步骤的新 Callable。管道输入先进入第一个阶段，再把结果透传给下一阶段直到结束，返回串联逻辑链末尾计算的值。

```scala
let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;    // HELLO!
```

#### `branch`

```scala
branch(callable1, callable2, ...)      // 返回新 callable
input |> branch(callable1, ...);       // 即时应用
```

**参数**：
- `callable1, callable2, ...` (`callable`，必填)：互不干涉地各自独立处理共同输入的平行阶段。

**返回机制**：
生成一个单输入平行分流聚合的新 Callable。同一个入口值 `$it` 会被等额发送给每个独立回调对象求值，并将各步的产出按传递顺序拼接为一个 `array` 并返回。

```scala
"Aethe" |> branch(type_of, size, upper) |> emit;    // [string, 5, AETHE]
```

#### `guard`

```scala
guard(test, on_true)
guard(test, on_true, on_false)
input |> guard(test, on_true, on_false);
```

**参数**：
- `test` (`callable`，必填)：判别测试函数。
- `on_true` (`callable`，必填)：若判别通过被激活。
- `on_false` (`callable`，可选)：若判别失败被激活。

**返回机制**：
生成安全流转机制的新 Callable。该主输入送入 `test` 进行真值判断。结果为真则自动调用 `on_true($it)` 并返回；为假则若有 `on_false` 调用 `on_false($it)` 并返回，未提供时则**原封不动返回初始值**（类似于贯穿过滤器）。

```scala
5 |> guard(pipe(x) { return $x > 3; }, bind(add, 100), bind(sub, 100)) |> emit;
// 105
```

---

### 12.3 输出与状态阶段

#### `emit` / `print` / `show`

```scala
input |> emit;
input |> print;
input |> show;
```

**参数**：
- 无显式参数。仅接受管道传递的主输入。

**返回机制**：
将主输入的值转换为字符串表示并向标准输出打印，末尾自带换行符。执行完毕后**原封不动返回原值**。三者完全等价。

#### `into` / `store`

```scala
input |> into(name);
input |> store(name);
```

**参数**：
- `name`（`identifier`，必填）：当前词法作用域中的目标变量裸标识符（如 `score`）。

**返回机制**：
将主输入赋值到对应名称的变量之中。若变量不存在则初始化。最后**原封不动返回原主输入值**。

#### `drop`

```scala
input |> drop;
```

**参数**：
- 无显式参数。

**返回机制**：
主动舍弃当前管道的主输入。返回空值 `nil`。

#### `give`

```scala
input |> give;
```

**参数**：
- 无显式参数。

**返回机制**：
管道视角的退出出口。从当前包裹的 `fn`、`flow`、`stage` 或对象方法作用域中立即返回该主输入。与 `return input;` 同样效力且不再流向下一个管道步骤。要求环境支持提前返回。

---

### 12.4 数值与逻辑阶段

这些管道阶段主要处理整数算术与比较逻辑。

#### `add` / `sub` / `mul`

```scala
input |> add(x);    // input + x
input |> sub(x);    // input - x
input |> mul(x);    // input * x
```

**参数**：
- `input` (`int`，必填)：基于管道传入的左操作数。
- `x` (`int`，必填)：右操作数。

**返回机制**：
分别执行加法、减法或乘法操作，返回计算出的新 `int`。非整数参数触发运行时错误。

#### `div` / `mod`

```scala
input |> div(x);    // input / x
input |> mod(x);    // input % x
```

**参数**：
- `x` (`int`，必填)：除数 或 模数。

**返回机制**：
执行向零取整的整数除法（`/`）或取模（`%`），返回结果 `int`。
**错误机制**：若 `x == 0`，则触发除零异常并中止运行。

#### `min` / `max`

```scala
input |> min(x);    // 两者中较小值
input |> max(x);    // 两者中较大值
```

**参数**：
- `x` (`int`，必填)：作对比的另一个整数。

**返回机制**：
严格取出两者中较小或较大的那个整数，返回 `int`。

#### `eq` / `ne` / `gt` / `gte` / `lt` / `lte`

```scala
input |> eq(x);     // input == x
input |> ne(x);     // input != x
input |> gt(x);     // input > x
input |> gte(x);    // input >= x
input |> lt(x);     // input < x
input |> lte(x);    // input <= x
```

**参数**：
- `x`（任意类型用于判等，`int`用于比较）：比对目标值。

**返回机制**：
- `eq`/`ne` 基于运行时类型的深度比较返回 `bool`（可用于数组/对象内部深比较）。
- 大小比较关系（大于、小于等）仅限支持 `int`。若参数具有非 `int` 将导致报错。最终一并返回真理判定 `bool`。

#### `not`

```scala
input |> not;       // !truthy(input)
```

**参数**：
- `input`（任意类型）：源判定对象。

**返回机制**：
按真值规则作一次翻转。若判断为 Truthy 返回 `false`；Falsy 返回 `true`。

#### `default`

```scala
input |> default(x);
```

**参数**：
- `x`（任意类型，必填）：备用的默认值。

**返回机制**：
若 `input` 严格为 `nil`，则返回备用参数 `x`；若其不为 `nil`，则返回原输入本身。（常用于过滤空字段）

#### `choose`

```scala
input |> choose(a, b);
```

**参数**：
- `a`（任意类型，必填）：判真时选择项目一。
- `b`（任意类型，必填）：判假时选择项目二。

**返回机制**：
三元算子的管道版本。若主输入求值为 Truthy，返回 `a`；否则返回 `b`。

---

### 12.5 字符串阶段

这些阶段严格约束主调用目标为原始纯粹的 `string` 字符排布序列。

#### `trim`

```scala
input |> trim;
```

**参数**：
- `input` (`string`，必填)：基于前序传来的字符串。

**返回机制**：
剔除首尾的所有留白字符层叠，生成新的 `string` 并返回（字串中央部位缝隙内的空白完整留存）。

#### `upper` / `to_upper` 与 `lower` / `to_lower`

```scala
input |> upper;
input |> lower;
```

**参数**：
- 无显式参数。

**返回机制**：
对目标字串内容分别彻底施行转换全大写与全小写字体的指令，返回处理后新的 `string`。互为别名等效。

#### `concat`

```scala
input |> concat(arg1, arg2, ...);
```

**参数**：
- `arg1, arg2, ...`（任意类型，可选）：额外追加的后缀项。

**返回机制**：
将所有的跟随参数经内部的隐含 `str()` 类型强制转换为文字化身，在宿主串体的后面无缝融合焊接。返回融合完备的修长 `string`。

```scala
"hello" |> concat(" ", "world") |> emit;    // hello world
```

#### `substring`

```scala
input |> substring(start, length);
```

**参数**：
- `start` (`int`，必填)：截取开始处（索引从 0 起）。
- `length` (`int`，必填)：预定保留的长度度量（必须非负）。

**返回机制**：
自 `start` 为原点开辟切片，撷取长至 `length` 个限度内的序列子串返回。
**容错边界**：`start < 0` 或 `length < 0`，抑或 `start` 越过整串尾侧门槛，一律安全返回空字串 `""`；若剩余跨越距离不够满血达成指定限制，自动在边缘终止获取。

#### `split` 与 `join`

```scala
input |> split(sep);     // string -> array
input |> join(sep);      // array -> string
```

**参数**：
- `sep` (`string`，必填)：切分断口或者合并骨架。

**返回机制**：
- `split` 专为切割字符串：基于所有 `sep` 出现的物理节点实施硬拆分斩断，获取返回子字符串的有序 `array` 队列。
- `join` 为特殊拼接：专门作用于存有文本碎片子元素的 `array` 上，以 `sep` 作为各组件接缝灌注物将其统一压进巨大的连贯 `string`。

```scala
"a,b,c" |> split(",") |> join(" - ") |> emit;    // a - b - c
```

#### `starts_with` 与 `ends_with`

```scala
input |> starts_with(prefix);
input |> ends_with(suffix);
```

**参数**：
- `prefix` / `suffix` (`string`，必填)：探针性质的边缘样本字符串。

**返回机制**：
向输入串侧翼分别发送扫描确认判定。检测原文本左段前缀乃至右段后缀地带，恰合相衬即返回 `true`，不然一律置为严防 `false`。

#### `replace`

```scala
input |> replace(from, to);
```

**参数**：
- `from` (`string`，必填)：需查杀搜捕的原罪标的字符串。
- `to` (`string`，必填)：所替补扶正的新目标字符串。

**返回机制**：
全局范围内查找非互相覆盖重叠穿插区位下的所有 `from` 子结构身段，并强横且彻底的改写覆盖成了 `to` 组件，获取其成品形态并反馈该大 `string`。
**错误机制**：极度禁止使用空串执行 `from` 作为检索靶心，此举由于缺乏坐标锚定而定性为非法异常导致主程序的挂起错乱。

---

### 12.6 复合值阶段

这些阶段用于操作数组、字典及对象这类承载容量或键值映射的复杂体系，亦能在局部针对 `string` 序列奏效。

#### `size` / `count`

```scala
input |> size;
input |> count;
```

**参数**：
- `input` (`string`, `array`, `dict`, `object`，必填)：基于前置传入的复合或序列载体。

**返回机制**：
- `string`: 获得严格字符数量；
- `array`: 探明内部存放的所有独立元素的个数；
- `dict` / `object`: 精密测量顶层键值对组合规模或字段数；
两者别名等效，统一反馈计量 `int`。

#### `get` / `field` / `at`

```scala
input |> get(key);
input |> field(key);
input |> at(index);
```

**参数**：
- `key` 或 `index` (`int` 或 `string`，必填)：提取位置指引器。强烈推荐对对象等采用裸标识符如 `get(name)`。

**返回机制**：
实现高频率下钻探测。
- 若 `input` 为 `array` / `string`，凭依整数下标提取该方位内的值；越界触发抛错。
- 若为 `dict` / `object`，凭借该字符串作为键或字段索引直接抽离里侧包裹的值，不存在时安全返回 `nil`。

#### `set`

```scala
input |> set(key_or_index, value);
```

**参数**：
- `key_or_index`（`int` 或 `string`，必填）：指定更新或扩张所锚定的坐标方位与条目名称。
- `value`（任意类型，必填）：欲行埋入的具体实体值。

**返回机制**：
覆盖性质赋值写入策略：
- 对线性 `array` / `string`，替换指定下标旧数据空间并重组；
- 对键值图谱 `dict` / `object`，暴力覆盖或新拓展相应条目；
所有行为皆产出新副档并返回**更新后的容器实体**自身。

#### `update`

```scala
input |> update(key_or_index, callable);
```

**参数**：
- `key_or_index`（`int` 或 `string`，必填）：需要刷新升级的地貌坐标点位或字典键。
- `callable` (`callable`，必填)：针对该点位旧数据实行演变逻辑的小型闭包（如 `bind(add, 5)`）。

**返回机制**：
动态自我进化机制：预读出所指位置目前的藏身旧值，将其投喂入 `callable` 中求得结果后，立刻回写封存原位，并向后传出**已变更完毕的容器全貌**。若该键尚未确立，初始值界定为 `nil` 给其计算。

#### `insert`

```scala
input |> insert(index, value);
```

**参数**：
- `index` (`int`，必填)：横加阻隔或强力插入的整数索引卡位点。
- `value`（任意类型，必填）：欲在此引爆并定居的新生数据。

**返回机制**：
限定 `array` 与 `string`。由索引所指点位强行撕裂一处空地，将目标埋入，且后续元素顺延右移一格。若越界报错。退回**成型后的新容器或字符串**。

#### `remove`

```scala
input |> remove(index_or_key);
```

**参数**：
- `index_or_key`（`int` 或 `string`，必填）：标记删除行刑位置的黑名单键。

**返回机制**：
执行除籍抹灭与内存缩编操作。
- `array` / `string`：抽除该下标注定受难的元素并缝合伤口，溢出即报错；
- `dict` / `object`：剥离并清洗涉事键对关联项，如果查无此人相安无事维持原判；
均交出**脱落完毕清减后的宿主对象**。

#### `contains` / `has`

```scala
input |> contains(x);
input |> has(x);
```

**参数**：
- `x`（任意类型，必填）：侦察期所需锁定的潜在线索与痕迹。

**返回机制**：
两者语用等效：全天候扫描并下达结论判定 `bool`。
- `string` 要求指纹同为串型作内部交集搜索；
- `array` 要求按 `==` 深层相符标准在池塘中打捞；
- `dict` / `object` 仅针对所配属的顶端词条“键空间”勘验它是否存活。

#### `index_of`

```scala
input |> index_of(x);
```

**参数**：
- `x`（任意类型，必填）：待缉拿的靶标元素或潜逃子段落。

**返回机制**：
严格侦测主载体（限 `array`、`string`），若有相等相干物体，则原路汇报首次成功发现所在的整型下标坐标 `int`（从零开计）。未觅得则抛回 `-1`。

#### `slice`

```scala
input |> slice(start, length);
```

**参数**：
- `start` (`int`，必填)：截取的发起点。
- `length` (`int`，必填)：希望挽救出的额定数量。

**返回机制**：
通用阵列子集截取阶段。与 `substring` 对于文字范式的策略完全同构。超大容限设计保证即便遭遇边缘危机也必定安全产出符合条件的部分序列或者是 `""` 与 `[]`。

#### `reverse`

```scala
input |> reverse;
```

**参数**：
- 无显式参数。

**返回机制**：
将 `array` 队列或 `string` 列阵排出的所有内容依颠倒时空的规则执行首尾互换。返送全新倒序排列的结构体。

#### `repeat`

```scala
input |> repeat(count);
```

**参数**：
- `count` (`int`，必填)：指令循环复读的硬倍数（不可小于零）。

**返回机制**：
驱动令 `array` 阵列本身或 `string` 段体本利堆叠连带组装出 `count` 个相连体，退回巨大规模的新型增量串/组列。

#### `take` 与 `skip`

```scala
input |> take(count);
input |> skip(count);
```

**参数**：
- `count` (`int`，必填)：截留配合的数量额度（不可为负）。

**返回机制**：
针对于 `array`、`string` 的防过载阶段。`take` 只批准并聚敛排位靠首的数名元素；而 `skip` 专门忽视漏走队伍前排跳过后置全部元素。若过界则接盘全部或置空。

#### `distinct`

```scala
input |> distinct;
```

**参数**：
- 无显式参数。

**返回机制**：
去重提纯阶段。清退 `array` 中全部相复重叠的冗杂元素，仅仅保障那些首次出现的正身原貌，稳定序列秩序。

#### `sort` 与 `sort_desc`

```scala
input |> sort;
input |> sort_desc;
```

**参数**：
- 无显式参数。

**返回机制**：
数组排序指令：限制输入必须是纯粹内含 `int`、`string` 或 `bool` 的单一元素队列。进行递增或降序梯度的 `array` 提取。

#### `sum`

```scala
input |> sum;
```

**参数**：
- 无显式参数。

**返回机制**：
纯粹整数阵的累加。要求所有元素全为 `int`，合并归约为单一的 `int`。

#### `flatten`

```scala
input |> flatten;
```

**参数**：
- 无显式参数。

**返回机制**：
针对其中含包裹子 `array` 的输入数组执行硬核解压法：只剥除单层嵌套将全体人员合流为一维长线状的新 `array`。

#### `chunk`

```scala
input |> chunk(size);
```

**参数**：
- `size` (`int`，必填)：划分块距尺幅要求。

**返回机制**：
对 `array` 或 `string` 启动成组截断包裹动作。按照硬性容量组建小规模群落。返出多切块的层组 `array`。

#### `zip`

```scala
input |> zip(other);
```

**参数**：
- `other` (`array`，必填)：随从伴飞协同阵列。

**返回机制**：
将两条 `array` 羁绊缝合，依索引分别各取一人组结两人子数组，总成员数由相对残缺较短的一方决定。反馈双生结构的新 `array`。

#### `window`

```scala
input |> window(size);
```

**参数**：
- `size` (`int`，必填)：滑动窥视框架数。

**返回机制**：
滑动剖析：沿途平移步进 `array`/`string` 产出每片段。视界超限直接悬空退回 `[]`。

#### `append` / `push` 与 `prepend`

```scala
input |> append(x);
input |> push(x);
input |> prepend(x);
```

**参数**：
- `x`（任意类型，必填）：塞入新增点。

**返回机制**：
序列扩充流。前两者向 `array` 大队列背侧注入添兵；`prepend` 插队至序列首排。

#### `head` 与 `last`

```scala
input |> head;
input |> last;
```

**参数**：
- 无显式参数。

**返回机制**：
对线性序列首位与末尾的提取。序列枯竭际返退宽大处理结果 `nil`。

#### `keys` / `values` / `entries`

```scala
input |> keys;
input |> values;
input |> entries;
```

**参数**：
- 无显式参数。

**返回机制**：
字典或对象解析三式：
- `keys`：提炼名称集合成 `string` 数组（系统恒定自排序）。
- `values`：跟随排好的键名呈上对应身价数据列表。
- `entries`：打碎为主键值装袋字典组，形状类似 `{key: "K", value: "V"}`。

#### `pick` 与 `omit`

```scala
input |> pick(key1, key2, ...);
input |> omit(key1, key2, ...);
```

**参数**：
- `key1, key2, ...`（`string` 或标识符，可选多项）：放行集/通缉集。

**返回机制**：
- `pick` 只取指定键而无情断绝其他多余成员；
- `omit` 专踢特定目标项并维持其他不动。
所有处理将迫使源对象转生为褪去光环的全新普惠制 `dict`。不牵涉抛波报错。

#### `merge`

```scala
input |> merge(other);
```

**参数**：
- `other` (`dict` 或 `object`，必填)：另一独立版图域。

**返回机制**：
合并吞没输入：存在字眼键位对撞时客将 `other` 当仁不让地顶替原主配置，其结果彻底转回常态纯 `dict`。

#### `rename`

```scala
input |> rename(from, to);
```

**参数**：
- `from` (`string` 或标识符，必填)：需变更的现有名条。
- `to` (`string` 或标识符，必填)：顶替其后的新王封号。

**返回机制**：
对 `dict` 或 `object` 的局部字段修饰替换表皮操作，若 `from` 原本就不存在则不惹任何麻烦安稳复刻放归管道。强制产出退化型 `dict` 副本。

```scala
{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
// {name: Alice, total_score: 95}
```

---

### 12.7 集合式管道阶段

这些阶段对容器（主攻 `array` 与结构体记录集合）进行一连串泛型批量重组分发流计算。`callable` 参数统一支持 stage 名、callable 名、`pipe` 匿名对象或组合子闭包衍生对象。

#### `map`

```scala
input |> map(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：承担计算转换重任的回调阶段闭包。
- `...`（任意多维附加静态参数）：会自动绑定到闭包内。

**返回机制**：
- 输入为 `array`：返回同维 `array`。
- 输入为 `stream`：返回惰性 `stream`（不会立刻遍历所有元素）。

```scala
[1, 2, 3] |> map(bind(add, 10)) |> emit;    // [11, 12, 13]
range(1000000) |> map(bind(add, 1)) |> take(3) |> emit; // [1, 2, 3]
```

#### `pmap`

```scala
input |> pmap(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：并行映射回调。
- `...`（任意附加参数）：透传给回调。

**返回机制**：
对数组执行并行映射，保持输出顺序与输入顺序一致。若输入为 `stream`，会先收集为数组再并行映射。

#### `flat_map`

```scala
input |> flat_map(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：必须且必定会返还小型 `array` 结构的扩展者回调闭包。

**返回机制**：
一转多：先如 `map` 般各显神通演化各成建制的多个子级 `array` 阵营，最终暴力拆毁所有的屏障薄膜，将其悉数打平压平连贯为一支庞大的满血 `array` 主队列返回。

#### `filter`

```scala
input |> filter(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：用于审问判别通过资格的检察官闭包。

**返回机制**：
执行优胜劣汰式检视，仅将经受住了审查（调用返回 真值 Truthy）的兵员保送放行，抛弃杂质后归结为更修长精简版的强力 `array` 返场。

#### `each`

```scala
input |> each(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：纯粹主抓附加副作用的执仗人闭包。

**返回机制**：
专设为“挂载副作用”打工。让每人都受洗一般被其波及扫荡一边（用于打日志 `emit` 等外部通信行为），但结束后完全不影响原输入大军前进步伐，**原封不动推回全体 `array`**。

#### `reduce` 与 `scan`

```scala
input |> reduce(callable, initial);
input |> scan(callable, initial);
```

**参数**：
- `callable` (`callable`，必填)：收割机闭包，需严格接收 `(累积底盘 acc, 目标单兵 item)` 两大维度入参。
- `initial`（任意类型，必填）：滚雪球首日基地的最初筹码数值或结构模型。

**返回机制**：
- `reduce`：从 `initial` 本钱起跑，借由 `callable` 一步一个脚印将大列里的目标合并消融，最后终结反馈终极的**单体极终汇率值**。
- `scan`：步骤等若同上照办。返回一个按年轮记录下了每一次迭代后产生历史留痕总额身价图谱的一条**演变长龙 `array` 序列**。

#### `find` / `all` / `any`

```scala
input |> find(callable, ...);
input |> all(callable, ...);
input |> any(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：用以打灯侦测发光的哨兵验证回调闭包。

**返回机制**：
- `find`：原路且完整安全地返回首个通过侦测验证（Truthy）的个体对象，全灭则绝望抛出 `nil`。
- `all`：判断全体是否为真。全员集体发光则送 `true`，一人隐没即退 `false`（输入空白列一律无过错算作 `true` ）。
- `any`：网开一面版。一人发光即得全域拯救而立刻立表 `true`，全盘死寂再送 `false`。

#### `tap`

```scala
input |> tap(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：截流检查用或引发支流旁支副作用的阶段。

**返回机制**：
不限输入载体形式。把全盘家当托底借给予闭包把玩巡视一次，结束后强硬夺回并**全额归还源对象结构且不减分毫**以继续后续的传输链。常在此安置透视探针打表查错。

#### `group_by`

```scala
input |> group_by(callable, ...);
```

**参数**：
- `callable` (`callable`，必填)：提炼分类归依回调判断，产出反馈必须清一色全是 `string` 作为同类别徽标。

**返回机制**：
聚沙成塔集结兵力。通过标签识别归类。返出的是以所得徽标字符串为词条锁匙（`key`），以下方列队完毕的小支序列 `array` 为麾领组织价值（`value`）的军团化**分组归纳大图谱字典 `dict` 结构**。

#### `pluck`

```scala
input |> pluck(key);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：需要横扫拔取收缴上来的统一名号旗帜名称。

**返回机制**：
专门针对于充盈着字典/对象的 `array` 部队。秋风扫叶般把每个对象内同名 `key` 的随身名牌值统统摘落提取，抽出为一条轻快干练的**新字段序列数组 `array`**。如名册身前不曾带有此卡牌者一律当场获取 `nil` 加持占位以免顺序撕裂。

#### `where`

```scala
input |> where(key, value);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：比对指标名。
- `value`（任意类型，必填）：判据等价额界限（通常为裸标识或 `$var`）。

**返回机制**：
字典或对象数组普查筛漏器过滤。产出并截获那些个体字典记录内存留对应着等价标段（自身之 `[key] == value` ）的特定优待人群编成精萃**缩影保留队伍 `array`** 回流。

#### `index_by`

```scala
input |> index_by(key);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：指定主键字段属性名。

**返回机制**：
字典实体化倒排重构建制：将整齐的结构方阵阵列化作一张超大网格 `dict` 回传。内里凭栏借其各自记录中指定域段值抽出重定义为主干悬挂网格横梁键（要求键所填对应必须是字串）。面逢雷同撞车案犯一概以后位压退旧档掩盖补位为主。

#### `count_by`

```scala
input |> count_by(key);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：供集中核查频次的挂榜字段键名。

**返回机制**：
针对字典数组各户人丁抽出指名字段进行同类目普查计数结算。造册并下发一部关于各界类别字符串作为悬挂标签 `key` 、频频刷脸出现累加总次数计录为成绩 `value` (即为直观整型 `int`) 所组建的总账簿汇总表形式 `dict` 。

#### `sort_by` / `sort_desc_by`

```scala
input |> sort_by(key);
input |> sort_desc_by(key);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：排位权重的字段名。

**返回机制**：
队列按记录内所含权重重铸阶梯新序。被抽验基准权重不许乱象杂糅且仅框死限定在单纯可比对的 `int` / `string` / `bool`。一波走后全列返送出规制良好的纯按序递进甚至坡落衰败的新阶层 `array`。

#### `distinct_by`

```scala
input |> distinct_by(key);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：当家作主查重剔骨比照尺标的核心字段号。

**返回机制**：
字典数组去重阶段。依时间线顺位检举入库，扫描核心字段值重复同框的影子记录予以完全排插无情抛放外太空。终得并交货一套**清奇防撞车序列的新阵列**无杂质护体。

#### `sum_by`

```scala
input |> sum_by(key);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：瞄准提取以释放其整型存款价值的特定提款字段键。

**返回机制**：
抽丝剥茧级收成器。深究要求数组内部各自挂牌该条字段皆悉数归顺为严格同源结构下的整数 `int` 存款性质。之后对其推衍大一统合计算单据操作，抛出且统还大项单件数字模型回手 `int`。

#### `evolve` 与 `derive`

```scala
input |> evolve(key, callable);
input |> derive(key, callable);
```

**参数**：
- `key`（`string` 或裸标识符，必填）：加工点锚定区与将生成的田字格。
- `callable` (`callable`，必填)：施加演化回调与衍生加工闭包。

**返回机制**：
结构突变大产线机制，兼顾支持单独个护结构字典抑或是集团并排数组集群体调度：
- **`evolve`** 会精确下钻，提取主单字典原本键位藏着的旧残余身躯体投递闭包接受单一深造再遣反而归原位原名。
- **`derive`** 视域维权更高位：交纳给予大刀闭包的不仅是单格视野片面残局，其直面入参全托呈现给操作者的即为**整具无删减版的当前整个户头大框架景图字典本身**作全盘分析参谋从而借刀发力定格出一个新生开创性子级属性域网段（存则覆抹，未存即立）。
双管齐下最终所皆产发呈现的是经历了一番宏观与微观演变更迭的全盛型精装大版完整 `dict` 结构亦或连发改造的阵列版 `array` 制式。

**差异辨明**：
- `evolve(name, upper)` 里 `upper` 仅能摸见和调起 `name` 内的一个独立文字信息端进行纯加工重打包装。
- `derive(level, badge)` 里 `badge` 则如同上帝之眼检视主体所有现有维度总面目，从而推拿产出最后总分级派发印记。

---

## 13. 内建名称索引

按字母排序。别名在括号中标注。

```text
add          append (push)    any          at (get)
bind         bool             branch       chain
choose       chunk            concat       contains (has)
count (size) count_by         default      derive
distinct     distinct_by      div          drop
each         emit (print, show)            ends_with
entries      eq               evolve       field (get)
filter       find             flat_map     flatten
get          give             group_by     gt
gte          guard            head         index_by
index_of     input            insert       int
into (store) join             keys         last
lower (to_lower)              lt           lte
map          max              merge        min
mod          mul              ne           not
omit         pick             pluck        prepend
pmap         push (append)    range        read_file
reduce       remove           rename       repeat
replace      reverse          scan         set
show (emit)  size (count)     skip         slice
sort         sort_by          sort_desc    sort_desc_by
split        starts_with      store (into) str
sub          substring        sum          sum_by
take         tap              to_lower (lower)
to_upper (upper)              trim         type_of
update       upper (to_upper) values       where
window       zip
```

---

## 14. 错误行为

错误分三类：

### 14.1 词法错误

- 非法转义序列
- 未闭合字符串

### 14.2 语法错误

- 缺失分号或括号
- 不完整的语句或表达式

### 14.3 运行时错误

- 访问不存在的变量
- 参数数量不匹配
- 类型不匹配（比如对字符串做算术）
- 除零或模零
- 数组/字符串下标越界（在 `remove`、`update`、`insert` 等写操作中）
- 在非管道上下文使用 `_`
- 在非循环上下文使用 `break` / `continue`
- 在非函数/方法体内使用 `give` / `return`
- `read_file` 无法打开文件
- `replace` 的 `from` 为空字符串

---

## 15. 示例

### 15.1 基础管道

```scala
"hello" |> upper |> emit;
```

### 15.2 函数与管道调用

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;      // 42
double(21) |> emit;        // 42
```

### 15.3 stream/stage 与 `$it`

```scala
stream wrap(left, right) {
    return concat($left, $it, $right);
}

"core" |> wrap("[", "]") |> emit;    // [core]
```

### 15.4 变量、赋值与循环

```scala
let total = 0;
let n = 5;

while $n > 0 {
    $total += $n;
    $n -= 1;
}

$total |> emit;    // 15
```

### 15.5 对象

```scala
type User(name, score) {
    fn badge() {
        when $self.score >= 90 {
            return "A";
        } else {
            return "B";
        }
    }
}

let user = User("Alice", 95);
$user.badge() |> emit;    // A
```

### 15.6 占位符与表达式管道

```scala
"Hello, Aethe!" |> substring(_, 7, 5) |> emit;    // Aethe
10 |> _ * 3 + 5 |> emit;                          // 35
```

### 15.7 匿名管道与组合子

```scala
let double = pipe(x) { return $x * 2; };
[1, 2, 3] |> map($double) |> emit;    // [2, 4, 6]

let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;    // HELLO!

5 |> guard(pipe(x) { return $x > 3; }, bind(add, 100)) |> emit;    // 105
```

### 15.8 字符串操作

```scala
"  Aethe Runtime  "
    |> trim
    |> upper
    |> replace("RUNTIME", "LANG")
    |> emit;
// AETHE LANG
```

### 15.9 集合管道

```scala
fn ge_three(x) { return $x >= 3; }

[1, 2, 3, 4] |> find(ge_three) |> emit;    // 3
[1, 2, 3, 4] |> all(ge_three) |> emit;     // false
[1, 2, 3, 4] |> any(ge_three) |> emit;     // true
[[1, 2], [3], [4, 5]] |> flatten |> emit;   // [1, 2, 3, 4, 5]
[1, 2, 3, 4] |> sum |> emit;               // 10
```

### 15.10 记录数组管道

```scala
let users = [
    {name: "Alice", role: admin, score: 95},
    {name: "Bob", role: guest, score: 88},
    {name: "Carol", role: admin, score: 91}
];

$users |> where(role, admin) |> pluck(name) |> emit;    // [Alice, Carol]
$users |> count_by(role) |> emit;                        // {admin: 2, guest: 1}
$users |> sort_desc_by(score) |> pluck(name) |> emit;   // [Alice, Carol, Bob]
$users |> sum_by(score) |> emit;                         // 274

[{name: " alice "}, {name: "bob"}]
    |> evolve(name, trim)
    |> evolve(name, upper)
    |> emit;
// [{name: ALICE}, {name: BOB}]

{name: "Alice", score: 95}
    |> derive(kind, type_of)
    |> emit;
// {name: Alice, score: 95, kind: dict}
```

### 15.11 for 双变量与 match guard

```scala
for index, item in [10, 20, 30] {
    [$index, $item] |> emit;
}
// [0, 10]
// [1, 20]
// [2, 30]

let score = 85;
match $score {
    case _ when $it >= 90 { "A" |> emit; }
    case _ when $it >= 80 { "B" |> emit; }
    else { "C" |> emit; }
}
// B
```

### 15.12 结构操作

```scala
{name: "Alice", score: 95, city: "Shanghai"} |> pick(name, city) |> emit;
// {name: Alice, city: Shanghai}

{name: "Alice", score: 95} |> merge({score: 99, level: "A"}) |> emit;
// {name: Alice, score: 99, level: A}

{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
// {name: Alice, total_score: 95}

{name: "Alice", score: 95} |> update(score, bind(add, 5)) |> emit;
// {name: Alice, score: 100}
```

---

## 16. 未实现特性

- 静态类型系统
- 类型注解
- 模块与导入系统

---

## 17. 内存模型与对象生命周期

Aethe 2 的底层由 C++ 实现，其内存管理机制和赋值语义在表现上与 C++ 智能指针（`std::shared_ptr`）或类似 Python 的引用动态语言一致，这与现代 C++ 中的**值语义（Value Semantics）**和**引用计数生命周期（Reference Counting Lifecycle）**直接相关。

### 17.1 值类型与引用类型差异

在 Aethe 中，运行时值被分为值类型（Trivial Types）与引用类型（Reference Types）。

- **值类型**：`int`、`bool`、`string`、`nil`
  - 采用**深拷贝 / 值拷贝（By Value）**。
  - 将一个数字或字符串赋给新变量，或作为参数截留，都会在内存中形成独立的副本。（注：目前的底层 `class Value` 对于 `string` 使用完整的 `std::string`，因此赋值本质上是 C++ 的字符串复制）。
- **引用类型**：`array`、`dict`、`object`、`callable`
  - 采用**浅拷贝 / 共享引用（By Reference）**。
  - 底层由 `std::shared_ptr` 承载结构体。赋值、传参和返回的成本总是 $O(1)$，修改某个位置的值，会反映到所有持有该引用的视图上。

```scala
let a = [1, 2, 3];
let b = $a;
$b[0] = 99;
$a |> emit; // [99, 2, 3] —— A 发生改变
```

### 17.2 对象生命周期与垃圾回收

Aethe 未实现诸如标记-清除（Mark & Sweep）或分代 GC，而是**完全对标 C++ 的内部机制——基于确定性引用计数（Deterministic Reference Counting）**：

- 当没有任何变量、管道持有对某对象、数组、字典或函数的引用时，该对象的引用计数降为 0，内存将被**立刻析构**。
- **循环引用泄漏风险**：如果发生循环引用（如 `$a[0] = $b; $b[0] = $a;`），根据 C++ `std::shared_ptr` 的强引用特性，这两个数组的内存将**永久泄漏**直至程序退出。

---

## 18. 作用域与名字解析

### 18.1 词法作用域（Lexical Scoping）

Aethe 具有严格的词法作用域。
名字决议在**运行时**从当前最内层作用域逐层向外查找，采用类似 C++ 中 Block Scope 的栈帧规则：

- `block`（如 `when`、`while`、`for` 的大括号内）会引入新的词法作用域。
- 内层作用域声明的 `let` 变量（显式写入，以及 `match`、`for` 中的暗访变量如 `$it`, 索引等）在退出结构时销毁。
- **暗影潜藏（Shadowing）**：允许在内层定义同名变量，遮蔽外层变量。

### 18.2 闭包变量捕获（Closure Capture by Value）

这对应于 C++11 Lambda 中十分经典的 `[=]` 捕获模式：

当定义一个匿名 `pipe` 时：
```scala
let multiplier = 5;
let scale = pipe(x) { return $x * $multiplier; };
```
解释器会**对此时词法作用域可见的所有局部变量做一次浅拷贝**，保存在新生成的 `callable` 对象的 `captured` 字典中。
- 捕获时刻发生在 `pipe` **生成（求值）瞬间**，不随外部变量后续改变而改变（值拷贝特性）。
- 然而，由于前面提过的“浅拷贝”规则，如果外部变量指向一个 `array`，闭包虽然捕获了引用，但后续依然可以通过此引用修改其内容。

---

## 19. 求值顺序与副作用

Aethe 参考了 C++17 后收紧求值顺序的原则，提供了相对确定的执行顺序：

### 19.1 表达式与阶段的顺序一致性

**从左到右严格求值（Strict Left-to-Right Evaluation）**：
- 函数参数：`foo(expr1, expr2)` 会先求 `expr1` 的值，再求 `expr2`，最后执行 `foo`。
- 复合表达式与赋值：`$a += foo()` 中，先获取左值 `$a` 对应的底层位置引用，再求 `foo()` 的值。
- 管道求值：管道表达式 `expr |> step1 |> step2` 将产生强制时间线。`expr` 求值完毕后立即交由 `step1` 处理，其结果再流向 `step2`。管道的求值在整个过程完成前是被阻塞的（无惰性执行）。

### 19.2 逻辑短路行为（Short-Circuiting）

对标 C++ 的布尔逻辑：
- `&&`（逻辑与）：左操作数为假（Falsy 值）时，立返该假值，**跳过**右操作数的执行。
- `||`（逻辑或）：左操作数为真（Truthy 值）时，立返该真值，**跳过**右操作数的执行。
利用这一特性可以编写安全防护：
```scala
// 若 user 没有配置 badge 字段，将安然通过，不发生字段访问的运行时报错。
$user != nil && $user.badge != "B" |> ...
```

---

## 20. 实现定义与未定义行为

借鉴 C++ 标准委员会的叙事物语，Aethe 提供某些未提供绝对承诺的行为（取决于编译器或主机的 C/C/C++ 运行时环境）：

### 20.1 实现定义行为（Implementation-Defined Behavior, IDB）

- **整数的大小界限与溢出**：对于 `int` 类型，Aethe 并未实现大整数算术。目前的整型对应于作为主机的 C++ 环境中 `int` 类型（通常为 32 位有符号整数）。数值溢出将直接继承底层编译器的实现。
- **关联容器中的序列顺序**：尽管 `dict` 和 `object` 具有确定的语义查找，内部通过 `std::unordered_map` 实现，当直接 `emit` 或者遍历时，除非通过 `keys` 和 `entries` 排序获取，普通的内置显示可能带来不可预测的键输出顺序。
- **并发环境**：当前的 Aethe 不提供任何线程支持与全局解释器锁（GIL），完全为单线程隔离模型。

### 20.2 未定义行为（Undefined Behavior, UB）

由于 Aethe 是构建于 C++ 层上的强类型沙盒虚拟语言，一般情况下不允许通过 Aethe 语言内部代码触发如“越界崩溃、段错误”的底层 C++ 内存泄漏行为，但存在部分极度罕见却无法从前沿捕获的未定义行为边界：
- 使用深度极其恐怖的函数、流与闭包互相递归可能耗尽系统的原生函数调用栈（Stack Overflow），引起操作系统级的程序终止。
- 创建恶意极长的循环并不断生成巨大数据导致内存爆满，会遭遇操作系统级的 OOM（Out Of Memory），直接结束进程。
