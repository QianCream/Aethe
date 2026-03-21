# Aethe 2 语言参考

本文档描述 `Aethe 2 / 2.1.0` 当前实现的词法、语法、语义、运行时行为与内建能力.

## 目录

- [1. 实现范围](#1-实现范围)
- [2. 词法约定](#2-词法约定)
- [3. 类型与值](#3-类型与值)
- [4. 真值规则](#4-真值规则)
- [5. 变量与裸标识符](#5-变量与裸标识符)
- [6. 表达式](#6-表达式)
- [7. 管道语义](#7-管道语义)
- [8. 语句](#8-语句)
- [9. 可调用对象](#9-可调用对象)
- [10. 对象系统](#10-对象系统)
- [11. REPL 执行模型](#11-repl-执行模型)
- [12. 内建调用与阶段](#12-内建调用与阶段)
- [13. 内建名称索引](#13-内建名称索引)
- [14. 错误行为](#14-错误行为)
- [15. 示例](#15-示例)
- [16. 未实现特性](#16-未实现特性)

## 1. 实现范围

Aethe 当前实现具有以下范围与限制：

- 单文件解释器实现，源码位于 [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)
- 目标编译标准为 C++11
- 运行方式为交互式 REPL
- 不支持脚本文件加载与执行
- 不支持模块系统
- 不支持静态类型系统
- 不支持类型注解

当前已实现：

- 函数：`fn` / `flow`
- 管道阶段：`stage`
- 匿名管道值：`pipe(...) { ... }`
- 对象构造与方法：`type`
- 条件、匹配、循环与延迟执行
- 自动首参注入、占位符注入、多位置复用与裸表达式管道
- 裸标识符作为符号值参与变量名、字段名和筛选值表达
- 常用字符串、容器、集合式管道与类型转换内建能力

## 2. 词法约定

### 2.1 空白符

空格、制表符、换行符和回车符用于分隔记号。除字符串字面量内部外，空白符本身不携带语义。

### 2.2 注释

仅支持单行注释。

语法：

```scala
// comment
```

注释从 `//` 开始，持续到当前行结束。

### 2.3 标识符

标识符用于表示：

- 函数名
- stage 名
- 类型名
- 变量名
- 字段名
- 裸标识符值

约定：

- 首字符可以是字母或下划线
- 后续字符可以是字母、数字或下划线

示例：

```scala
name
user_score
_tmp
Value42
```

### 2.4 关键字

当前关键字如下：

- `fn`
- `flow`
- `stage`
- `type`
- `when`
- `else`
- `match`
- `case`
- `while`
- `for`
- `in`
- `give`
- `return`
- `let`
- `defer`
- `break`
- `continue`
- `pipe`
- `true`
- `false`
- `nil`

### 2.5 数字字面量

当前仅支持十进制整数。

语法：

```scala
0
42
1024
```

说明：

- 不支持浮点数
- 不支持十六进制
- 负数由一元 `-` 运算生成，而不是单独的负数字面量

### 2.6 字符串字面量

字符串使用双引号表示。

语法：

```scala
"hello"
```

支持的转义序列：

- `\n`
- `\t`
- `\"`
- `\\`

非法转义会触发词法错误。

### 2.7 符号

当前涉及的主要符号包括：

- `|>`
- `(` `)`
- `{` `}`
- `[` `]`
- `,` `:` `;`
- `$` `.` `_`
- `+` `-` `*` `/` `%`
- `!` `==` `!=` `>` `>=` `<` `<=`
- `&&` `||`
- `=`

## 3. 类型与值

Aethe 是动态类型语言。每个值在运行时携带具体类型。

### 3.1 支持的运行时类型

- `int`
- `bool`
- `string`
- `nil`
- `array`
- `dict`
- `object`
- `callable`

### 3.2 `int`

表示整数值。

示例：

```scala
10
-3
```

### 3.3 `bool`

表示布尔值。

```scala
true
false
```

### 3.4 `string`

表示字符串值。

```scala
"Aethe"
```

### 3.5 `nil`

表示空值。

```scala
nil
```

`nil` 常见用途：

- 缺失值
- `drop` 的返回值
- 越界读取失败时的返回值

### 3.6 `array`

表示有序值序列。

语法：

```scala
[1, 2, 3]
["a", "b", "c"]
[]
```

特性：

- 元素可异构
- 支持读取、追加、头尾访问、映射与过滤

### 3.7 `dict`

表示键值映射。

语法：

```scala
{name: "Alice", score: 99}
{"lang": "Aethe", "ver": 1}
{}
```

约束：

- 键必须是字符串字面量或裸标识符
- 运行时内部统一按字符串键存储

### 3.8 `object`

对象值由 `type` 构造产生。

示例：

```scala
User("Alice", 99)
```

对象包含：

- 类型名
- 字段表
- 通过类型定义绑定的方法

### 3.9 `callable`

表示可调用值。

来源包括：

- `fn` / `flow` 名字
- 内建可调用对象名
- 匿名 `pipe(...) { ... }` 表达式
- `bind(...)`、`chain(...)`、`branch(...)`、`guard(...)` 生成的组合子值
- 存有可调用值的变量

## 4. 真值规则

以下值被解释为假：

- `nil`
- `false`
- `0`
- 空字符串 `""`
- 空数组 `[]`
- 空字典 `{}`

其余值均为真。

真值规则影响以下语法：

- `when`
- `while`
- `choose(a, b)`
- 逻辑运算 `&&` / `||`

## 5. 变量与裸标识符

Aethe 明确区分“变量读取”和“裸标识符”。

### 5.1 裸标识符

裸标识符默认表示一个符号式字符串值，而不是变量内容。

示例：

```scala
name
score
```

常见用法：

```scala
100 |> into(score);
{name: "Alice"} |> get(name) |> emit;
[{name: "Alice", role: admin}, {name: "Bob", role: guest}] |> where(role, admin) |> pluck(name) |> emit;
```

说明：

- `score` 在 `into(score)` 中表示变量名
- `name` 在 `get(name)` 中表示字段名
- `role` 在 `where(role, admin)` 中表示字段名
- `admin` 在 `where(role, admin)` 中表示符号式字符串值，而不是变量内容

### 5.2 变量读取

变量读取必须显式使用 `$`。

语法：

```scala
$name
$score
```

示例：

```scala
let score = 100;
$score |> emit;
```

### 5.3 变量写入

变量写入有两种形式：

语法糖形式：

```scala
let score = 100;
```

管道形式：

```scala
100 |> into(score);
```

两者语义等价。

### 5.4 管道标识符风格

Aethe 鼓励把“名字”直接写进管道，而不是总是退回到字符串字面量。

示例：

```scala
let role = admin;

$users |> where(role, $role) |> pluck(name) |> emit;
$users |> index_by(name) |> emit;
$users |> count_by(role) |> emit;
$users |> sort_by(score) |> emit;
$users |> distinct_by(role) |> emit;
$users |> sum_by(score) |> emit;
$users |> evolve(name, upper) |> emit;
$users |> derive(label, badge) |> emit;
{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
```

说明：

- `role` 在 `where(role, $role)` 中是字段名
- `$role` 在同一个表达式里是变量读取
- `name` 在 `pluck(name)` 中是字段名
- `name` 在 `index_by(name)` 中也是字段名
- `role` 在 `count_by(role)` 中也是字段名
- `score` 在 `sort_by(score)` 与 `sum_by(score)` 中也是字段名
- `name` 在 `evolve(name, upper)` 中是字段名，而 `upper` 是可调用目标名
- `label` 在 `derive(label, badge)` 中是目标字段名，而 `badge` 是可调用目标名
- `score` 和 `total_score` 在 `rename(...)` 中都是符号值
- 这种“裸标识符负责命名、`$` 负责取值”的分工，是 Aethe 读写记录和构造管道时的重要特点

## 6. 表达式

### 6.1 概览

表达式用于生成值。Aethe 支持：

- 字面量表达式
- 数组与字典字面量
- 变量读取
- 裸标识符
- 括号表达式
- 调用表达式
- 成员访问表达式
- `pipe` 字面量表达式
- 一元表达式
- 二元表达式
- 管道表达式
- 占位符表达式

### 6.2 主表达式

主表达式包括：

- 数字字面量
- 字符串字面量
- `true`
- `false`
- `nil`
- 数组字面量
- 字典字面量
- 裸标识符
- 变量读取
- 括号表达式
- `pipe(...) { ... }`
- `_`

### 6.3 数组字面量

语法：

```scala
[expr, expr, ...]
```

示例：

```scala
[1, 2, 3]
[$a, $b, $c]
[]
```

### 6.4 字典字面量

语法：

```scala
{key: expr, key: expr, ...}
```

示例：

```scala
{name: "Alice", score: 99}
{"lang": "Aethe"}
{}
```

### 6.5 调用表达式

语法：

```scala
callee(arg1, arg2, ...)
```

`callee` 可以是：

- `fn` / `flow`
- `type` 构造器
- 对象方法
- 内建可调用对象，如 `range`、`str`、`int`、`bool`
- 变量中的可调用值
- 匿名 `pipe(...) { ... }`
- 由 `bind(...)`、`chain(...)`、`branch(...)`、`guard(...)` 生成的组合子值

示例：

```scala
add(1, 2)
User("Alice", 99)
$user.badge()
$double(21)
pipe(x) { return $x * 2; }(21)
```

### 6.6 `pipe` 字面量表达式

语法：

```scala
pipe(param1, param2, ...) {
    ...
}
```

说明：

- `pipe` 生成一个可调用值
- 参数在体内通过 `$name` 读取
- 体内可以使用与 `fn` 相同的语句，并通过 `return` / `give` 返回结果
- `pipe` 会捕获定义位置可见的变量，因此可形成闭包

示例：

```scala
let n = 5;
let add_n = pipe(x) {
    return $x + $n;
};
10 |> $add_n |> emit;

let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;
```
### 6.7 成员访问

语法：

```scala
expr.member
expr.method(...)
```

适用对象：

- `dict`
- `object`

对于对象，成员访问既可读取字段，也可进一步触发方法调用。

### 6.8 一元运算

支持的运算符：

- `!expr`
- `-expr`

语义：

- `!expr`：按真值规则求逻辑非
- `-expr`：要求操作数可解释为整数

### 6.9 二元运算

支持的运算符分组：

- 算术：`+` `-` `*` `/` `%`
- 比较：`==` `!=` `>` `>=` `<` `<=`
- 逻辑：`&&` `||`

说明：

- 算术运算要求参与值可解释为整数
- `==` 与 `!=` 允许对所有运行时值做比较
- `&&` 与 `||` 按真值规则短路求值

### 6.9 运算符优先级

从高到低：

1. 调用与成员访问
2. 一元运算 `!` `-`
3. 乘除模 `*` `/` `%`
4. 加减 `+` `-`
5. 比较 `>` `>=` `<` `<=`
6. 相等性 `==` `!=`
7. 逻辑与 `&&`
8. 逻辑或 `||`
9. 管道 `|>`

示例：

```scala
1 + 2 * 3
($x + 1) |> emit;
```

## 7. 管道语义

### 7.1 总体模型

管道是 Aethe 的核心语法：

```scala
source |> target |> target;
```

每一步都会把左侧结果传递到右侧目标，并把计算结果继续向后流动。

### 7.2 自动首参注入

当右侧目标不包含 `_` 时，左侧结果会自动成为右侧目标的第一个参数。

示例：

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;
```

等价理解：

```text
double(21)
```

同理：

```scala
10 |> add(20)
```

等价于：

```text
add(10, 20)
```

### 7.3 占位符注入

当右侧表达式包含 `_` 时，`_` 的位置就是当前输入的注入位置。

示例：

```scala
"Hello, Aethe!" |> substring(_, 7, 3) |> emit;
```

等价理解：

```text
substring("Hello, Aethe!", 7, 3)
```

### 7.4 多位置复用

同一输入值可以在一个管道目标中出现多次。

```scala
5 |> power(_, _) |> emit;
```

这意味着相同输入会被多次复制到占位位置，而不是被消费一次。

### 7.5 裸表达式目标

右侧目标可以是表达式，而不必是函数名或 stage 名。

```scala
10 |> _ * 3 + 5 |> emit;
```

这类表达式会在当前输入上下文中直接求值。

### 7.6 `_` 的语义边界

`_` 的含义固定为“当前这一步管道的输入值”。

它不是：

- 匿名函数
- lambda 参数
- 可被单独存储的值占位器

如果你需要匿名可调用对象，请使用 `pipe(...) { ... }`。

合法示例：

```scala
"hallo word" |> my_func(substring(_, 7, 3), substring(_, 7, 3));
```

这里两个 `substring(_, 7, 3)` 会先被求值为字符串，再作为参数传给 `my_func`。

非法示例：

```scala
_ * 3
```

因为 `_` 只能在管道右侧使用。

### 7.7 管道目标类型

无占位符时，右侧目标通常为：

- stage 名
- `fn` 名
- `stage(...)`
- `fn(...)`
- 变量中的 `pipe` 值，例如 `$double`
- 匿名 `pipe(...) { ... }`

有占位符时，右侧目标可以是一般表达式。

## 8. 语句

### 8.1 表达式语句

语法：

```scala
expr;
```

示例：

```scala
1 + 2 |> emit;
"abc" |> upper |> emit;
```

### 8.2 `let`

语法：

```scala
let name = expr;
```

说明：

- `let` 是变量定义语法糖
- 语义上等价于 `expr |> into(name);`

示例：

```scala
let score = 100;
let name = "Alice";
```

### 8.3 `when`

语法：

```scala
when condition {
    ...
} else {
    ...
}
```

说明：

- `condition` 按真值规则求值
- `else` 为可选分支
- 分支体是语句块

示例：

```scala
when $score >= 60 {
    "pass" |> emit;
} else {
    "fail" |> emit;
}
```

### 8.4 `match`

语法：

```scala
match expr {
    case value {
        ...
    }
    case value {
        ...
    }
    else {
        ...
    }
}
```

说明：

- `expr` 会先求值一次
- `case` 按相等性比较匹配
- `else` 为可选兜底分支

### 8.5 `while`

语法：

```scala
while condition {
    ...
}
```

说明：

- 每轮循环前重新计算 `condition`
- 循环体可包含 `break` 与 `continue`

### 8.6 `for`

语法：

```scala
for item in expr {
    ...
}
```

当前 `expr` 可为：

- `array`
- `string`
- `dict`

迭代行为：

- 数组：逐个绑定数组元素
- 字符串：逐个绑定单字符字符串
- 字典：绑定 `{key: ..., value: ...}` 形式的字典值

### 8.7 `break`

语法：

```scala
break;
```

作用：

- 立即退出当前最近一层循环
- 在非循环上下文中使用会触发运行时错误

### 8.8 `continue`

语法：

```scala
continue;
```

作用：

- 立即结束当前轮循环
- 进入下一轮
- 在非循环上下文中使用会触发运行时错误

### 8.9 `defer`

语法：

```scala
defer {
    ...
}
```

作用：

- 将一个语句块登记到当前作用域退出时执行
- 适合收尾、日志和延迟输出

### 8.10 `give` / `return`

语法：

```scala
$value |> give;
return $value;
```

说明：

- 两者等价
- 只能在 `fn`、`flow`、`stage` 或方法体内部使用
- 无返回值时可写 `return;`

## 9. 可调用对象

### 9.1 `fn`

`fn` 定义普通函数。

语法：

```scala
fn name(param1, param2, ...) {
    ...
}
```

示例：

```scala
fn add(a, b) {
    return $a + $b;
}
```

### 9.2 `flow`

`flow` 是 `fn` 的别名。

语法：

```scala
flow name(param1, param2, ...) {
    ...
}
```

说明：

- 语义与 `fn` 完全相同
- 保留 `flow` 主要是为了强调“数据流函数”的命名风格

### 9.3 `stage`

`stage` 定义面向管道的处理阶段。

语法：

```scala
stage name(param1, param2, ...) {
    ...
}
```

特殊变量：

- `$it`：当前管道输入值

示例：

```scala
stage shout(suffix) {
    return $it |> upper |> concat($suffix);
}
```

### 9.4 `pipe`

`pipe` 定义匿名可调用值。

语法：

```scala
pipe(param1, param2, ...) {
    ...
}
```

示例：

```scala
let double = pipe(x) {
    return $x * 2;
};

[1, 2, 3] |> map($double) |> emit;
[1, 2, 3, 4] |> reduce(pipe(acc, item) { return $acc + $item; }, 0) |> emit;
```

说明：

- `pipe` 是表达式，不是顶层定义语句
- 可以存入变量、作为参数传递、作为普通调用目标、也可直接作为管道目标
- 会捕获定义位置可见的变量

`bind`、`chain`、`branch`、`guard` 也会生成同样的 `callable` 值，只是它们更偏向“组合已有步骤”而不是手写匿名体。

### 9.5 选择建议

适合使用 `fn` 的场景：

- 逻辑以显式参数列表为主
- 既可能普通调用，也可能管道调用

适合使用 `stage` 的场景：

- 逻辑天然对应“当前流过来的值”
- 主要作为管道步骤使用

适合使用 `pipe` 的场景：

- 只在局部使用一次或少量次数
- 需要把可调用对象直接传给 `map`、`filter`、`reduce`
- 需要匿名闭包而不想额外声明顶层 `fn`

适合使用 `bind` / `chain` / `branch` / `guard` 的场景：

- 已有步骤足够，但你想把它们重新装配成新的可调用值
- 需要把“带参数的 stage”转成可传递对象，例如 `bind(get, name)`、`bind(add, 10)`
- 需要把同一路输入扇出成多路结果，或把条件分流打包成一个步骤

## 10. 对象系统

### 10.1 `type`

`type` 用于定义对象构造器、字段和方法集合。

语法：

```scala
type TypeName(field1, field2, ...) {
    fn method(...) {
        ...
    }
}
```

说明：

- 构造参数会成为对象字段
- 类型体内只允许方法定义
- 方法使用 `fn` / `flow` 语义

### 10.2 构造

构造语法：

```scala
TypeName(arg1, arg2, ...)
```

示例：

```scala
let user = User("Alice", 99);
```

### 10.3 方法

方法在运行时绑定到对象类型。

方法体内可访问：

- `$self`：当前对象

示例：

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
```

### 10.4 成员访问

方法调用：

```scala
$user.badge()
```

字段访问：

```scala
$user.name
```

## 11. REPL 执行模型

REPL 不是逐行自动执行，而是缓冲执行。

规则：

- 输入的多行内容会先进入缓冲区
- 输入 `run` 后统一解析与执行
- 如果缓冲区不完整，则拒绝执行
- 执行完成后清空当前缓冲区
- 已定义的函数、stage 与类型在解释器实例中保留

提示符含义：

- `>>>`：当前缓冲区为空
- `...>`：当前缓冲区非空
- 程序执行期间如果调用 `input()` 或 `input(prompt)`，会直接从标准输入读取一行文本
- 如果 `input()` 读到 EOF，则返回 `nil`

## 12. 内建调用与阶段

### 12.1 使用约定

本节将所有内建能力按条目列出。除特别说明外，管道阶段都遵循以下约定：

- 语法中的 `input` 表示当前管道输入
- 如果条目写成 `input |> name(...)`，表示该能力以管道阶段形式调用
- 如果条目写成 `name(...)`，表示该能力以普通调用形式调用
- 类型不匹配或参数数量不匹配时会触发运行时错误
- 本节中的 `callable` 参数统一支持三类值：stage 名、普通 callable 名如 `type_of`，以及 `pipe` / 组合子生成的值

### 12.2 通用可调用对象

#### `range`

语法：

```scala
range(end)
range(start, end)
```

参数：

- `end`：整数
- `start`：整数

结果：

- 返回整数数组

说明：

- `range(end)` 生成 `[0, end)` 区间
- `range(start, end)` 生成 `[start, end)` 区间

错误：

- 参数数量不是 1 或 2
- 参数不可解释为整数

示例：

```scala
range(5) |> emit;
range(2, 5) |> emit;
```

#### `str`

语法：

```scala
str(x)
```

参数：

- `x`：任意值

结果：

- 返回字符串

说明：

- 采用值的运行时字符串表示

错误：

- 参数数量不是 1

#### `int`

语法：

```scala
int(x)
```

参数：

- `x`：可转换值

结果：

- 返回整数

说明：

- `bool` 转换为 `0` 或 `1`
- 数字字符串会尝试解析为十进制整数

错误：

- 参数数量不是 1
- 字符串内容不是合法整数
- 值不可转换为整数

#### `bool`

语法：

```scala
bool(x)
```

参数：

- `x`：任意值

结果：

- 按真值规则返回布尔值

错误：

- 参数数量不是 1

#### `type_of`

语法：

```scala
type_of(x)
```

参数：

- `x`：任意值

结果：

- 返回运行时类型名字符串

说明：

- 结果范围与当前实现的运行时类型一致
- 典型返回值包括 `int`、`bool`、`string`、`nil`、`array`、`dict`、`object`、`callable`

错误：

- 参数数量不是 1

#### `input`

语法：

```scala
input()
input(prompt)
```

参数：

- `prompt`：字符串，可选

结果：

- 返回读取到的一行字符串
- 如果读到 EOF，则返回 `nil`

说明：

- `input()` 会从标准输入读取一整行，不包含行尾换行符
- `input(prompt)` 会先输出提示文本，再读取输入
- `input` 是普通可调用对象，不是管道阶段

错误：

- 参数数量大于 1
- `prompt` 不是字符串

示例：

```scala
let name = input("name> ");
$name |> emit;
```

#### `bind`

语法：

```scala
bind(callable, ...)
input |> bind(callable, ...);
```

参数：

- 第一个参数是可调用对象
- 后续参数会被绑定在输入值之后

结果：

- 普通调用形式返回一个新的单输入 `callable`
- 管道阶段形式会立刻把当前输入送入该调用目标

说明：

- 适合把 `get(name)`、`add(10)` 这类“带参数 stage”转换成可传递值

示例：

```scala
[1, 2, 3] |> map(bind(add, 10)) |> emit;
[{name: "Alice"}, {name: "Bob"}] |> map(bind(get, name)) |> emit;
```

#### `chain`

语法：

```scala
chain(callable1, callable2, ...);
input |> chain(callable1, callable2, ...);
```

参数：

- 每个参数都必须是可调用对象

结果：

- 普通调用形式返回一个新的单输入 `callable`
- 管道阶段形式返回串行执行后的最终结果

说明：

- 每一步都会接收上一步的结果
- 适合把常见清洗、格式化、记录改造步骤打包成“路线值”

示例：

```scala
let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;
```

#### `branch`

语法：

```scala
branch(callable1, callable2, ...);
input |> branch(callable1, callable2, ...);
```

参数：

- 每个参数都必须是可调用对象

结果：

- 普通调用形式返回一个新的单输入 `callable`
- 管道阶段形式返回结果数组，顺序与参数顺序一致

说明：

- 每条路线都会收到同一个原始输入

示例：

```scala
"Aethe" |> branch(type_of, str, bind(concat, "!")) |> emit;
```

#### `guard`

语法：

```scala
guard(test, on_true)
guard(test, on_true, on_false)
input |> guard(test, on_true)
input |> guard(test, on_true, on_false);
```

参数：

- `test`：谓词可调用对象
- `on_true`：谓词为真时执行的路线
- `on_false`：可选，谓词为假时执行的路线

结果：

- 普通调用形式返回一个新的单输入 `callable`
- 管道阶段形式返回被选中的路线结果
- 当未提供 `on_false` 且谓词为假时，返回原始输入

示例：

```scala
5 |> guard(pipe(x) { return $x > 3; }, bind(add, 100), bind(sub, 100)) |> emit;
```

### 12.3 输出与状态阶段

#### `emit`

语法：

```scala
input |> emit;
```

参数：

- 无

结果：

- 返回原值

说明：

- 输出当前值的字符串表示

错误：

- 传入任何参数

#### `print`

语法：

```scala
input |> print;
```

别名：

- `emit`

结果：

- 返回原值

#### `show`

语法：

```scala
input |> show;
```

别名：

- `emit`

结果：

- 返回原值

#### `into`

语法：

```scala
input |> into(name);
```

参数：

- `name`：裸标识符或可解释为字符串的名称

结果：

- 返回原值

说明：

- 将当前值写入变量 `name`

错误：

- 参数数量不是 1

#### `store`

语法：

```scala
input |> store(name);
```

别名：

- `into`

结果：

- 返回原值

#### `drop`

语法：

```scala
input |> drop;
```

参数：

- 无

结果：

- 返回 `nil`

错误：

- 传入任何参数

#### `give`

语法：

```scala
input |> give;
```

参数：

- 无

结果：

- 从当前函数、stage 或方法体中返回 `input`

错误：

- 在 `fn`、`flow`、`stage` 或方法体外使用
- 传入任何参数

### 12.4 数值与逻辑阶段

#### `add`

语法：

```scala
input |> add(x);
```

参数：

- `x`：整数

结果：

- `input + x`

错误：

- `input` 或 `x` 不可解释为整数

#### `sub`

语法：

```scala
input |> sub(x);
```

结果：

- `input - x`

#### `mul`

语法：

```scala
input |> mul(x);
```

结果：

- `input * x`

#### `div`

语法：

```scala
input |> div(x);
```

结果：

- `input / x`

错误：

- `x == 0`
- 任一参与值不可解释为整数

#### `mod`

语法：

```scala
input |> mod(x);
```

结果：

- `input % x`

错误：

- `x == 0`
- 任一参与值不可解释为整数

#### `min`

语法：

```scala
input |> min(x);
```

结果：

- 两者中的较小值

#### `max`

语法：

```scala
input |> max(x);
```

结果：

- 两者中的较大值

#### `eq`

语法：

```scala
input |> eq(x);
```

结果：

- 返回布尔值，表示相等

#### `ne`

语法：

```scala
input |> ne(x);
```

结果：

- 返回布尔值，表示不相等

#### `gt`

语法：

```scala
input |> gt(x);
```

结果：

- 返回布尔值，表示 `input > x`

#### `gte`

语法：

```scala
input |> gte(x);
```

结果：

- 返回布尔值，表示 `input >= x`

#### `lt`

语法：

```scala
input |> lt(x);
```

结果：

- 返回布尔值，表示 `input < x`

#### `lte`

语法：

```scala
input |> lte(x);
```

结果：

- 返回布尔值，表示 `input <= x`

#### `not`

语法：

```scala
input |> not;
```

结果：

- 返回 `!truthy(input)`

#### `default`

语法：

```scala
input |> default(x);
```

结果：

- 如果 `input` 是 `nil`，返回 `x`
- 否则返回 `input`

#### `choose`

语法：

```scala
input |> choose(a, b);
```

结果：

- 如果 `input` 为真，返回 `a`
- 否则返回 `b`

### 12.5 字符串阶段

#### `trim`

语法：

```scala
input |> trim;
```

输入要求：

- `input` 必须为字符串

结果：

- 去除首尾空白后的字符串

#### `upper`

语法：

```scala
input |> upper;
```

结果：

- 大写字符串

#### `to_upper`

语法：

```scala
input |> to_upper;
```

别名：

- `upper`

#### `lower`

语法：

```scala
input |> lower;
```

结果：

- 小写字符串

#### `to_lower`

语法：

```scala
input |> to_lower;
```

别名：

- `lower`

#### `concat`

语法：

```scala
input |> concat(arg1, arg2, ...);
```

结果：

- 将 `input` 与参数列表按字符串表示拼接后的字符串

说明：

- 参数数量可以为零或多个

示例：

```scala
"world" |> concat("hello ", _) |> emit;
```

#### `substring`

语法：

```scala
input |> substring(start, length);
```

输入要求：

- `input` 必须为字符串

参数：

- `start`：整数
- `length`：整数

结果：

- 返回子串

边界行为：

- `start < 0` 返回空字符串
- `length < 0` 返回空字符串
- `start` 越界返回空字符串

#### `split`

语法：

```scala
input |> split(sep);
```

输入要求：

- `input` 必须为字符串
- `sep` 必须为字符串

结果：

- 返回字符串数组

#### `starts_with`

语法：

```scala
input |> starts_with(prefix);
```

输入要求：

- `input` 必须为字符串
- `prefix` 必须为字符串

结果：

- 返回布尔值，表示 `input` 是否以前缀 `prefix` 开头

#### `ends_with`

语法：

```scala
input |> ends_with(suffix);
```

输入要求：

- `input` 必须为字符串
- `suffix` 必须为字符串

结果：

- 返回布尔值，表示 `input` 是否以后缀 `suffix` 结尾

#### `replace`

语法：

```scala
input |> replace(from, to);
```

输入要求：

- `input` 必须为字符串
- `from` 必须为字符串
- `to` 必须为字符串

结果：

- 返回替换后的字符串

说明：

- 会替换所有非重叠匹配项

错误：

- `from` 为空字符串

#### `join`

语法：

```scala
input |> join(sep);
```

输入要求：

- `input` 必须为数组
- `sep` 必须为字符串

结果：

- 返回拼接后的字符串

### 12.6 复合值阶段

#### `size`

语法：

```scala
input |> size;
```

输入要求：

- `input` 必须为字符串、数组、字典或对象

结果：

- 返回元素个数或长度

#### `count`

语法：

```scala
input |> count;
```

别名：

- `size`

#### `append`

语法：

```scala
input |> append(x);
```

输入要求：

- `input` 必须为数组

结果：

- 返回追加元素后的新数组

#### `push`

语法：

```scala
input |> push(x);
```

别名：

- `append`

#### `prepend`

语法：

```scala
input |> prepend(x);
```

输入要求：

- `input` 必须为数组

结果：

- 返回头部插入元素后的新数组

#### `contains`

语法：

```scala
input |> contains(x);
```

输入要求：

- `input` 必须为字符串、数组、字典或对象

结果：

- 字符串：检查是否包含子串 `x`
- 数组：检查是否存在与 `x` 相等的元素
- 字典：检查是否存在键 `x`
- 对象：检查是否存在字段名 `x`

说明：

- 对字符串、字典和对象，参数 `x` 必须为字符串

#### `has`

语法：

```scala
input |> has(x);
```

别名：

- `contains`

#### `slice`

语法：

```scala
input |> slice(start, length);
```

输入要求：

- `input` 必须为字符串或数组

参数：

- `start`：整数
- `length`：整数

结果：

- 字符串：返回子串
- 数组：返回子数组

边界行为：

- `start < 0` 返回空值容器
- `length < 0` 返回空值容器
- `start` 越界返回空值容器

说明：

- 对字符串，空值容器为 `""`
- 对数组，空值容器为 `[]`

#### `reverse`

语法：

```scala
input |> reverse;
```

输入要求：

- `input` 必须为字符串或数组

结果：

- 返回逆序后的字符串或数组

#### `index_of`

语法：

```scala
input |> index_of(x);
```

输入要求：

- `input` 必须为字符串或数组

结果：

- 字符串：返回子串第一次出现的位置
- 数组：返回首个相等元素的下标
- 未找到时返回 `-1`

说明：

- 对字符串，参数 `x` 必须为字符串

#### `repeat`

语法：

```scala
input |> repeat(count);
```

输入要求：

- `input` 必须为字符串或数组

参数：

- `count`：非负整数

结果：

- 字符串：返回重复拼接后的字符串
- 数组：返回重复展开后的数组

错误：

- `count < 0`

#### `take`

语法：

```scala
input |> take(count);
```

输入要求：

- `input` 必须为字符串或数组

参数：

- `count`：非负整数

结果：

- 字符串：返回前 `count` 个字符
- 数组：返回前 `count` 个元素

说明：

- `count` 大于长度时，返回完整输入

错误：

- `count < 0`

#### `skip`

语法：

```scala
input |> skip(count);
```

输入要求：

- `input` 必须为字符串或数组

参数：

- `count`：非负整数

结果：

- 字符串：跳过前 `count` 个字符后的剩余部分
- 数组：跳过前 `count` 个元素后的剩余部分

说明：

- `count` 大于长度时，返回空字符串或空数组

错误：

- `count < 0`

#### `distinct`

语法：

```scala
input |> distinct;
```

输入要求：

- `input` 必须为数组

结果：

- 返回去重后的新数组

说明：

- 保留每个值第一次出现时的顺序
- 相等性判断使用运行时值相等语义

#### `sort`

语法：

```scala
input |> sort;
```

输入要求：

- `input` 必须为数组

结果：

- 返回升序排序后的新数组

限制：

- 当前仅支持 `int`、`string`、`bool` 数组
- 数组中所有元素类型必须一致

#### `sort_desc`

语法：

```scala
input |> sort_desc;
```

输入要求：

- `input` 必须为数组

结果：

- 返回降序排序后的新数组

限制：

- 当前仅支持 `int`、`string`、`bool` 数组
- 数组中所有元素类型必须一致

#### `chunk`

语法：

```scala
input |> chunk(size);
```

输入要求：

- `input` 必须为字符串或数组

参数：

- `size`：正整数

结果：

- 字符串：返回由子串组成的数组
- 数组：返回由子数组组成的数组

说明：

- 最后一组长度可以小于 `size`

错误：

- `size <= 0`

#### `zip`

语法：

```scala
input |> zip(other);
```

输入要求：

- `input` 必须为数组
- `other` 必须为数组

结果：

- 返回二维数组
- 每一项都是 `[left, right]` 形式的两元素数组

说明：

- 结果长度取两边数组长度的较小值

#### `sum`

语法：

```scala
input |> sum;
```

输入要求：

- `input` 必须为整数数组

结果：

- 返回数组元素之和

错误：

- 任一元素不是整数

#### `flatten`

语法：

```scala
input |> flatten;
```

输入要求：

- `input` 必须为数组，且每个元素也必须是数组

结果：

- 按顺序展开一层，返回新数组

说明：

- 当前实现只做一层展开，不会递归展开更深层级

#### `get`

语法：

```scala
input |> get(key);
```

结果：

- 数组：按整数下标读取
- 字符串：按整数下标读取单字符字符串
- 字典：按字符串键读取
- 对象：按字段名读取

说明：

- 对于字典和对象，`key` 推荐直接写成裸标识符，例如 `get(name)`

#### `field`

语法：

```scala
input |> field(key);
```

别名：

- `get`

#### `at`

语法：

```scala
input |> at(index);
```

别名：

- `get`

#### `set`

语法：

```scala
input |> set(key, value);
```

输入要求：

- `input` 必须为字典或对象

结果：

- 字典：返回写入后的新字典
- 对象：修改对象字段并返回对象

说明：

- `key` 对于字典和对象是字符串键名，推荐直接写成裸标识符，例如 `set(score, 95)`

#### `keys`

语法：

```scala
input |> keys;
```

输入要求：

- `input` 必须为字典或对象

结果：

- 返回按键名排序的字符串数组

#### `values`

语法：

```scala
input |> values;
```

输入要求：

- `input` 必须为字典或对象

结果：

- 返回与排序后键顺序对应的值数组

#### `entries`

语法：

```scala
input |> entries;
```

输入要求：

- `input` 必须为字典或对象

结果：

- 返回数组
- 数组中的每一项都是形如 `{key: ..., value: ...}` 的字典

说明：

- 输出顺序按键名排序

#### `pick`

语法：

```scala
input |> pick(key1, key2, ...);
```

输入要求：

- `input` 必须为字典或对象

参数：

- 一个或多个字符串键名，推荐直接写成裸标识符

结果：

- 返回新字典，仅保留指定键

说明：

- 不存在的键会被忽略
- 即使输入是对象，结果也统一为 `dict`

#### `omit`

语法：

```scala
input |> omit(key1, key2, ...);
```

输入要求：

- `input` 必须为字典或对象

参数：

- 一个或多个字符串键名，推荐直接写成裸标识符

结果：

- 返回新字典，移除指定键

说明：

- 不存在的键会被忽略
- 即使输入是对象，结果也统一为 `dict`

#### `merge`

语法：

```scala
input |> merge(other);
```

输入要求：

- `input` 必须为字典或对象
- `other` 必须为字典或对象

结果：

- 返回合并后的新字典

说明：

- 当键冲突时，`other` 中的值覆盖 `input` 中的值
- 即使输入包含对象，结果也统一为 `dict`

#### `rename`

语法：

```scala
input |> rename(from, to);
```

输入要求：

- `input` 必须为字典或对象

参数：

- `from`、`to` 都是字符串键名，推荐直接写成裸标识符

结果：

- 返回重命名后的新字典

说明：

- 若 `from` 不存在，则结果保持原样
- 若 `from` 与 `to` 相同，则结果保持原样
- 即使输入是对象，结果也统一为 `dict`

#### `evolve`

语法：

```scala
input |> evolve(key, callable, ...);
```

输入要求：

- `input` 可以是字典、对象、字典数组或对象数组

参数：

- `key` 是字符串键名，推荐直接写成裸标识符
- `callable` 是可调用对象，可以是名字，也可以是 `pipe` / 组合子值

结果：

- 单条记录输入时，返回更新后的新字典
- 记录数组输入时，返回更新后的新字典数组

说明：

- callable 接收到的是该字段当前值
- 即使输入是对象，结果也统一转换为 `dict`

#### `derive`

语法：

```scala
input |> derive(key, callable, ...);
```

输入要求：

- `input` 可以是字典、对象、字典数组或对象数组

参数：

- `key` 是字符串键名，推荐直接写成裸标识符
- `callable` 是可调用对象，可以是名字，也可以是 `pipe` / 组合子值

结果：

- 单条记录输入时，返回补充字段后的新字典
- 记录数组输入时，返回补充字段后的新字典数组

说明：

- callable 接收到的是整条记录，而不是单个字段值
- 如果 `key` 已存在，则新结果会覆盖原字段
- 即使输入是对象，结果也统一转换为 `dict`

#### `head`

语法：

```scala
input |> head;
```

输入要求：

- `input` 必须为数组或字符串

结果：

- 数组：首元素
- 字符串：首字符字符串
- 空输入：`nil`

#### `last`

语法：

```scala
input |> last;
```

输入要求：

- `input` 必须为数组或字符串

结果：

- 数组：尾元素
- 字符串：尾字符字符串
- 空输入：`nil`

### 12.7 集合式管道阶段

本节中的 `callable` 参数统一支持：

- stage 名，例如 `upper`
- 普通 callable 名，例如 `type_of`
- `pipe` 或组合子生成的值，例如 `bind(add, 10)`、`chain(trim, upper)`

#### `tap`

语法：

```scala
input |> tap(callable, ...);
```

输入要求：

- 任意输入都可以使用

参数：

- 第一个参数必须是可调用对象，可以是名字，也可以是 `pipe` / 组合子值
- 后续参数会作为额外参数传递

结果：

- 调用目标会收到当前管道值
- 当前阶段最终返回原始输入，不改变管道内容

说明：

- 适合在长管道中做调试、打印或副作用处理

#### `map`

语法：

```scala
input |> map(callable, ...);
```

输入要求：

- `input` 必须为数组

参数：

- 第一个参数必须是可调用对象，可以是名字，也可以是 `pipe` / 组合子值
- 后续参数会作为额外参数传递

结果：

- 返回映射后的新数组

#### `flat_map`

语法：

```scala
input |> flat_map(callable, ...);
```

输入要求：

- `input` 必须为数组

参数：

- 第一个参数必须是可调用对象，可以是名字，也可以是 `pipe` / 组合子值
- 后续参数会作为额外参数传递

结果：

- 对每个元素调用目标
- 目标返回的数组会被顺序展开并拼接成一个新数组

错误：

- 调用结果不是数组

#### `filter`

语法：

```scala
input |> filter(callable, ...);
```

输入要求：

- `input` 必须为数组

结果：

- 对每个元素调用目标
- 仅保留结果为真的元素

#### `each`

语法：

```scala
input |> each(callable, ...);
```

输入要求：

- `input` 必须为数组

结果：

- 对每个元素执行调用
- 返回原数组

#### `reduce`

语法：

```scala
input |> reduce(callable, initial, ...);
```

输入要求：

- `input` 必须为数组

参数：

- `callable`：可调用对象，可以是名字，也可以是 `pipe` / 组合子值
- `initial`：初始累积值

结果：

- 返回折叠后的单个值

#### `find`

语法：

```scala
input |> find(callable, ...);
```

输入要求：

- `input` 必须为数组

结果：

- 返回第一个使调用结果为真的原始元素
- 若不存在匹配项，返回 `nil`

#### `all`

语法：

```scala
input |> all(callable, ...);
```

输入要求：

- `input` 必须为数组

结果：

- 当所有元素的调用结果都为真时返回 `true`
- 空数组返回 `true`

#### `group_by`

语法：

```scala
input |> group_by(callable, ...);
```

输入要求：

- `input` 必须为数组

参数：

- 第一个参数必须是可调用对象，可以是名字，也可以是 `pipe` 值
- 后续参数会作为额外参数传递

结果：

- 返回字典
- 字典键来自调用结果
- 每个键对应的值是原始元素数组

说明：

- 调用结果必须为字符串
- 每组内部仍保持输入中的原始顺序

#### `index_by`

语法：

```scala
input |> index_by(key);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符

结果：

- 返回字典
- 字典键来自每一项的指定字段值
- 字典值是对应的原始元素

说明：

- 指定字段值必须为字符串
- 若存在重复键，以后出现的元素覆盖之前的元素

#### `count_by`

语法：

```scala
input |> count_by(key);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符

结果：

- 返回字典
- 字典键来自每一项的指定字段值
- 字典值是对应键出现的次数

说明：

- 指定字段值必须为字符串

#### `sort_by`

语法：

```scala
input |> sort_by(key);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符

结果：

- 返回按指定字段升序排序的新数组

说明：

- 指定字段必须全部存在
- 字段类型必须一致，且只能为 `int`、`string`、`bool`

#### `sort_desc_by`

语法：

```scala
input |> sort_desc_by(key);
```

输入要求：

- 与 `sort_by` 相同

结果：

- 返回按指定字段降序排序的新数组

#### `distinct_by`

语法：

```scala
input |> distinct_by(key);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符

结果：

- 返回去重后的原始元素数组

说明：

- 按指定字段值判断重复
- 保留第一次出现的元素

#### `sum_by`

语法：

```scala
input |> sum_by(key);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符

结果：

- 返回整数字段求和结果

说明：

- 指定字段值必须为 `int`

#### `pluck`

语法：

```scala
input |> pluck(key);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符

结果：

- 返回由同名字段值组成的新数组

说明：

- 若某项缺失该键，则对应位置返回 `nil`

#### `where`

语法：

```scala
input |> where(key, value);
```

输入要求：

- `input` 必须为数组
- 数组中的每一项都必须为字典或对象

参数：

- `key` 是字符串键名，推荐直接写成裸标识符
- `value` 是用于比较的目标值，可直接写裸标识符，也可写普通表达式

结果：

- 返回所有满足“字段值等于 `value`”条件的原始元素数组

#### `any`

语法：

```scala
input |> any(callable, ...);
```

输入要求：

- `input` 必须为数组

结果：

- 当至少一个元素的调用结果为真时返回 `true`
- 空数组返回 `false`

### 12.8 类型转换

#### `str`

已在 [12.2 通用可调用对象](#122-通用可调用对象) 中定义。

#### `int`

已在 [12.2 通用可调用对象](#122-通用可调用对象) 中定义。

#### `bool`

已在 [12.2 通用可调用对象](#122-通用可调用对象) 中定义。

## 13. 内建名称索引

按字母排序：

- `add`
- `append`
- `any`
- `at`
- `bool`
- `chunk`
- `choose`
- `concat`
- `contains`
- `count`
- `default`
- `derive`
- `distinct`
- `div`
- `drop`
- `each`
- `emit`
- `evolve`
- `entries`
- `ends_with`
- `eq`
- `field`
- `filter`
- `find`
- `flat_map`
- `flatten`
- `get`
- `give`
- `group_by`
- `count_by`
- `distinct_by`
- `gt`
- `gte`
- `has`
- `head`
- `index_of`
- `index_by`
- `input`
- `int`
- `into`
- `join`
- `keys`
- `last`
- `lower`
- `lt`
- `lte`
- `map`
- `merge`
- `max`
- `min`
- `mod`
- `mul`
- `ne`
- `not`
- `omit`
- `pick`
- `pluck`
- `prepend`
- `print`
- `push`
- `range`
- `reduce`
- `rename`
- `repeat`
- `replace`
- `reverse`
- `skip`
- `show`
- `size`
- `slice`
- `sort`
- `sort_by`
- `sort_desc`
- `sort_desc_by`
- `split`
- `starts_with`
- `store`
- `str`
- `sum_by`
- `sub`
- `substring`
- `sum`
- `tap`
- `take`
- `to_lower`
- `to_upper`
- `type_of`
- `trim`
- `upper`
- `values`
- `where`
- `zip`

## 14. 错误行为

当前实现中的错误分为三类：

- 词法错误
- 语法错误
- 运行时错误

典型情况：

- 非法转义序列
- 未闭合字符串
- 缺失分号或括号
- 在非管道上下文使用 `_`
- 访问不存在变量
- 参数数量不匹配
- 类型不匹配
- 除零或模零
- 在非循环上下文使用 `break` 或 `continue`
- 在非法上下文使用 `give` / `return`

## 15. 示例

### 15.1 基础管道

```scala
"hello" |> upper |> emit;
```

### 15.2 自定义函数

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;
```

### 15.3 自定义阶段

```scala
stage wrap(left, right) {
    return concat($left, $it, $right);
}

"core" |> wrap("[", "]") |> emit;
```

### 15.4 变量与循环

```scala
let count = 5;
let sum = 0;

while $count > 0 {
    let sum = $sum + $count;
    let count = $count - 1;
}

$sum |> emit;
```

### 15.5 对象与方法

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

let user = User("Alice", 99);
$user.badge() |> emit;
```

### 15.6 占位符与表达式管道

```scala
"Hello, Aethe!" |> substring(_, 7, 3) |> emit;
10 |> _ * 3 + 5 |> emit;
```

### 15.7 新增字符串与容器能力

```scala
"alpha-beta-gamma" |> contains("beta") |> emit;
"Aethe Runtime" |> starts_with("Aethe") |> emit;
"Aethe Runtime" |> replace("Runtime", "Core") |> emit;
[1, 2, 3, 4] |> slice(1, 2) |> emit;
[1, 2, 3] |> reverse |> emit;
type_of({name: "Aethe"}) |> emit;
```

### 15.8 新增集合与聚合能力

```scala
fn ge_three(x) {
    return $x >= 3;
}

[1, 2, 3, 4] |> find(ge_three) |> emit;
[1, 2, 3, 4] |> all(ge_three) |> emit;
[1, 2, 3, 4] |> any(ge_three) |> emit;
[[1, 2], [3], [4, 5]] |> flatten |> emit;
[1, 2, 3, 4] |> sum |> emit;
"banana" |> index_of("na") |> emit;
"ha" |> repeat(3) |> emit;
```

### 15.9 新增排序与截取能力

```scala
"Aethe" |> take(3) |> emit;
"Aethe" |> skip(2) |> emit;
[1, 2, 2, 3, 1] |> distinct |> emit;
[4, 1, 3, 2] |> sort |> emit;
[4, 1, 3, 2] |> sort_desc |> emit;
```

### 15.10 新增结构操作能力

```scala
{name: "Alice"} |> set(score, 95) |> emit;
{name: "Alice", score: 95} |> entries |> emit;
{name: "Alice", score: 95, city: "Shanghai"} |> pick(name, city) |> emit;
{name: "Alice", score: 95, city: "Shanghai"} |> omit(score) |> emit;
{name: "Alice", score: 95} |> merge({score: 99, level: "A"}) |> emit;
{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
[{name: "Alice", role: admin}, {name: "Bob", role: guest}, {name: "Carol", role: admin}]
    |> where(role, admin)
    |> pluck(name)
    |> emit;
[{name: "Alice", role: admin}, {name: "Bob", role: guest}, {name: "Carol", role: admin}]
    |> count_by(role)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}]
    |> index_by(name)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}, {name: "Carol", score: 95}]
    |> distinct_by(score)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}, {name: "Carol", score: 91}]
    |> sort_by(score)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}, {name: "Carol", score: 91}]
    |> sort_desc_by(score)
    |> emit;
[{name: "Alice", score: 95}, {name: "Bob", score: 88}, {name: "Carol", score: 91}]
    |> sum_by(score)
    |> emit;
[{name: " alice "}, {name: "bob"}]
    |> evolve(name, trim)
    |> evolve(name, upper)
    |> emit;
{name: "Alice", score: 95}
    |> derive(kind, type_of)
    |> emit;
```

## 16. 未实现特性

当前版本尚未实现以下特性：

- 静态类型系统
- 类型注解
- 模块与导入系统
- 脚本文件执行模式
