# Aethe 语言参考

本文档描述 Aethe 当前实现的语法、运行时语义和内建能力。内容按 reference 风格组织，适合作为查询手册使用。

## 1. 词法元素

### 1.1 标识符

标识符用于表示：

- 变量名
- 函数名
- stage 名
- 类型名
- 字段名
- 裸标识符值

标识符由字母、数字和下划线组成，首字符可以是字母或下划线。

示例：

```scala
name
user_score
_tmp
```

### 1.2 关键字

当前关键字：

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
- `true`
- `false`
- `nil`

### 1.3 注释

仅支持单行注释：

```scala
// this is a comment
```

### 1.4 数字字面量

当前仅支持十进制整数：

```scala
0
42
1024
```

### 1.5 字符串字面量

字符串使用双引号表示：

```scala
"hello"
```

支持的转义序列：

- `\n`
- `\t`
- `\"`
- `\\`

## 2. 运行时值

当前支持以下运行时值类型：

- `int`
- `bool`
- `string`
- `nil`
- `array`
- `dict`
- `object`

### 2.1 `int`

```scala
10
-3
```

### 2.2 `bool`

```scala
true
false
```

### 2.3 `string`

```scala
"Aethe"
```

### 2.4 `nil`

```scala
nil
```

### 2.5 `array`

```scala
[1, 2, 3]
["a", "b", "c"]
```

### 2.6 `dict`

字典键当前必须是字符串字面量或裸标识符。

```scala
{name: "Alice", score: 99}
{"lang": "Aethe", "ver": 1}
```

### 2.7 `object`

对象值通过 `type` 构造产生：

```scala
User("Alice", 99)
```

## 3. 真值规则

以下值视为假：

- `nil`
- `false`
- `0`
- `""`
- `[]`
- `{}`

其他值视为真。

## 4. 裸标识符与变量

### 4.1 裸标识符

裸标识符默认表示符号式字符串值，而不是变量内容。

```scala
name
score
```

常见用途：

- `into(score)`
- `get(name)`
- 字典字段名

### 4.2 变量读取

变量读取必须使用 `$`。

```scala
$name
$score
```

示例：

```scala
let score = 100;
$score |> emit;
```

## 5. 表达式

### 5.1 主表达式

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
- 占位符 `_`

### 5.2 调用表达式

调用语法：

```scala
add(1, 2)
User("Alice", 99)
```

### 5.3 成员访问

语法：

```scala
$user.name
$user.badge()
```

成员访问可用于：

- 对象字段读取
- 字典字段读取
- 方法调用

### 5.4 一元运算

支持：

- `!expr`
- `-expr`

### 5.5 二元运算

支持：

- 算术：`+` `-` `*` `/` `%`
- 比较：`==` `!=` `>` `>=` `<` `<=`
- 逻辑：`&&` `||`

示例：

```scala
1 + 2 * 3
$x > 10 && $x < 20
```

### 5.6 运算符优先级

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

## 6. 管道

管道是 Aethe 的核心语法：

```scala
source |> target |> target;
```

每一步把左侧结果传递给右侧目标。

### 6.1 自动首参注入

如果右侧不包含 `_`，则左侧值默认作为第一个参数注入右侧目标。

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

### 6.2 占位符注入

如果右侧包含 `_`，则 `_` 所在位置就是当前输入的注入位置。

```scala
"Hello, Aethe!" |> substring(_, 7, 3) |> emit;
```

等价理解：

```text
substring("Hello, Aethe!", 7, 3)
```

### 6.3 多次使用 `_`

同一输入值可以在同一条管道目标内复用多次。

```scala
5 |> power(_, _) |> emit;
```

### 6.4 裸表达式目标

右侧可以是表达式，不要求必须是函数名。

```scala
10 |> _ * 3 + 5 |> emit;
```

### 6.5 `_` 的限制

`_` 不是匿名函数，也不是闭包参数。它只在当前管道右侧表达式中表示“当前输入值”。

非法示例：

```scala
_ * 3
```

## 7. 语句

### 7.1 表达式语句

表达式语句以 `;` 结束：

```scala
1 + 2 |> emit;
"abc" |> upper |> emit;
```

### 7.2 `let`

语法：

```scala
let name = expr;
```

语义上等价于：

```scala
expr |> into(name);
```

### 7.3 `when`

语法：

```scala
when condition {
    ...
} else {
    ...
}
```

`else` 可省略。

### 7.4 `match`

语法：

```scala
match expr {
    case value {
        ...
    }
    else {
        ...
    }
}
```

用于等值匹配。

### 7.5 `while`

```scala
while condition {
    ...
}
```

### 7.6 `for`

```scala
for item in expr {
    ...
}
```

当前可遍历：

- `array`
- `string`
- `dict`

字典迭代时，每个元素是一个包含 `key` 与 `value` 的字典。

### 7.7 `break`

```scala
break;
```

提前终止当前循环。

### 7.8 `continue`

```scala
continue;
```

跳过当前循环剩余部分。

### 7.9 `defer`

```scala
defer {
    ...
}
```

在当前作用域退出时执行。

### 7.10 `give` / `return`

两者等价：

```scala
$value |> give;
return $value;
```

仅可在 `fn`、`flow`、`stage` 或方法体内使用。

## 8. 可调用对象

### 8.1 `fn`

定义普通函数：

```scala
fn add(a, b) {
    return $a + $b;
}
```

### 8.2 `flow`

`flow` 是 `fn` 的别名，语义完全相同。

### 8.3 `stage`

定义面向管道的处理阶段：

```scala
stage shout(suffix) {
    return $it |> upper |> concat($suffix);
}
```

`stage` 体内可通过 `$it` 访问当前输入值。

### 8.4 选择建议

- 参数驱动逻辑使用 `fn`
- 流水线步骤使用 `stage`

## 9. `type`

`type` 用于定义对象字段与方法集合。

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

行为：

- 构造参数成为对象字段
- 方法使用 `fn` 定义
- 方法体内通过 `$self` 访问当前对象

构造示例：

```scala
let user = User("Alice", 99);
```

调用示例：

```scala
$user.badge() |> emit;
```

## 10. 类型系统状态

当前没有静态类型系统，也没有类型注解语法。

`type` 在当前实现中的作用是对象结构与方法定义，不是静态类型声明。

## 11. 内建阶段参考

### 11.1 输出与状态

#### `emit`

输出当前值，并继续向后传递该值。

#### `print`

`print` 是 `emit` 的别名。

#### `show`

`show` 是 `emit` 的别名。

#### `into(name)` / `store(name)`

把当前值写入变量，并返回原值。

#### `drop`

丢弃当前值，返回 `nil`。

#### `give`

从当前函数、stage 或方法中返回当前值。

### 11.2 数值与逻辑

#### `add(x)` `sub(x)` `mul(x)` `div(x)` `mod(x)`

对当前整数输入做算术运算。

#### `min(x)` `max(x)`

在当前整数输入与参数之间取最小值或最大值。

#### `eq(x)` `ne(x)`

做相等与不等比较。

#### `gt(x)` `gte(x)` `lt(x)` `lte(x)`

做大小比较。

#### `not`

对当前值做逻辑非。

#### `default(x)`

如果当前值是 `nil`，返回 `x`，否则返回原值。

#### `choose(a, b)`

如果当前值为真，返回 `a`，否则返回 `b`。

### 11.3 字符串

#### `trim`

去除字符串首尾空白。

#### `upper` / `to_upper`

转为大写。

#### `lower` / `to_lower`

转为小写。

#### `concat(...)`

拼接字符串表示。

#### `substring(start, length)`

从当前字符串中截取子串。

#### `split(sep)`

按分隔符拆分字符串，返回数组。

#### `join(sep)`

把当前数组按分隔符连接成字符串。

### 11.4 容器与对象

#### `size` / `count`

返回字符串、数组、字典或对象的大小。

#### `append(x)` / `push(x)`

向数组末尾追加元素。

#### `prepend(x)`

向数组头部插入元素。

#### `get(key)` / `field(key)` / `at(index)`

从数组、字符串、字典或对象中读取元素。

#### `set(key, value)`

对字典或对象写入字段。

#### `keys`

返回字典或对象的键数组。

#### `values`

返回字典或对象的值数组。

#### `head`

返回数组或字符串的首元素。

#### `last`

返回数组或字符串的尾元素。

### 11.5 集合式管道

#### `map(callable, ...)`

对数组中每个元素调用指定目标，返回新数组。

#### `filter(callable, ...)`

按谓词过滤数组元素。

#### `each(callable, ...)`

逐个执行调用，但返回原数组。

#### `reduce(callable, initial, ...)`

将数组折叠为单个值。

#### `range(...)`

生成整数区间数组，支持：

- `range(end)`
- `range(start, end)`

### 11.6 类型转换

#### `str(x)`

转为字符串。

#### `int(x)`

转为整数。

#### `bool(x)`

转为布尔值。

## 12. 示例

### 12.1 基础管道

```scala
"hello" |> upper |> emit;
```

### 12.2 自定义函数

```scala
fn double(x) {
    return $x * 2;
}

21 |> double |> emit;
```

### 12.3 自定义阶段

```scala
stage wrap(left, right) {
    return concat($left, $it, $right);
}

"core" |> wrap("[", "]") |> emit;
```

### 12.4 变量与循环

```scala
let count = 5;
let sum = 0;

while $count > 0 {
    let sum = $sum + $count;
    let count = $count - 1;
}

$sum |> emit;
```

### 12.5 对象与方法

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
