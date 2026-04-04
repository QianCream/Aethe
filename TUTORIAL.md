# Aethe 3 新手教程

这份教程面向从未接触过 Aethe，甚至没有系统学过编程的读者。

这份教程对应 `Aethe 3 / 3.1.0`。目标不是只让你"看懂例子"，而是让你能够独立写出一段可运行的 Aethe 程序，并知道什么时候该使用 `fn`、什么时候该使用 `stream`（兼容 `stage`）、什么时候该使用匿名 `pipe`、什么时候该使用管道组合子、什么时候该写管道表达式。

如果你只打算看一份文档，先读这份教程；如果你已经学会基础语法，再去看 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)。

---

## 1. Aethe 是什么

Aethe 是一门以"管道"为核心的语言。

在大多数语言里，你想对一个值做三件事，代码长这样：

```text
print(to_upper(trim("  hello  ")))
```

要读懂它，你得从最里面读到最外面——先看 `trim`，再看 `to_upper`，最后看 `print`。函数名在前面，数据藏在括号深处。写得越长，脑子里要拆的层数越多。

Aethe 更倾向于这样写：

```scala
"  hello  " |> trim |> upper |> emit;
```

从左到右读就行：

1. 先拿到字符串 `"  hello  "`
2. 交给 `trim`，去掉首尾空白
3. 再交给 `upper`，转成大写
4. 再交给 `emit`，输出到屏幕

这种写法的重点是"数据在流动"。你可以把 `|>` 想象成一条传送带——值从左边进入，每经过一个步骤就被处理一次，最终到达终点。你要关心的只有两件事：什么值进来了，这一步要做什么。

后面你会发现，Aethe 的绝大部分设计都是围绕"让管道更好写"来展开的。函数、阶段、匿名管道、组合子，都是不同粒度的管道步骤。

---

## 2. 把 Aethe 跑起来

### 2.1 构建

Aethe 是一个单文件 C++ 程序。如果你的电脑上装了 C++ 编译器，一行命令就能构建：

```bash
g++ -std=c++11 main.cpp -o aethe
```

这会在当前目录下生成一个叫 `aethe` 的可执行文件。

> 如果你用的是 Mac，系统自带的 `clang++` 也可以：
> ```bash
> clang++ -std=c++11 main.cpp -o aethe
> ```

> 如果你用 CLion 或 CMake，直接打开项目编译即可，`CMakeLists.txt` 已经配好了。

### 2.2 运行

```bash
./aethe
```

默认启动后会进入一个全屏的终端编辑器（IDE 模式）。你可以在里面写代码、运行代码、打开和保存文件。

IDE 最常用的快捷键：

| 快捷键 | 功能 |
|--------|------|
| `Ctrl-R` | 运行当前代码 |
| `Ctrl-S` | 保存文件 |
| `Ctrl-O` | 打开文件 |
| `Ctrl-Q` | 退出 |
| `Ctrl-C` | 复制选区（无选区时复制整行） |
| `Ctrl-X` | 剪切选区（无选区时剪切整行） |
| `Ctrl-V` | 粘贴 |

IDE 还支持语法高亮、`Shift + 方向键` 选区、多行编辑、粘贴大段代码。你可以把它当成一个简单的编辑器来用。

### 2.3 REPL 模式

如果你不想进 IDE，而是想一步一步地试验，可以用旧的 REPL：

```bash
./aethe --repl
```

启动后会看到提示符：

```text
>>>
```

这表示你正在输入一个新的缓冲区。

REPL 的规则非常重要，请牢记：

- **你输入代码后，不会立刻运行。**
- 只有单独输入一行 `run`，当前缓冲区的所有代码才会统一执行。
- `>>>` 表示新缓冲区开始。
- `...>` 表示当前代码还没输完，继续补下一行。

示例：

```text
>>> "hello" |> emit;
...> run
hello
>>>
```

退出方式：

- 输入 `exit`
- 输入 `quit`
- 按 `Ctrl+D`

### 2.4 脚本模式

如果你已经写好了一个 `.ae` 文件，想直接运行而不进 IDE：

```bash
./aethe --run example.ae
```

---

## 3. 写第一个程序

不管你在 IDE 里还是在 REPL 里，输入下面这行：

```scala
"hello, Aethe" |> emit;
```

如果你在 REPL 里，还需要输入 `run`：

```text
>>> "hello, Aethe" |> emit;
...> run
hello, Aethe
>>>
```

如果你在 IDE 里，按 `Ctrl-R` 就行。

你已经学会了三件事：

- 字符串写在双引号 `"..."` 里
- `|>` 是管道符，表示"把左边的值交给右边"
- `emit` 会把当前值输出到屏幕

再试一个稍微复杂的：

```scala
"  hello  " |> trim |> upper |> emit;
```

输出：

```text
HELLO
```

发生了什么？

1. `"  hello  "` 是一个两端有空格的字符串
2. `|> trim` 把空格去掉了，变成 `"hello"`
3. `|> upper` 把它变成大写，变成 `"HELLO"`
4. `|> emit` 把结果输出

这就是管道。数据从左往右流，每一步做一件事。

---

## 4. 值：程序处理的对象

程序操作的东西叫"值"。Aethe 有这些类型的值：

### 4.1 整数 `int`

```scala
123 |> emit;
-42 |> emit;
0 |> emit;
```

Aethe 当前只支持整数，不支持小数。

### 4.2 布尔值 `bool`

只有两个：`true` 和 `false`。

```scala
true |> emit;
false |> emit;
```

用来表示"是"或"不是"。

### 4.3 字符串 `string`

用双引号包起来的文本：

```scala
"hello" |> emit;
"Aethe Language" |> emit;
"" |> emit;
```

最后一个是空字符串——什么内容都没有，但它仍然是一个字符串。

字符串里可以用转义符号：

- `\n` 表示换行
- `\t` 表示制表符
- `\"` 表示双引号本身
- `\\` 表示反斜杠本身

### 4.4 空值 `nil`

`nil` 表示"没有值"。

```scala
nil |> emit;
```

它在很多场景下会出现，比如从数组里找不到元素时就会返回 `nil`。

### 4.5 数组 `array`

用方括号括起来，元素用逗号分隔：

```scala
[1, 2, 3] |> emit;
["a", "b", "c"] |> emit;
[1, "hello", true, nil] |> emit;
[] |> emit;
```

数组里可以放不同类型的值。空数组写成 `[]`。

### 4.6 字典 `dict`

用花括号，键值对之间用逗号分隔：

```scala
{name: "Alice", score: 95} |> emit;
{} |> emit;
```

注意看——键的写法是裸标识符 `name`，不是字符串 `"name"`。这是 Aethe 的设计，后面会详细解释。

### 4.7 对象 `object`

通过 `type` 定义的结构体实例。后面会专门讲。

### 4.8 可调用值 `callable`

函数、阶段、匿名管道 `pipe` 等都属于 callable。它们可以被调用、传递、组合。后面也会专门讲。

---

## 5. 语句和分号

Aethe 里的大多数语句都要以 `;` 结尾。

```scala
1 |> emit;
2 |> emit;
"hello" |> emit;
```

如果你漏了分号，REPL 会继续等待下一行输入，因为它认为这条语句还没结束。IDE 运行时也会报错。

一个重要的细节：`fn`、`stage`、`type`、`when`、`match`、`while`、`for` 这些带花括号 `{}` 的结构不需要在 `}` 后面加分号。

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;
```

`}` 后面没有 `;`，这是对的。但管道语句 `21 |> double |> emit;` 后面必须有 `;`。

---

## 6. 注释

用 `//` 开头的行是注释，不会被执行：

```scala
// 这是一个注释
"hello" |> emit; // 这也是注释
```

当前不支持 `/* ... */` 多行注释。

---

## 7. 管道：Aethe 最重要的概念

这是 Aethe 的核心，值得花时间好好理解。

### 7.1 基本形式

```scala
值 |> 步骤1 |> 步骤2 |> 步骤3;
```

`|>` 叫管道符。它的意思是"把左边产生的值，送到右边去处理"。

```scala
"  hello  " |> trim |> upper |> emit;
```

可以读成一句话："拿到字符串，去掉空白，转大写，输出。"

### 7.2 养成一个习惯

读 Aethe 代码时，不要先想"函数嵌套"，先想"值怎么从左往右流动"。

这两种写法做的事情完全一样：

```text
// 其他语言的嵌套写法（Aethe 不支持这么写）
print(upper(trim("  hello  ")))

// Aethe 的管道写法
"  hello  " |> trim |> upper |> emit;
```

但管道写法的好处是：你可以从左到右、从上到下读，不需要在脑子里反复嵌套。

### 7.3 管道可以跨行

当管道很长时，每一步占一行会更清楚：

```scala
"  hello world  "
    |> trim
    |> upper
    |> replace("WORLD", "AETHE")
    |> emit;
```

缩进不影响执行，但会让代码更好看。推荐每个 `|>` 对齐。

### 7.4 管道的自动注入

当你这样写：

```scala
21 |> double;
```

如果 `double` 是一个可调用对象（函数、阶段等），左边的 `21` 会自动成为它的第一个参数。也就是说，上面等价于：

```text
double(21)
```

再看一个有参数的例子：

```scala
fn add(a, b) {
    return $a + $b;
}

10 |> add(5) |> emit;
```

这里 `10` 会自动成为 `add` 的第一个参数 `a`，而 `5` 是第二个参数 `b`。所以实际调用的是 `add(10, 5)`，结果是 `15`。

这个规则叫"自动首参注入"——管道左边的值总是自动填到右边函数的第一个参数位置。

### 7.5 占位符 `_`

有时候你不想让输入值总是去第一个参数，这时候用 `_`。

```scala
"Hello, Aethe!" |> substring(_, 7, 5) |> emit;
```

这里 `_` 代表"当前管道输入值"。所以这一步等价于 `substring("Hello, Aethe!", 7, 5)`。

`_` 可以出现多次：

```scala
fn pair(a, b) {
    return concat($a, " & ", $b);
}

"echo" |> pair(_, _) |> emit;
```

输出：

```text
echo & echo
```

两个 `_` 都被替换成了同一个输入值 `"echo"`。

**`_` 不是匿名函数。** 这一点很重要。它只表示"当前管道步骤的输入值"，不能脱离管道使用。

合法：

```scala
10 |> _ * 3 + 5 |> emit;
```

非法：

```scala
_ * 3;  // 错误：_ 只能在管道右侧使用
```

如果你需要匿名可调用对象，请使用后面会讲的 `pipe(...) { ... }`。

### 7.6 裸表达式管道

管道右边不一定非要是一个函数名，也可以直接写表达式：

```scala
10 |> _ * 3 + 5 |> emit;
```

输出：

```text
35
```

这是怎么工作的？

1. `10` 进入管道
2. `_ * 3 + 5` 中的 `_` 被替换成 `10`，所以变成 `10 * 3 + 5 = 35`
3. `emit` 输出 `35`

这在做简单变换时非常方便，不需要专门定义一个函数。

---

## 8. `emit`、`print`、`show`

这三个名字当前是同义词，都做同一件事：把当前值输出到屏幕。

```scala
"hello" |> emit;
"hello" |> print;
"hello" |> show;
```

它们都会输出 `hello`。

一个重要的细节：它们输出之后，会把值继续往后传。

```scala
"hello" |> emit |> upper |> emit;
```

输出：

```text
hello
HELLO
```

第一个 `emit` 输出了 `"hello"`，然后把 `"hello"` 继续往下传；`upper` 把它变成 `"HELLO"`，第二个 `emit` 再输出。

这意味着你可以在管道中间插入 `emit` 来调试中间结果，而不会打断管道的流动。

---

## 9. 变量

变量就是"给一个值起名字"，以后可以通过这个名字再取回来。

### 9.1 定义变量

用 `let`：

```scala
let name = "Alice";
let score = 95;
let is_admin = true;
```

### 9.2 读取变量

读取变量时必须加 `$`：

```scala
$name |> emit;
$score |> emit;
```

输出：

```text
Alice
95
```

### 9.3 裸标识符 vs 变量读取

这是 Aethe 最独特的一个设计，值得特别理解。

在 Aethe 里：

- `name`——这是一个"裸标识符"，它的值就是符号 `name` 本身（类似于一个字符串，但不需要引号）
- `$name`——这是变量读取，表示"去拿 `name` 这个变量里存的值"

两者完全不同。看一个对比：

```scala
let name = "Alice";

name |> emit;   // 输出 name（裸标识符，就是 name 这个符号本身）
$name |> emit;  // 输出 Alice（变量读取，拿到了存储的值）
```

为什么要这样设计？因为 Aethe 的管道里经常需要直接写字段名。比如：

```scala
{name: "Alice", role: admin} |> get(name) |> emit;
```

这里 `get(name)` 的意思是"取 `name` 这个字段"。如果没有裸标识符，你就得写成 `get("name")`，到处补引号。

再看一个更典型的例子：

```scala
[
    {name: "Alice", role: admin},
    {name: "Bob", role: guest},
    {name: "Carol", role: admin}
]
    |> where(role, admin)
    |> pluck(name)
    |> emit;
```

这段代码的意思是：从数组里筛选出 `role` 字段等于 `admin` 的记录，然后取出它们的 `name` 字段。

`role`、`admin`、`name` 全都是裸标识符——它们直接表示字段名和字段值，不需要引号。这让管道代码读起来像自然语言一样流畅。

**记住这条规则：**

- 你想写名字本身（字段名、键名、筛选值） → 直接写 `name`
- 你想读变量里的值 → 写 `$name`

### 9.4 更新变量

变量定义之后可以更新：

```scala
let score = 100;
$score = 120;       // 直接赋值
$score += 5;        // 加 5，现在是 125
$score -= 10;       // 减 10，现在是 115
$score |> emit;
```

输出：

```text
115
```

支持的复合赋值运算：`+=`、`-=`、`*=`、`/=`、`%=`。

### 9.5 更新字段和下标

如果变量存的是字典或对象，你可以直接改里面的字段：

```scala
let user = {name: "Alice", score: 95};
$user.name = "Bob";        // 改 name 字段
$user[score] += 5;         // 给 score 字段加 5
$user |> emit;
```

输出：

```text
{name: Bob, score: 100}
```

`$user.name` 和 `$user[name]` 都可以访问字段。`.` 后面直接跟标识符；`[]` 里面可以放表达式。

### 9.6 `into`：管道风格的变量绑定

除了 `let`，你也可以用管道风格来创建变量：

```scala
100 |> into(score);
```

这和 `let score = 100;` 做的事情一样。

大多数时候 `let` 更直观，但当你已经在一段管道里时，`into` 可以让值继续流动而不打断管道。

---

## 10. 表达式

表达式就是"会得到结果的代码"。

### 10.1 算术运算

```scala
1 + 2 |> emit;        // 3
10 - 3 |> emit;       // 7
4 * 5 |> emit;        // 20
10 / 3 |> emit;       // 3（整数除法，不保留小数）
10 % 3 |> emit;       // 1（取余）
```

运算符优先级和数学一样：先乘除，后加减。

```scala
1 + 2 * 3 |> emit;    // 7，不是 9
(1 + 2) * 3 |> emit;  // 9，括号可以改变优先级
```

### 10.2 比较运算

```scala
3 > 2 |> emit;        // true
3 < 2 |> emit;        // false
3 == 3 |> emit;       // true
3 != 4 |> emit;       // true
3 >= 3 |> emit;       // true
3 <= 2 |> emit;       // false
```

比较运算的结果是 `true` 或 `false`。

### 10.3 逻辑运算

```scala
true && true |> emit;   // true（两个都为真才为真）
true && false |> emit;  // false
true || false |> emit;  // true（至少一个为真就为真）
!true |> emit;          // false（取反）
```

### 10.4 字符串拼接

字符串可以用 `+` 拼接：

```scala
"Hello" + " " + "World" |> emit;
```

输出：

```text
Hello World
```

---

## 11. 条件分支：`when`

`when` 相当于"如果……就……"。

### 11.1 基本写法

```scala
let score = 75;

when $score >= 60 {
    "pass" |> emit;
}
```

输出：

```text
pass
```

### 11.2 加上 `else`

```scala
let score = 45;

when $score >= 60 {
    "pass" |> emit;
} else {
    "fail" |> emit;
}
```

输出：

```text
fail
```

### 11.3 真值规则

在 Aethe 里，下面这些值被视为"假"：

- `false`
- `nil`
- `0`
- `""` 空字符串
- `[]` 空数组
- `{}` 空字典

其他值通常被视为"真"。

这意味着你可以这样写：

```scala
let name = "";

when $name {
    "has name" |> emit;
} else {
    "no name" |> emit;
}
```

因为空字符串是假，所以输出 `no name`。

---

## 12. 多分支：`match`

当你要根据一个值做多种判断时，`match` 比一连串 `when...else` 更清楚。

### 12.1 基本写法

```scala
let grade = "A";

match $grade {
    case "A" {
        "excellent" |> emit;
    }
    case "B" {
        "good" |> emit;
    }
    case "C" {
        "average" |> emit;
    }
    else {
        "keep going" |> emit;
    }
}
```

执行时会从上到下逐个比较。找到匹配的就执行对应的代码块，然后跳出 `match`。如果都不匹配，就执行 `else`。

### 12.2 Wildcard `case _`

`case _` 表示"匹配任何值"：

```scala
let x = 42;

match $x {
    case 1 {
        "one" |> emit;
    }
    case _ {
        "something else" |> emit;
    }
}
```

`case _` 和 `else` 的效果类似，但 `case _` 可以出现在中间位置。

### 12.3 Guard 条件

你可以在 `case` 后面加 `when` 来写额外条件：

```scala
let score = 85;

match $score {
    case _ when $it >= 90 {
        "A" |> emit;
    }
    case _ when $it >= 80 {
        "B" |> emit;
    }
    case _ when $it >= 60 {
        "C" |> emit;
    }
    else {
        "F" |> emit;
    }
}
```

输出：

```text
B
```

注意这里出现了 `$it`——在 `match` 的每个分支里，`$it` 自动绑定到当前被匹配的值。这里 `$it` 就是 `$score` 的值 `85`。

---

## 13. 循环

### 13.1 `while`：只要条件成立，就一直重复

```scala
let n = 5;

while $n > 0 {
    $n |> emit;
    $n -= 1;
}
```

输出：

```text
5
4
3
2
1
```

`while` 每次先检查条件。如果为真，执行代码块，然后回到开头再检查。如果为假，跳出循环。

注意不要写成死循环——如果条件永远为真，程序就会一直跑下去。

### 13.2 `for`：遍历集合

`for` 用于逐个处理集合里的元素。

**遍历数组：**

```scala
for item in [10, 20, 30] {
    $item |> emit;
}
```

输出：

```text
10
20
30
```

每一轮循环，`$item` 会绑定到数组的下一个元素。

**遍历字符串：**

```scala
for ch in "abc" {
    $ch |> emit;
}
```

输出：

```text
a
b
c
```

字符串会被按字符拆开。

**遍历字典：**

```scala
for entry in {name: "Alice", score: 90} {
    $entry.key |> emit;
    $entry.value |> emit;
}
```

遍历字典时，每次拿到的是一个包含 `key` 和 `value` 的小字典。

### 13.3 双变量绑定

如果你既想拿到位置（或键），也想拿到值，可以用两个变量：

```scala
for index, item in [10, 20, 30] {
    [$index, $item] |> emit;
}
```

输出：

```text
[0, 10]
[1, 20]
[2, 30]
```

数组和字符串会给你"索引 + 当前值"；字典和对象会给你"键 + 当前值"：

```scala
for key, value in {name: "Alice", score: 90} {
    [$key, $value] |> emit;
}
```

输出：

```text
[name, Alice]
[score, 90]
```

### 13.4 `break`：提前结束循环

```scala
for item in [1, 2, 3, 4, 5] {
    when $item == 3 {
        break;
    }
    $item |> emit;
}
```

输出：

```text
1
2
```

遇到 `break` 时，整个循环立刻结束，不会继续执行。

### 13.5 `continue`：跳过本轮

```scala
for item in [1, 2, 3, 4, 5] {
    when $item == 3 {
        continue;
    }
    $item |> emit;
}
```

输出：

```text
1
2
4
5
```

遇到 `continue` 时，本轮循环的剩余代码不再执行，直接进入下一轮。

---

## 14. `defer`：离开作用域时执行

`defer` 表示"等当前作用域结束时再执行这段代码"。

```scala
fn demo() {
    defer {
        "leaving demo" |> emit;
    }

    "working" |> emit;
}

demo();
```

输出：

```text
working
leaving demo
```

虽然 `defer` 写在前面，但它登记的代码会等到函数结束时才执行。这适合做收尾清理工作。

---

## 15. 函数：`fn`

`fn` 用来定义可复用的逻辑。

### 15.1 基本写法

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;
```

输出：

```text
42
```

拆解：

- `fn double(x)` 定义了一个叫 `double` 的函数，接受一个参数 `x`
- 函数体里用 `$x` 读取参数值
- `return` 把结果返回出去

### 15.2 多个参数

```scala
fn add(a, b) {
    return $a + $b;
}

add(3, 4) |> emit;         // 普通调用方式
10 |> add(5) |> emit;      // 管道调用方式
```

两种调用方式都可以。管道调用时，左边的值自动填到第一个参数。

### 15.3 `return` 和 `give`

这两个当前是同义词。下面两种写法等价：

```scala
fn double(x) {
    return $x * 2;
}
```

```scala
fn double(x) {
    $x * 2 |> give;
}
```

`give` 更有管道味——你可以把 `give` 想象成管道的终点，"把这个值交出去"。但 `return` 更通用，大多数人更熟悉。

### 15.4 注意：别忘了 return

下面这段代码有一个常见错误：

```scala
fn double(x) {
    $x * 2;
}
```

它计算了 `$x * 2`，但没有显式返回。如果你需要这个函数有返回值，必须写 `return`：

```scala
fn double(x) {
    return $x * 2;
}
```

---

## 16. 管道阶段：`stream`（兼容 `stage`）

`stream` 也是一种可复用逻辑，但它专门为管道设计（旧写法 `stage` 仍可用）。

### 16.1 基本写法

```scala
stream shout(suffix) {
    return $it |> upper |> concat($suffix);
}

"hello" |> shout("!!!") |> emit;
```

输出：

```text
HELLO!!!
```

### 16.2 `$it`：当前输入

`stream`（或 `stage`）和 `fn` 最大的区别在于：`stream` 里会自动提供一个特殊变量 `$it`，表示"流进这个阶段的输入值"。

```scala
stream double_and_show() {
    let result = $it * 2;
    $result |> emit;
    return $result;
}

21 |> double_and_show;
```

输出：

```text
42
```

`$it` 就是左边传进来的 `21`。

### 16.3 什么时候用 `fn`，什么时候用 `stream`

最简单的记法：

- **`fn`**：我在写普通函数——参数地位平等，逻辑更像普通运算。
- **`stream`**（兼容 `stage`）：我在写管道步骤——有一个主要的输入值在流动，这段代码是对它的处理。

举个例子：

```scala
// 这更适合 fn——两个参数地位平等
fn add(a, b) {
    return $a + $b;
}

// 这更适合 stream/stage——明显在处理输入值
stream shout(suffix) {
    return $it |> upper |> concat($suffix);
}
```

如果你拿不准，用 `fn` 就行。`fn` 也可以挂在管道后面。

---

## 17. 匿名管道：`pipe`

有时候你只是临时需要一小段可调用逻辑，不想专门起名字。`pipe` 就是为这个场景设计的。

### 17.1 基本写法

```scala
let double = pipe(x) {
    return $x * 2;
};

21 |> $double |> emit;
```

输出：

```text
42
```

`pipe(x) { ... }` 创建一个匿名的可调用对象。你可以把它存进变量，也可以直接内联使用。

### 17.2 内联使用

`pipe` 最常见的用法是直接传给 `map`、`filter`、`reduce` 等高阶阶段：

```scala
[1, 2, 3, 4]
    |> map(pipe(x) { return $x * 10; })
    |> emit;
```

输出：

```text
[10, 20, 30, 40]
```

### 17.3 多个参数

```scala
[1, 2, 3, 4]
    |> reduce(pipe(acc, item) { return $acc + $item; }, 0)
    |> emit;
```

输出：

```text
10
```

`reduce` 接受一个两参数的 callable：第一个是累积值，第二个是当前元素。

### 17.4 闭包

`pipe` 可以捕获它定义时可见的变量：

```scala
let factor = 10;
let multiply = pipe(x) {
    return $x * $factor;
};

3 |> $multiply |> emit;
```

输出：

```text
30
```

`$factor` 在 `pipe` 外面定义，但 `pipe` 里面可以读到它。这叫闭包。

### 17.5 `fn` vs `pipe` vs `stage` 该选哪个？

- **`fn`**：有名字，可复用，像普通函数。大多数时候用这个。
- **`stage`**：有名字，天然挂在管道后面，自带 `$it`。适合写管道步骤。
- **`pipe`**：没名字（匿名），可以存进变量，适合临时逻辑。传给 `map`/`filter` 时最方便。

---

## 18. 管道组合子：把步骤拼起来

Aethe 提供四个组合子，它们的作用是"把已有的步骤组装成新的可调用对象"。

### 18.1 `bind`：给已有步骤补参数

```scala
[1, 2, 3] |> map(bind(add, 10)) |> emit;
```

输出：

```text
[11, 12, 13]
```

`bind(add, 10)` 创建了一个新的 callable：给它一个输入值 `x`，它会调用 `add(x, 10)`。

这比写一个完整的 `pipe(x) { return $x + 10; }` 简洁得多。

```scala
// 这两种写法等价
[1, 2, 3] |> map(bind(add, 10)) |> emit;
[1, 2, 3] |> map(pipe(x) { return $x + 10; }) |> emit;
```

另一个典型用法——从记录数组里取字段：

```scala
[{name: "Alice"}, {name: "Bob"}] |> map(bind(get, name)) |> emit;
```

输出：

```text
[Alice, Bob]
```

### 18.2 `chain`：串联多个步骤

```scala
let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;
```

输出：

```text
HELLO!
```

`chain(trim, upper, bind(concat, "!"))` 把三个步骤串成一条"管道模板"。之后你可以像使用普通函数一样使用 `$loud`。

这很适合当你发现自己反复写同样一串管道步骤时——打包成 `chain`，复用它。

### 18.3 `branch`：一个输入，多路结果

```scala
"Aethe" |> branch(type_of, size, upper) |> emit;
```

输出：

```text
[string, 5, AETHE]
```

`branch` 会把同一个输入分别送给每个步骤，然后把所有结果收集成数组。

### 18.4 `guard`：条件分流

```scala
5 |> guard(pipe(x) { return $x > 3; }, bind(add, 100), bind(sub, 100)) |> emit;
```

输出：

```text
105
```

`guard(test, on_true, on_false)` 的意思是：

1. 先把输入值送给 `test`
2. 如果结果为真，把输入值送给 `on_true`
3. 如果结果为假，把输入值送给 `on_false`

因为 `5 > 3` 为真，所以执行 `add(5, 100)` 得到 `105`。

### 18.5 总结

- **`pipe(x) { ... }`**——你亲自写逻辑体
- **`bind(f, arg)`**——给已有步骤补参数
- **`chain(f1, f2, ...)`**——把多个步骤串成一条路线
- **`branch(f1, f2, ...)`**——一个输入分流到多路
- **`guard(test, on_true, on_false)`**——条件二选一

---

## 19. 数组

数组是 Aethe 里最重要的复合值类型之一。

### 19.1 创建

```scala
[1, 2, 3]
["a", "b", "c"]
[1, "hello", true, nil]
[]
```

### 19.2 读取元素

用下标访问（从 0 开始）：

```scala
[10, 20, 30][0] |> emit;    // 10
[10, 20, 30][2] |> emit;    // 30
```

或者用 `get`：

```scala
[10, 20, 30] |> get(1) |> emit;    // 20
```

或者取首尾：

```scala
[10, 20, 30] |> head |> emit;    // 10
[10, 20, 30] |> last |> emit;    // 30
```

### 19.3 修改

**替换指定位置：**

```scala
[10, 20, 30] |> set(1, 99) |> emit;    // [10, 99, 30]
```

**用 callable 改写指定位置：**

```scala
[10, 20, 30] |> update(1, bind(add, 5)) |> emit;    // [10, 25, 30]
```

`update` 会先读出下标 1 的值（`20`），送给 `bind(add, 5)`（加 5 得到 `25`），再把结果写回。

**在指定位置插入：**

```scala
[10, 20, 30] |> insert(1, 15) |> emit;    // [10, 15, 20, 30]
```

**删除指定位置：**

```scala
[10, 20, 30] |> remove(0) |> emit;    // [20, 30]
```

**尾部追加 / 头部插入：**

```scala
[1, 2, 3] |> append(4) |> emit;     // [1, 2, 3, 4]
[1, 2, 3] |> prepend(0) |> emit;    // [0, 1, 2, 3]
```

### 19.4 查询

```scala
[1, 2, 3] |> size |> emit;              // 3
[1, 2, 3] |> contains(2) |> emit;       // true
[10, 20, 30] |> index_of(20) |> emit;   // 1（找不到返回 -1）
```

### 19.5 变换

```scala
[3, 1, 2] |> sort |> emit;           // [1, 2, 3]
[3, 1, 2] |> sort_desc |> emit;      // [3, 2, 1]
[1, 2, 2, 3] |> distinct |> emit;    // [1, 2, 3]
[1, 2, 3] |> reverse |> emit;        // [3, 2, 1]
```

### 19.6 截取

```scala
[1, 2, 3, 4, 5] |> take(3) |> emit;        // [1, 2, 3]
[1, 2, 3, 4, 5] |> skip(2) |> emit;        // [3, 4, 5]
[1, 2, 3, 4, 5] |> slice(1, 3) |> emit;    // [2, 3, 4]
```

- `take(n)` 取前 n 个
- `skip(n)` 跳过前 n 个
- `slice(start, length)` 从 start 开始取 length 个

### 19.7 其他

```scala
[1, 2, 3] |> repeat(2) |> emit;              // [1, 2, 3, 1, 2, 3]
[[1, 2], [3], [4, 5]] |> flatten |> emit;     // [1, 2, 3, 4, 5]
[1, 2, 3, 4] |> sum |> emit;                  // 10
[1, 2, 3, 4] |> window(2) |> emit;            // [[1, 2], [2, 3], [3, 4]]
[1, 2, 3, 4] |> chunk(2) |> emit;             // [[1, 2], [3, 4]]
["A", "B", "C"] |> zip([1, 2]) |> emit;       // [[A, 1], [B, 2]]
```

- `flatten` 把子数组展开一层
- `sum` 对整数数组求和
- `window(n)` 生成连续滑动窗口
- `chunk(n)` 按固定长度切段
- `zip(other)` 把两路数据并排配对（取较短的长度）

---

## 20. 字典

字典是"键到值"的映射。

### 20.1 创建

```scala
{name: "Alice", score: 95}
{}
```

键是裸标识符，不需要引号。

### 20.2 读取

```scala
{name: "Alice", score: 95} |> get(name) |> emit;       // Alice
{name: "Alice", score: 95}[score] |> emit;              // 95
{name: "Alice", score: 95} |> keys |> emit;             // [name, score]
{name: "Alice", score: 95} |> values |> emit;           // [Alice, 95]
```

### 20.3 写入和修改

```scala
{name: "Alice"} |> set(score, 95) |> emit;
// {name: Alice, score: 95}

{name: "Alice", score: 95} |> update(score, bind(add, 5)) |> emit;
// {name: Alice, score: 100}
```

`update` 和数组一样：先读出旧值，送给 callable，把结果写回。

### 20.4 删除

```scala
{name: "Alice", score: 95} |> remove(score) |> emit;
// {name: Alice}
```

### 20.5 展开与筛选

```scala
{name: "Alice", score: 95, city: "Shanghai"} |> entries |> emit;
// [{key: name, value: Alice}, {key: score, value: 95}, {key: city, value: Shanghai}]

{name: "Alice", score: 95, city: "Shanghai"} |> pick(name, city) |> emit;
// {name: Alice, city: Shanghai}

{name: "Alice", score: 95, city: "Shanghai"} |> omit(score) |> emit;
// {name: Alice, city: Shanghai}
```

- `entries` 把字典展开成键值对数组
- `pick(...)` 只保留指定的键
- `omit(...)` 移除指定的键

### 20.6 合并与重命名

```scala
{name: "Alice", score: 95} |> merge({score: 99, level: "A"}) |> emit;
// {name: Alice, score: 99, level: A}

{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
// {name: Alice, total_score: 95}
```

`merge` 在键冲突时以右侧为准。

### 20.7 判断键是否存在

```scala
{name: "Alice"} |> has(name) |> emit;       // true
{name: "Alice"} |> contains(score) |> emit;  // false
```

---

## 21. 字符串处理

Aethe 提供了丰富的字符串阶段。

### 21.1 大小写与清理

```scala
"  hello  " |> trim |> emit;       // hello
"hello" |> upper |> emit;          // HELLO
"HELLO" |> lower |> emit;          // hello
```

`trim` 去掉首尾空白，`upper` 转大写，`lower` 转小写。`to_upper` 和 `to_lower` 是它们的别名。

### 21.2 拼接

```scala
"hello" |> concat(", ", "world") |> emit;
```

输出：

```text
hello, world
```

### 21.3 截取与切片

```scala
"Hello, Aethe!" |> substring(_, 7, 5) |> emit;   // Aethe
"abcdef" |> take(3) |> emit;                       // abc
"abcdef" |> skip(2) |> emit;                       // cdef
"abcdef" |> slice(1, 3) |> emit;                   // bcd
```

### 21.4 查找与判断

```scala
"Hello, Aethe!" |> contains("Aethe") |> emit;        // true
"Hello, Aethe!" |> starts_with("Hello") |> emit;      // true
"Hello, Aethe!" |> ends_with("!") |> emit;            // true
"Hello, Aethe!" |> index_of("Aethe") |> emit;         // 7
```

### 21.5 替换

```scala
"hello world" |> replace("world", "Aethe") |> emit;
```

输出：

```text
hello Aethe
```

`replace` 会替换所有匹配项。

### 21.6 拆分与组合

```scala
"a,b,c" |> split(",") |> emit;    // [a, b, c]
["a", "b", "c"] |> join(", ") |> emit;    // a, b, c
```

### 21.7 其他

```scala
"abc" |> size |> emit;              // 3
"abc" |> reverse |> emit;           // cba
"abc" |> repeat(3) |> emit;         // abcabcabc
"abcdef" |> window(3) |> emit;      // [abc, bcd, cde, def]
"abcdef" |> chunk(2) |> emit;       // [ab, cd, ef]
```

### 21.8 修改单个字符

```scala
"hello" |> set(0, "H") |> emit;               // Hello
"hello" |> update(0, upper) |> emit;           // Hello
"hello" |> insert(5, "!") |> emit;             // hello!
"hello" |> remove(4) |> emit;                  // hell
```

`update` 对字符串也有效：先取出指定位置的字符，送给 callable 处理，再写回。

---

## 22. 集合式管道：批量处理数组

这是 Aethe 最有"管道味"的部分。这些阶段让你可以像描述数据流一样处理整个数组。

### 22.1 `map`：逐个变换

把数组的每个元素都变成另一个值。

```scala
fn double(x) {
    return $x * 2;
}

[1, 2, 3] |> map(double) |> emit;
```

输出：

```text
[2, 4, 6]
```

每个元素都被送进 `double`，结果收集成新数组。

### 22.2 `filter`：筛选

只保留满足条件的元素。

```scala
fn is_big(x) {
    return $x >= 3;
}

[1, 2, 3, 4, 5] |> filter(is_big) |> emit;
```

输出：

```text
[3, 4, 5]
```

每个元素被送进 `is_big`，返回 `true` 的留下，返回 `false` 的丢掉。

### 22.3 `each`：逐个执行，但保留原数组

```scala
[1, 2, 3] |> each(emit) |> emit;
```

输出：

```text
1
2
3
[1, 2, 3]
```

`each` 会对每个元素调用 `emit`（所以每个元素都被输出了），但最终返回原数组不变。适合做副作用操作。

### 22.4 `reduce`：折叠成一个值

```scala
fn add2(a, b) {
    return $a + $b;
}

[1, 2, 3, 4] |> reduce(add2, 0) |> emit;
```

输出：

```text
10
```

`reduce(callable, initial)` 的工作过程：

1. 从初始值 `0` 开始
2. 取第一个元素 `1`，调用 `add2(0, 1)` 得到 `1`
3. 取第二个元素 `2`，调用 `add2(1, 2)` 得到 `3`
4. 取第三个元素 `3`，调用 `add2(3, 3)` 得到 `6`
5. 取第四个元素 `4`，调用 `add2(6, 4)` 得到 `10`
6. 没有更多元素了，返回 `10`

### 22.5 `scan`：保留每一步累积结果

`scan` 和 `reduce` 的逻辑一样，但它会把每一步的累积结果都收集起来：

```scala
[1, 2, 3, 4] |> scan(add2, 0) |> emit;
```

输出：

```text
[1, 3, 6, 10]
```

- 第 1 步：`0 + 1 = 1`
- 第 2 步：`1 + 2 = 3`
- 第 3 步：`3 + 3 = 6`
- 第 4 步：`6 + 4 = 10`

`reduce` 只给你最终结果 `10`；`scan` 给你整个过程 `[1, 3, 6, 10]`。

### 22.6 `find`：找第一个满足条件的元素

```scala
fn is_big(x) {
    return $x >= 3;
}

[1, 2, 3, 4] |> find(is_big) |> emit;
```

输出：

```text
3
```

找到第一个就停了，不会继续遍历。如果找不到，返回 `nil`。

### 22.7 `all` 和 `any`

```scala
fn is_big(x) {
    return $x >= 3;
}

[1, 2, 3, 4] |> all(is_big) |> emit;    // false（不是全部 >= 3）
[1, 2, 3, 4] |> any(is_big) |> emit;    // true（至少有一个 >= 3）
```

- `all`：全部满足才返回 `true`
- `any`：至少一个满足就返回 `true`

### 22.8 `tap`：中途看一眼

```scala
fn double(x) {
    return $x * 2;
}

[1, 2, 3]
    |> tap(emit)
    |> map(double)
    |> emit;
```

输出：

```text
[1, 2, 3]
[2, 4, 6]
```

`tap` 会把当前值送去做一次调用（这里是 `emit`，输出了 `[1, 2, 3]`），但最后仍然把原值继续往后传。

这在长管道里调试中间结果非常有用——插一个 `tap(emit)` 就能看到当前值是什么，不需要拆管道。

### 22.9 `flat_map`：映射后展开

```scala
fn around(x) {
    return [$x - 1, $x, $x + 1];
}

[2, 5] |> flat_map(around) |> emit;
```

输出：

```text
[1, 2, 3, 4, 5, 6]
```

`flat_map` 是"先 `map`，再 `flatten`"。每个元素被映射成一个数组，然后所有数组被展开合并。

### 22.10 `group_by`：按规则分组

```scala
fn parity(x) {
    when $x % 2 == 0 {
        return "even";
    } else {
        return "odd";
    }
}

[1, 2, 3, 4, 5] |> group_by(parity) |> emit;
```

输出：

```text
{"even": [2, 4], "odd": [1, 3, 5]}
```

每个元素被送进 callable，返回值作为分组键。

---

## 23. 记录数组管道：处理"一组对象"

在实际使用中，你经常会处理"一组长得差不多的字典"——比如用户列表、订单列表。Aethe 专门为这种场景提供了一套阶段。

这也是 Aethe "裸标识符"设计发光的地方。

### 23.1 `where`：按字段筛选

```scala
let users = [
    {name: "Alice", role: admin},
    {name: "Bob", role: guest},
    {name: "Carol", role: admin}
];

$users |> where(role, admin) |> emit;
```

输出：

```text
[{name: Alice, role: admin}, {name: Carol, role: admin}]
```

`where(role, admin)` 的意思是"只保留 `role` 字段等于 `admin` 的记录"。

注意 `role` 和 `admin` 都是裸标识符——不需要引号，直接写名字。

如果你要筛选的值来自变量，就用 `$`：

```scala
let wanted = admin;
$users |> where(role, $wanted) |> pluck(name) |> emit;
```

### 23.2 `pluck`：提取字段

```scala
$users |> pluck(name) |> emit;
```

输出：

```text
[Alice, Bob, Carol]
```

从每条记录里取出 `name` 字段，收集成数组。

### 23.3 `index_by`：按字段重组成字典

```scala
[
    {name: "Alice", score: 95},
    {name: "Bob", score: 88}
]
    |> index_by(name)
    |> emit;
```

输出：

```text
{Alice: {name: Alice, score: 95}, Bob: {name: Bob, score: 88}}
```

把指定字段的值当作字典键，整条记录当作字典值。

### 23.4 `count_by`：按字段计数

```scala
[
    {name: "Alice", role: admin},
    {name: "Bob", role: guest},
    {name: "Carol", role: admin}
]
    |> count_by(role)
    |> emit;
```

输出：

```text
{admin: 2, guest: 1}
```

统计每个字段值出现了多少次。

### 23.5 `sort_by` 和 `sort_desc_by`：按字段排序

```scala
let users = [
    {name: "Carol", score: 92},
    {name: "Alice", score: 95},
    {name: "Bob", score: 88}
];

$users |> sort_by(score) |> emit;
$users |> sort_desc_by(score) |> emit;
```

`sort_by(score)` 按 `score` 升序排列整条记录。`sort_desc_by` 是降序。

### 23.6 `distinct_by`：按字段去重

```scala
[
    {name: "Alice", score: 95},
    {name: "Bob", score: 88},
    {name: "Carol", score: 95}
]
    |> distinct_by(score)
    |> emit;
```

输出：

```text
[{name: Alice, score: 95}, {name: Bob, score: 88}]
```

按 `score` 去重——`score` 相同的只保留第一个出现的。注意返回的是整条记录，不是只有 score。

### 23.7 `sum_by`：按字段求和

```scala
[
    {name: "Alice", score: 95},
    {name: "Bob", score: 88},
    {name: "Carol", score: 91}
]
    |> sum_by(score)
    |> emit;
```

输出：

```text
274
```

把每条记录的 `score` 字段加起来。比先 `pluck(score)` 再 `sum` 更直接。

### 23.8 `evolve`：改字段

```scala
[
    {name: " alice ", role: admin},
    {name: "bob", role: guest}
]
    |> evolve(name, trim)
    |> evolve(name, upper)
    |> emit;
```

输出：

```text
[{name: ALICE, role: admin}, {name: BOB, role: guest}]
```

`evolve(name, trim)` 的意思是：取出每条记录的 `name` 字段值，送给 `trim` 处理，把结果写回 `name`。

注意 callable 看到的是字段的值，不是整条记录。

### 23.9 `derive`：加字段

```scala
fn badge(user) {
    when $user.score >= 90 {
        return "A";
    } else {
        return "B";
    }
}

[
    {name: "Alice", score: 95},
    {name: "Bob", score: 88}
]
    |> derive(level, badge)
    |> emit;
```

输出：

```text
[{name: Alice, score: 95, level: A}, {name: Bob, score: 88, level: B}]
```

`derive(level, badge)` 的意思是：把整条记录送给 `badge`，把返回值写到 `level` 字段。

**`evolve` 和 `derive` 的区别很重要：**

- `evolve(name, upper)` 里的 `upper` 看到的是 `name` 字段当前值
- `derive(level, badge)` 里的 `badge` 看到的是整条记录

一个改已有字段，一个基于记录创建新字段。

---

## 24. 对象：`type`

如果你想把一组字段和方法放在一起，组成一个有名字的结构，用 `type`。

### 24.1 基本写法

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

这定义了一个叫 `User` 的类型。它有两个字段 `name` 和 `score`，以及一个方法 `badge`。

### 24.2 构造对象

```scala
let user = User("Alice", 95);
```

`User("Alice", 95)` 会创建一个 User 对象，`name` 为 `"Alice"`，`score` 为 `95`。

### 24.3 访问字段和方法

```scala
$user.name |> emit;       // Alice
$user.score |> emit;      // 95
$user.badge() |> emit;    // A
```

### 24.4 `$self`

在方法里，`$self` 指向调用这个方法的对象本身。

```scala
type Counter(value) {
    fn increment() {
        $self.value = $self.value + 1;
    }

    fn current() {
        return $self.value;
    }
}

let c = Counter(0);
$c.increment();
$c.increment();
$c.increment();
$c.current() |> emit;
```

输出：

```text
3
```

---

## 25. `type_of`

你可以用 `type_of` 查看任何值的运行时类型：

```scala
type_of(123) |> emit;         // int
type_of("abc") |> emit;       // string
type_of(true) |> emit;        // bool
type_of(nil) |> emit;         // nil
type_of([1, 2]) |> emit;      // array
type_of({a: 1}) |> emit;      // dict
```

也可以在管道里用：

```scala
"hello" |> type_of |> emit;   // string
```

---

## 26. 常用内建函数

除了管道阶段，Aethe 还有一些以普通调用形式使用的内建函数：

### 26.1 类型转换

```scala
str(123) |> emit;       // "123"（整数转字符串）
int("42") |> emit;      // 42（字符串转整数）
bool("") |> emit;       // false（按真值规则转换）
```

### 26.2 范围生成

```scala
range(5) |> emit;        // [0, 1, 2, 3, 4]
range(2, 6) |> emit;     // [2, 3, 4, 5]
range(10, nil) |> take(5) |> emit; // [10, 11, 12, 13, 14]
```

`range(...)` 现在返回惰性流（lazy stream）：只有真正消费时（例如 `take`、`head`、`emit`）才会逐项计算。

### 26.3 用户输入

```scala
let name = input("your name> ");
$name |> emit;
```

`input()` 读取标准输入一行。`input("prompt")` 会先输出提示文本。

### 26.4 读取文件

```scala
read_file("data.txt") |> emit;
```

把整个文件读入为一个字符串。

### 26.5 并行映射 `pmap`

```scala
range(10000) |> pmap(bind(mul, 2)) |> take(5) |> emit;
```

`pmap` 会并行处理数组（保持顺序），适合大列表计算。

---

## 27. 一段完整的小程序

把前面学的东西串起来：

```scala
// 定义处理函数
fn normalize(name) {
    return $name |> trim |> lower;
}

fn is_long(name) {
    return $name |> size |> _ >= 5;
}

// 原始数据
let users = ["  Alice  ", "BOB", "   Carol", "ed"];

// 管道处理
$users
    |> map(normalize)
    |> filter(is_long)
    |> join(", ")
    |> concat("valid: ")
    |> emit;
```

输出：

```text
valid: alice, carol
```

从左往右读：

1. 拿到用户名数组
2. 每个都做规范化（去空白、转小写）
3. 过滤掉长度不够 5 的
4. 用逗号拼成字符串
5. 加上前缀
6. 输出

---

## 28. 更大一些的例子

下面这个例子更接近实际场景——处理一组结构化数据：

```scala
fn badge(user) {
    when $user.score >= 90 {
        return "excellent";
    } else {
        return "good";
    }
}

let students = [
    {name: " alice ", score: 95, dept: engineering},
    {name: "bob", score: 82, dept: design},
    {name: "carol", score: 91, dept: engineering},
    {name: "dave", score: 78, dept: design}
];

// 清理名字、加评级、按部门分组
$students
    |> evolve(name, trim)
    |> evolve(name, upper)
    |> derive(level, badge)
    |> group_by(bind(get, dept))
    |> emit;

// 工程部门总分
$students
    |> where(dept, engineering)
    |> sum_by(score)
    |> emit;

// 按分数降序排列
$students
    |> sort_desc_by(score)
    |> pluck(name)
    |> emit;
```

这段代码展示了 Aethe 的典型用法：数据从上面流进来，每一步做一件事，最终得到你想要的结果。

---

## 29. 常见错误

### 29.1 忘了 `run`

在 REPL 里写完代码后没有输出？你可能忘了单独输入 `run`。

### 29.2 把变量名写成裸标识符

错误：

```scala
name |> emit;     // 输出的是符号 name，不是变量值
```

正确：

```scala
$name |> emit;    // 输出变量 name 的值
```

### 29.3 在管道外用 `_`

错误：

```scala
let x = _ * 2;   // _ 只能在管道右侧用
```

正确：

```scala
10 |> _ * 2 |> into(x);
```

### 29.4 漏分号

如果 REPL 一直显示 `...>` 不让你 `run`，很可能是某条语句漏了 `;`。

### 29.5 函数里忘了 `return`

```scala
// 这段代码没问题——但函数不会返回值
fn double(x) {
    $x * 2;
}

// 加上 return
fn double(x) {
    return $x * 2;
}
```

### 29.6 该用 `$` 没用，不该用 `$` 用了

```scala
// 错误：想取字段但写成了变量读取
{name: "Alice"} |> get($name);
// 如果有一个变量叫 name 且值为 "xxx"，这会去取 "xxx" 字段

// 正确：裸标识符表示字段名
{name: "Alice"} |> get(name);
```

```scala
// 错误：想读变量但写成了裸标识符
let score = 95;
score |> emit;     // 输出符号 score

// 正确
$score |> emit;    // 输出 95
```

---

## 30. 学习顺序建议

如果你打算系统学会 Aethe，按下面的顺序来：

1. **环境**：能构建、能运行、熟悉 IDE 或 REPL
2. **基础**：值、变量、`emit`、分号
3. **管道**：`|>`、自动首参注入、`_` 占位符、裸表达式管道
4. **函数**：`fn`、`return`
5. **控制流**：`when`、`while`、`for`
6. **数组和字典**：创建、访问、常用阶段
7. **集合管道**：`map`、`filter`、`reduce`、`find`、`each`
8. **stage 和 pipe**：理解它们和 `fn` 的区别
9. **记录数组**：`where`、`pluck`、`sort_by`、`evolve`、`derive`
10. **组合子**：`bind`、`chain`、`branch`、`guard`
11. **对象**：`type`、`$self`

每学一步，自己动手写几段代码试试。Aethe 是一门"写了才能理解"的语言。

---

## 31. 练习

### 练习 1：字符串处理

要求：输入 `"  hello world  "`，去空白，转大写，输出。

参考答案：

```scala
"  hello world  " |> trim |> upper |> emit;
```

### 练习 2：求和

要求：计算 1 到 5 的和。

参考答案（循环版）：

```scala
let total = 0;
let n = 5;

while $n > 0 {
    $total += $n;
    $n -= 1;
}

$total |> emit;
```

参考答案（管道版）：

```scala
[1, 2, 3, 4, 5] |> sum |> emit;
```

### 练习 3：数组变换

要求：把 `[1, 2, 3, 4]` 的每项乘以 2。

参考答案：

```scala
fn double(x) {
    return $x * 2;
}

[1, 2, 3, 4] |> map(double) |> emit;
```

或者用 `bind`：

```scala
[1, 2, 3, 4] |> map(bind(mul, 2)) |> emit;
```

### 练习 4：记录数组筛选

要求：从下面的用户列表中，找出所有管理员的名字。

```scala
let users = [
    {name: "Alice", role: admin},
    {name: "Bob", role: guest},
    {name: "Carol", role: admin},
    {name: "Dave", role: guest}
];
```

参考答案：

```scala
$users
    |> where(role, admin)
    |> pluck(name)
    |> emit;
```

### 练习 5：定义对象

要求：定义一个 `Rectangle` 类型，有 `width` 和 `height`，方法 `area` 返回面积。

参考答案：

```scala
type Rectangle(width, height) {
    fn area() {
        return $self.width * $self.height;
    }
}

let r = Rectangle(5, 3);
$r.area() |> emit;
```

### 练习 6：管道组合

要求：定义一个组合步骤，能把字符串清理成"去空白、转大写、加感叹号"的格式，然后对数组里的每个元素使用它。

参考答案：

```scala
let loud = chain(trim, upper, bind(concat, "!"));

["  hello  ", " world ", "  aethe  "]
    |> map($loud)
    |> emit;
```

---

## 32. 学完这份教程后该看什么

当你已经能写出上面的例子后，下一步应该看：

- [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)：查完整语法和所有内建能力的精确定义
- [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)：看具体实现

如果你发现自己"知道概念，但忘了某个阶段的参数形式"，优先查参考手册；如果你发现自己"根本不知道应该怎么开始写"，回来看这份教程。
