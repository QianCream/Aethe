# Aethe 2 新手教程

这份教程面向从未接触过 Aethe，甚至没有系统学过编程的读者。

这份教程对应 `Aethe 2 / 2.1.0`。目标不是只让你“看懂例子”，而是让你能够独立写出一段可运行的 Aethe 程序，并知道什么时候该使用 `fn`、什么时候该使用 `stage`、什么时候该使用匿名 `pipe`、什么时候该使用管道组合子、什么时候该写管道表达式。

如果你只打算看一份文档，先读这份教程；如果你已经学会基础语法，再去看 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)。

## 1. Aethe 是什么

Aethe 是一门以“管道”为核心的语言。

很多语言会这样写：

```text
print(to_upper(trim("  hello  ")))
```

Aethe 更倾向于这样写：

```scala
"  hello  " |> trim |> upper |> emit;
```

从左到右读就行：

1. 先拿到字符串 `"  hello  "`
2. 交给 `trim`
3. 再交给 `upper`
4. 再交给 `emit`

这种写法的重点是“数据在流动”。你可以把它理解成一条装配线，值从左边进入，每经过一步就被处理一次。

## 2. 启动方式

构建：

```bash
g++ -std=c++11 main.cpp -o aethe
```

运行：

```bash
./aethe
```

启动后会看到提示符：

```text
>>>
```

这表示你正在输入一个新的缓冲区。

## 3. REPL 怎么工作

REPL 的规则非常重要：

- 你输入代码后，不会立刻运行
- 只有单独输入一行 `run`，当前缓冲区才会统一执行
- `>>>` 表示新缓冲区
- `...>` 表示当前代码还没输完，继续补下一行

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

## 4. 先写第一个程序

输入下面几行：

```scala
"hello, Aethe" |> emit;
run
```

输出：

```text
hello, Aethe
```

这里你已经学会了三件事：

- 字符串写在双引号里
- `|>` 是管道符
- `emit` 用来输出当前值

## 5. Aethe 里的“值”

程序处理的对象叫“值”。Aethe 当前支持这些值：

- `int`：整数
- `bool`：布尔值，只有 `true` 和 `false`
- `string`：字符串
- `nil`：空值
- `array`：数组
- `dict`：字典
- `object`：对象
- `callable`：可调用值，例如匿名 `pipe`

示例：

```scala
123 |> emit;
true |> emit;
"Aethe" |> emit;
nil |> emit;
[1, 2, 3] |> emit;
{name: "Alice", score: 99} |> emit;
run
```

## 6. 语句与分号

Aethe 里的大多数语句都要以 `;` 结尾。

例如：

```scala
1 |> emit;
2 |> emit;
run
```

如果你漏了分号，REPL 会继续等待，认为这段输入还没结束。

## 7. 管道的基本读法

这是 Aethe 最重要的部分。

```scala
"  hello  " |> trim |> upper |> emit;
run
```

可以读成：

1. 产生 `"  hello  "`
2. 交给 `trim` 去掉首尾空白
3. 把结果交给 `upper` 转成大写
4. 把结果交给 `emit` 输出

输出是：

```text
HELLO
```

你应该养成一个习惯：

- 不要先想“函数嵌套”
- 先想“值怎么从左往右流动”

## 8. `emit`、`print`、`show`

这三个名字当前是同义词：

- `emit`
- `print`
- `show`

它们都会输出当前值，并且把这个值继续传下去。

例如：

```scala
"hello" |> emit |> upper |> emit;
run
```

会输出两次：

```text
hello
HELLO
```

## 9. 变量是什么

变量就是“给一个值起名字”，以后可以通过这个名字再取回来。

定义变量：

```scala
let name = "Alice";
let score = 95;
run
```

读取变量时要加 `$`：

```scala
$name |> emit;
$score |> emit;
run
```

这是 Aethe 的固定规则：

- `name`：裸标识符，表示一个符号字符串
- `$name`：变量读取

这两者不能混用。

也正因为这样，你会经常写出 `into(score)`、`get(name)`、`where(role, admin)` 这种“把名字直接写进管道”的代码。

## 10. `let` 和 `into`

下面两种写法等价：

```scala
let score = 100;
```

```scala
100 |> into(score);
```

第一种更像常见语言；第二种更符合 Aethe 的管道风格。

大多数时候你可以优先使用 `let`，代码更容易读。

## 11. 表达式：先算出一个值

表达式就是“会得到结果的代码”。

例如：

```scala
1 + 2 * 3 |> emit;
run
```

输出：

```text
7
```

支持的常用运算符：

- 算术：`+` `-` `*` `/` `%`
- 比较：`==` `!=` `>` `>=` `<` `<=`
- 逻辑：`&&` `||`
- 一元：`!` `-`

字符串也能参与 `+`，会变成拼接：

```scala
"Aethe " + "Language" |> emit;
run
```

## 12. 管道的自动注入

当你这样写：

```scala
21 |> double;
```

如果 `double` 是一个可调用对象，那么左边的 `21` 会自动成为它的第一个参数。

也就是说，上面的写法等价于：

```text
double(21)
```

再看一个例子：

```scala
fn add(a, b) {
    return $a + $b;
}

10 |> add(5) |> emit;
run
```

等价理解：

```text
add(10, 5)
```

输出：

```text
15
```

## 13. 占位符 `_`

有时候你不想让输入值总是变成第一个参数，这时就用 `_`。

例如：

```scala
"Hello, Aethe!" |> substring(_, 7, 5) |> emit;
run
```

这里 `_` 表示“当前管道输入”，所以这一步等价于：

```text
substring("Hello, Aethe!", 7, 5)
```

### 13.1 `_` 可以出现多次

```scala
fn pair(a, b) {
    return concat($a, " | ", $b);
}

"echo" |> pair(_, _) |> emit;
run
```

输出：

```text
echo | echo
```

### 13.2 `_` 不是匿名函数

这是 Aethe 的固定规则。

`_` 只表示“当前这一步管道的输入值”，不是 lambda，不是匿名函数参数，也不能单独拿出来存起来。

如果你真的需要匿名可调用对象，应该写 `pipe(x) { ... }`。

合法：

```scala
10 |> _ * 3 + 5 |> emit;
run
```

非法：

```scala
_ * 3;
```

因为 `_` 只能出现在管道右侧。

## 14. 裸表达式管道

右边不一定非要是函数名，也可以直接是表达式：

```scala
10 |> _ * 3 + 5 |> emit;
run
```

输出：

```text
35
```

这在做简单变换时非常方便。

## 15. `fn`：普通函数

`fn` 用来定义可复用逻辑。

示例：

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;
run
```

输出：

```text
42
```

### 15.1 如何理解参数

```scala
fn add(a, b) {
    return $a + $b;
}
```

这里：

- `a`、`b` 是参数名
- 在函数体里要用 `$a`、`$b` 读取

### 15.2 `return` 和 `give`

这两个现在是同义形式。

下面两段等价：

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

## 16. `stage`：管道阶段

`stage` 也是可复用逻辑，但它专门为管道设计。

示例：

```scala
stage shout(suffix) {
    return $it |> upper |> concat($suffix);
}

"hello" |> shout("!!!") |> emit;
run
```

输出：

```text
HELLO!!!
```

### 16.1 `stage` 和 `fn` 的区别

最简单的记法：

- `fn`：我在写普通函数
- `stage`：我在写一段天然挂在 `|>` 后面的步骤

`stage` 里会自动提供一个特殊变量：`$it`

它表示“当前流进这个阶段的输入值”。

### 16.2 什么时候用 `fn`

适合这些情况：

- 逻辑更像普通运算
- 参数地位相对平等
- 你希望既能普通调用，也能被管道调用

### 16.3 什么时候用 `stage`

适合这些情况：

- 你明显是在处理当前输入值
- 代码读法天然是“某值 |> 某步骤”
- 你希望阶段体里频繁用 `$it`

### 16.4 `pipe`：匿名可调用对象

从 Aethe 2 开始，你可以直接在表达式里写匿名 `pipe`。

```scala
let double = pipe(x) {
    return $x * 2;
};

21 |> $double |> emit;
[1, 2, 3] |> map($double) |> emit;
run
```

`pipe` 和 `fn` 的差别是：

- `fn` 会给逻辑起一个顶层名字
- `pipe` 是一个值，可以像别的值一样存进变量、传来传去

它也可以直接内联：

```scala
[1, 2, 3, 4]
|> reduce(pipe(acc, item) {
    return $acc + $item;
}, 0)
|> emit;
run
```

### 16.5 管道组合子：`bind`、`chain`、`branch`、`guard`

到 `Aethe 2.1` 为止，可调用对象不只是“你自己手写一个 `pipe`”，还可以是几条现有步骤重新拼出来的新值。

`bind` 用来把“带参数的 stage”变成可传递对象：

```scala
[1, 2, 3] |> map(bind(add, 10)) |> emit;
[{name: "Alice"}, {name: "Bob"}] |> map(bind(get, name)) |> emit;
run
```

`chain` 用来把多个步骤串成一条路线：

```scala
let loud = chain(trim, upper, bind(concat, "!"));
"  hello  " |> $loud |> emit;
run
```

`branch` 用来把同一个输入同时送进多条路线：

```scala
"Aethe" |> branch(type_of, str, bind(concat, "!")) |> emit;
run
```

`guard` 用来把条件分流包成一个可调用步骤：

```scala
5 |> guard(pipe(x) { return $x > 3; }, bind(add, 100), bind(sub, 100)) |> emit;
run
```

可以这样理解：

- `pipe(x) { ... }` 适合你亲自写逻辑体
- `bind(...)` 适合给现有步骤补参数
- `chain(...)` 适合把常用路线打包
- `branch(...)` 适合一次派生多种结果
- `guard(...)` 适合把条件分流压缩成一个管道步骤

## 17. 条件分支：`when`

`when` 相当于“如果……就……”。

```scala
let score = 75;

when $score >= 60 {
    "pass" |> emit;
} else {
    "fail" |> emit;
}
run
```

输出：

```text
pass
```

### 17.1 真值规则

在 Aethe 里：

- `false` 为假
- `nil` 为假
- `0` 为假
- 空字符串 `""` 为假
- 空数组 `[]` 为假
- 空字典 `{}` 为假
- 其他值通常为真

## 18. 多分支：`match`

当你要根据一个值做多路分支时，用 `match` 更清楚。

```scala
let grade = "A";

match $grade {
    case "A" {
        "excellent" |> emit;
    }
    case "B" {
        "good" |> emit;
    }
    else {
        "keep going" |> emit;
    }
}
run
```

## 19. 循环：`while`

`while` 表示“只要条件成立，就一直重复”。

```scala
let n = 3;

while $n > 0 {
    $n |> emit;
    let n = $n - 1;
}
run
```

输出：

```text
3
2
1
```

## 20. 遍历：`for`

`for` 用于逐个处理集合里的元素。

### 20.1 遍历数组

```scala
for item in [10, 20, 30] {
    $item |> emit;
}
run
```

### 20.2 遍历字符串

```scala
for ch in "abc" {
    $ch |> emit;
}
run
```

### 20.3 遍历字典

遍历字典时，每次拿到的是一个包含 `key` 和 `value` 的小字典。

```scala
for entry in {name: "Alice", score: 90} {
    $entry.key |> emit;
    $entry.value |> emit;
}
run
```

## 21. `break` 和 `continue`

`break`：提前结束当前循环。

```scala
for item in [1, 2, 3, 4] {
    when $item == 3 {
        break;
    }
    $item |> emit;
}
run
```

输出：

```text
1
2
```

`continue`：跳过本轮，直接开始下一轮。

```scala
for item in [1, 2, 3, 4] {
    when $item == 3 {
        continue;
    }
    $item |> emit;
}
run
```

输出：

```text
1
2
4
```

## 22. `defer`

`defer` 表示“离开当前作用域时再执行”。

它适合做收尾工作。

```scala
fn demo() {
    defer {
        "leaving demo" |> emit;
    }

    "working" |> emit;
}

demo();
run
```

输出顺序：

```text
working
leaving demo
```

## 23. 数组与字典

### 23.1 数组

数组就是有顺序的一组值：

```scala
[1, 2, 3]
["a", "b", "c"]
[]
```

常用阶段：

- `append(x)` / `push(x)`：尾部追加
- `prepend(x)`：头部插入
- `get(i)` / `at(i)`：按下标读取
- `head`：首元素
- `last`：尾元素
- `size`：长度
- `index_of(x)`：查找第一个匹配元素的位置
- `repeat(count)`：重复数组内容
- `take(count)`：取前几个元素
- `skip(count)`：跳过前几个元素
- `slice(start, length)`：切片
- `reverse`：逆序
- `distinct`：去重
- `sort`：升序排序
- `sort_desc`：降序排序
- `sum`：对整数数组求和
- `flatten`：把数组中的子数组展开一层

示例：

```scala
[1, 2, 3] |> append(4) |> reverse |> emit;
run
```

### 23.2 字典

字典就是“键到值”的映射：

```scala
{name: "Alice", score: 95}
```

常用阶段：

- `get(key)` / `field(key)`：读取键
- `set(key, value)`：写入键
- `keys`：所有键
- `values`：所有值
- `entries`：把字典展开成键值对数组
- `pick(...)`：只保留指定键
- `omit(...)`：移除指定键
- `merge(other)`：合并另一份结构
- `rename(from, to)`：重命名键
- `has(key)` / `contains(key)`：判断键是否存在

示例：

```scala
{name: "Alice"} |> set(score, 95) |> emit;
{name: "Alice", score: 95, city: "Shanghai"} |> pick(name, city) |> emit;
{name: "Alice", score: 95, city: "Shanghai"} |> omit(score) |> emit;
{name: "Alice", score: 95} |> merge({score: 99, level: "A"}) |> emit;
{name: "Alice", score: 95} |> rename(score, total_score) |> emit;
run
```

说明：

- `entries` 返回数组
- `pick`、`omit`、`merge` 即使接收对象输入，结果也统一返回 `dict`
- `rename` 也统一返回 `dict`
- 这里的 `score`、`name`、`city` 都是裸标识符，也就是名字本身，不是变量读取

## 24. 字符串处理

常用字符串阶段：

- `trim`
- `upper` / `to_upper`
- `lower` / `to_lower`
- `concat(...)`
- `substring(start, length)`
- `split(sep)`
- `contains(x)`
- `index_of(x)`
- `starts_with(prefix)`
- `ends_with(suffix)`
- `replace(from, to)`
- `repeat(count)`
- `take(count)`
- `skip(count)`
- `slice(start, length)`
- `reverse`

示例：

```scala
"  aethe runtime  "
|> trim
|> upper
|> replace("RUNTIME", "LANG")
|> emit;
run
```

## 25. 集合式管道：批量处理数组

Aethe 提供一组很重要的数组阶段：

- `tap`
- `map`
- `flat_map`
- `filter`
- `find`
- `each`
- `all`
- `any`
- `reduce`
- `group_by`
- `pluck`
- `where`
- `index_by`
- `count_by`
- `sort_by`
- `sort_desc_by`
- `distinct_by`
- `sum_by`
- `evolve`
- `derive`
- `chunk`
- `zip`

### 25.1 `map`

把数组的每个元素都变成另一个值。

```scala
fn double(x) {
    return $x * 2;
}

[1, 2, 3] |> map(double) |> emit;
run
```

输出：

```text
[2, 4, 6]
```

### 25.2 `filter`

只保留满足条件的元素。

```scala
fn ge_three(x) {
    return $x >= 3;
}

[1, 2, 3, 4] |> filter(ge_three) |> emit;
run
```

输出：

```text
[3, 4]
```

### 25.3 `each`

对每个元素做事，但返回原数组。

```scala
[1, 2, 3] |> each(print) |> emit;
run
```

### 25.4 `reduce`

把整个数组折叠成一个值。

```scala
fn add2(a, b) {
    return $a + $b;
}

[1, 2, 3, 4] |> reduce(add2, 0) |> emit;
run
```

输出：

```text
10
```

### 25.5 `find`

返回第一个满足条件的原始元素；如果不存在，返回 `nil`。

```scala
fn ge_three(x) {
    return $x >= 3;
}

[1, 2, 3, 4] |> find(ge_three) |> emit;
run
```

输出：

```text
3
```

### 25.6 `all` 与 `any`

`all` 用来判断“是否全部满足”，`any` 用来判断“是否至少一个满足”。

```scala
fn ge_three(x) {
    return $x >= 3;
}

[1, 2, 3, 4] |> all(ge_three) |> emit;
[1, 2, 3, 4] |> any(ge_three) |> emit;
run
```

输出：

```text
false
true
```

### 25.7 `sum` 与 `flatten`

`sum` 适合把整数数组快速求和；`flatten` 适合把“数组里的数组”展开一层。

```scala
[1, 2, 3, 4] |> sum |> emit;
[[1, 2], [3], [4, 5]] |> flatten |> emit;
run
```

输出：

```text
10
[1, 2, 3, 4, 5]
```

### 25.8 `take`、`skip`、`distinct`

这三个阶段适合处理“数组子集”和“去重”。

```scala
[1, 2, 3, 4, 5] |> take(3) |> emit;
[1, 2, 3, 4, 5] |> skip(2) |> emit;
[1, 2, 2, 3, 1] |> distinct |> emit;
run
```

输出：

```text
[1, 2, 3]
[3, 4, 5]
[1, 2, 3]
```

### 25.9 `sort` 与 `sort_desc`

这两个阶段用于数组排序。

```scala
[4, 1, 3, 2] |> sort |> emit;
[4, 1, 3, 2] |> sort_desc |> emit;
run
```

输出：

```text
[1, 2, 3, 4]
[4, 3, 2, 1]
```

当前限制：

- 只能排序 `int`、`string`、`bool` 数组
- 数组里的元素类型必须一致

### 25.10 `tap`：中途看一眼，但继续往下流

`tap` 很有“管道味”。

它会把当前值送去做一次调用，但最后仍然把原值继续往后传。

```scala
[1, 2, 3]
|> tap(emit)
|> map(double)
|> emit;
run
```

这很适合在长管道中调试中间结果。

### 25.11 `chunk` 与 `zip`

`chunk(size)` 适合把一长串数据切成小段；`zip(other)` 适合把两路数据并排对齐。

```scala
"abcdef" |> chunk(2) |> emit;
["A", "B", "C"] |> zip([1, 2]) |> emit;
run
```

输出：

```text
[ab, cd, ef]
[[A, 1], [B, 2]]
```

### 25.12 `flat_map` 与 `group_by`

`flat_map` 是“先映射，再立刻展开一层”；`group_by` 是“按规则重新分桶”。

```scala
fn around(x) {
    return [$x - 1, $x, $x + 1];
}

fn parity(x) {
    when $x % 2 == 0 {
        return "even";
    } else {
        return "odd";
    }
}

[2, 4] |> flat_map(around) |> emit;
[1, 2, 3, 4, 5] |> group_by(parity) |> emit;
run
```

输出：

```text
[1, 2, 3, 3, 4, 5]
{"even": [2, 4], "odd": [1, 3, 5]}
```

### 25.13 `where` 与 `pluck`

这两个阶段把 Aethe 的“管道标识符”特点放大了。

你可以直接把字段名和值写成裸标识符，而不是总是写成字符串：

```scala
let users = [
    {name: "Alice", role: admin},
    {name: "Bob", role: guest},
    {name: "Carol", role: admin}
];

$users
|> where(role, admin)
|> pluck(name)
|> emit;
run
```

输出：

```text
[Alice, Carol]
```

如果比较目标来自变量，就显式写 `$`：

```scala
let wanted_role = admin;

$users
|> where(role, $wanted_role)
|> pluck(name)
|> emit;
run
```

### 25.14 `index_by` 与 `count_by`

如果你不只是想筛选，还想把记录数组重新编成字典或做分组计数，这两个阶段会更直接。

`index_by(key)` 会把指定字段值当成字典键：

```scala
[
    {name: "Alice", score: 95},
    {name: "Bob", score: 88}
]
|> index_by(name)
|> emit;
run
```

`count_by(key)` 会统计每个字段值出现了多少次：

```scala
[
    {name: "Alice", role: admin},
    {name: "Bob", role: guest},
    {name: "Carol", role: admin}
]
|> count_by(role)
|> emit;
run
```

这两种写法都延续了同一个规则：

- `name`、`role` 是字段名
- 它们是裸标识符，不是变量读取
- 如果字段值不是字符串，就不能直接拿来做 `index_by` / `count_by` 的键

### 25.15 `sort_by`、`distinct_by`、`sum_by`

到了这一步，记录数组已经不只是能筛选和重组，还能直接按字段做排序、去重和汇总。

```scala
let users = [
    {name: "Alice", role: admin, score: 95},
    {name: "Bob", role: guest, score: 88},
    {name: "Carol", role: admin, score: 95},
    {name: "Dora", role: guest, score: 91}
];

$users |> sort_by(score) |> emit;
$users |> sort_desc_by(name) |> emit;
$users |> distinct_by(score) |> emit;
$users |> sum_by(score) |> emit;
run
```

可以这样理解：

- `sort_by(score)` 是“按 `score` 字段排序记录”
- `distinct_by(score)` 是“按 `score` 字段去重，但保留原始记录”
- `sum_by(score)` 是“把每条记录里的 `score` 加起来”

这比先 `pluck(score)` 再手动回到原记录或继续组合要更直接，也更符合 Aethe 现在这条“字段名直接进管道”的路线。

### 25.16 `evolve` 与 `derive`

这是这一轮里更偏“语言能力”的更新。

前面的阶段主要是筛选、重排、统计；`evolve` 和 `derive` 开始让你直接在管道里改造记录本身。

`evolve(key, callable)` 用来改已有字段：

```scala
[
    {name: " alice ", role: admin},
    {name: "bob", role: guest}
]
|> evolve(name, trim)
|> evolve(name, upper)
|> emit;
run
```

`derive(key, callable)` 用来根据整条记录补一个新字段：

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
run
```

它们的区别很重要：

- `evolve(name, upper)` 里的 `upper` 看到的是 `name` 字段当前值
- `derive(level, badge)` 里的 `badge` 看到的是整条记录
- 两者结果都会回到管道里继续流动

这让记录变形第一次真正变成 Aethe 自己的表达方式，而不是“先拆字段，再手动拼回去”。

## 26. 对象：`type`

如果你想把一组字段和方法放在一起，用 `type`。

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
$user.name |> emit;
$user.badge() |> emit;
run
```

这里你会接触两个新概念：

- `User("Alice", 95)`：构造一个对象
- `$self`：方法里的当前对象

## 27. `type_of`

你可以用 `type_of(x)` 查看运行时类型名：

```scala
type_of(123) |> emit;
type_of("abc") |> emit;
type_of([1, 2, 3]) |> emit;
run
```

输出类似：

```text
int
string
array
```

## 28. 常用内建函数

普通调用形式的常用内建：

- `range(end)`
- `range(start, end)`
- `input()`
- `input(prompt)`
- `str(x)`
- `int(x)`
- `bool(x)`
- `type_of(x)`

例如：

```scala
let name = input("name> ");
$name |> emit;
range(5) |> emit;
str(123) |> emit;
int("42") |> emit;
bool("") |> emit;
run
```

## 29. 一段完整的小程序

下面这段代码把一批用户名规范化，并输出汇总结果：

```scala
fn normalize(name) {
    return $name |> trim |> lower;
}

fn is_long(name) {
    return $name |> size |> gte(5);
}

let users = ["  Alice  ", "BOB", "   Carol", "ed"];

$users
|> map(normalize)
|> filter(is_long)
|> join(", ")
|> concat("valid users: ")
|> emit;
run
```

你应该能从左往右读懂它：

1. 先拿到用户数组
2. 每个名字都做规范化
3. 过滤掉长度不够的名字
4. 拼成字符串
5. 加上前缀
6. 输出

## 30. 常见错误

### 30.1 忘记输入 `run`

现象：代码输入完了，但没有输出。

原因：Aethe 不会自动执行，必须手动输入 `run`。

### 30.2 把变量写成裸标识符

错误写法：

```scala
name |> emit;
```

如果 `name` 是变量，应该写：

```scala
$name |> emit;
```

### 30.3 在管道外使用 `_`

错误写法：

```scala
let x = _ * 2;
```

`_` 只能在管道右侧使用。

### 30.4 漏分号

现象：提示符一直是 `...>`。

原因：当前代码块还没结束。

### 30.5 在 `fn` 里忘了返回

例如：

```scala
fn double(x) {
    $x * 2;
}
```

这段代码会计算 `$x * 2`，但没有显式返回结果。更稳妥的写法是：

```scala
fn double(x) {
    return $x * 2;
}
```

## 31. 学习顺序建议

如果你打算系统学会 Aethe，建议按下面顺序练：

1. 先熟悉 REPL、分号、`run`
2. 再熟悉值、变量、`emit`
3. 学会最基础的管道和字符串阶段
4. 学 `_` 占位符和表达式管道
5. 学 `fn`
6. 学 `stage`
7. 学 `when`、`while`、`for`
8. 学数组与集合式管道
9. 学 `type`

## 32. 练习

### 练习 1：把字符串转成大写

要求：

- 输入 `"  hello world  "`
- 去掉首尾空白
- 转成大写
- 输出

参考答案：

```scala
"  hello world  " |> trim |> upper |> emit;
run
```

### 练习 2：计算 1 到 5 的和

参考答案：

```scala
let n = 5;
let sum = 0;

while $n > 0 {
    let sum = $sum + $n;
    let n = $n - 1;
}

$sum |> emit;
run
```

### 练习 3：把数组每项都乘以 2

参考答案：

```scala
fn double(x) {
    return $x * 2;
}

[1, 2, 3, 4] |> map(double) |> emit;
run
```

### 练习 4：定义一个对象并输出等级

参考答案：

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

let user = User("Alice", 96);
$user.badge() |> emit;
run
```

## 33. 学完这份教程后该看什么

当你已经能写出上面的例子后，下一步应该看：

- [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)：查完整语法和所有内建能力
- [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)：看具体实现

如果你发现自己“知道概念，但忘了某个阶段的参数形式”，优先查参考手册；如果你发现自己“根本不知道应该怎么开始写”，回来看这份教程。
