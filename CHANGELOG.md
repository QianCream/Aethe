# Aethe 更新说明

本文档按阶段整理 Aethe 在最近几轮开发中的主要更新内容。

这些版本号用于表达功能演进顺序，便于阅读和归档；它们不是基于 Git tag 自动生成的发布记录。

当前工作区对应的状态可视为 `2.1.0`。

## 版本索引

- `2.1.0`：管道组合子成型，开始把编排本身变成值
- `2.0.0`：管道值一等公民化，Aethe 2 起点
- `1.13.0`：字段变形管道初步成型
- `1.12.0`：字段驱动的记录管道进一步成型
- `1.11.0`：管道标识符驱动的数据编排补强
- `1.10.0`：更有管道味的编排阶段补齐
- `1.9.0`：标准输入能力补齐
- `1.8.0`：字典与对象结构操作补齐
- `1.7.0`：排序、去重、截取阶段补齐
- `1.6.0`：集合查询、聚合能力补强，新手教程补全
- `1.5.0`：字符串与复合值标准库扩展，Doxygen 中文注释落地
- `1.4.0`：REPL 运行模型定稿，命名统一为 Aethe
- `1.3.0`：语言从纯管道 DSL 扩展为可编程解释型语言
- `1.2.0`：单文件化、C++11 化、纯管道核心建立
- `1.1.0`：语法方向重构，正式转向全管道设计
- `1.0.0`：早期原型基线

## 2.1.0

### 主题

管道组合子成型，开始把编排本身变成值。

### 新增

- 新增 `bind(callable, ...)`，把可调用目标和额外参数绑定成单输入 `pipe` 值
- 新增 `chain(callable1, callable2, ...)`，把多个可调用目标串成一条可复用管道
- 新增 `branch(callable1, callable2, ...)`，把同一输入分流到多条路线并收集结果数组
- 新增 `guard(test, on_true, on_false?)`，按谓词结果在两条路线间切换

### 语义说明

- 这组组合子既能以普通调用形式返回 `callable`，也能直接作为管道阶段即时应用
- `bind(get, name)`、`bind(add, 10)` 这类写法可以把“带参数 stage”转成可传递的管道值
- `chain(trim, upper, bind(concat, "!"))` 让“管道模板”第一次可以直接按值保存和复用
- `branch(type_of, str, bind(concat, "!"))` 让单次输入可以在一条语句里扇出成多路结果
- 高阶阶段里的 `callable` 现在同时接受 stage 名、普通 callable 名如 `type_of`，以及 `pipe` / 组合子值

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的版本定位、能力概览和最小示例
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的可调用对象、组合子与集合式管道章节
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的匿名管道与组合子教程

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `bind(add, 10)(7)`、`map(bind(add, 10))`、`chain(...)`、`branch(...)`、`guard(...)`
- 回归验证了 `pipe` 闭包、`map(pipe(...))`、`reduce(pipe(acc, item) { ... })` 与 `type_of($callable)`

### 升级提示

- 如果你之前为了给 `map` / `filter` / `group_by` 传参而额外写一个小 `fn`，现在优先改成 `bind(...)`
- 如果你之前反复复制同一段管道步骤，现在优先改成 `chain(...)` 生成可复用路线
- 如果你之前为了观察多种派生结果而拆成多段临时变量，现在优先考虑 `branch(...)`
- 如果你之前通过 `when` 手动包一层小 `fn` 做条件分流，现在优先考虑 `guard(...)`

## 2.0.0

### 主题

管道值一等公民化，Aethe 2 起点。

### 新增

- 新增匿名 `pipe(param1, param2, ...) { ... }` 表达式
- 匿名 `pipe` 可以存入变量、作为参数传递、作为普通调用目标、也可以直接挂在管道右侧
- `map`、`filter`、`flat_map`、`group_by`、`reduce`、`tap`、`evolve`、`derive` 等阶段现在都同时接受“名字”和 `pipe` 值

### 语义说明

- `pipe(...) { ... }` 的参数通过 `$name` 读取，语义上接近匿名 `fn`
- `pipe` 会捕获定义时可见的变量，因此可以自然形成闭包
- `21 |> $double`、`$double(21)`、`map(pipe(x) { ... })` 都是合法写法
- `_` 的语义不变，仍然只是“当前管道输入槽位”；如果你需要匿名可调用对象，应使用 `pipe(...) { ... }`

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的语言定位、能力概览和示例
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的表达式、可调用对象、集合式管道和限制说明
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的匿名管道与高阶管道章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `pipe` 变量调用、`map(pipe(...))`、`reduce(pipe(acc, item) { ... })`
- 回归验证了闭包捕获和把 `pipe` 直接用作管道目标

### 升级提示

- 如果你之前为了传给 `map` / `filter` / `reduce` 而额外声明一次 `fn`，现在可以直接内联成 `pipe(...) { ... }`
- 如果你之前把 `_` 当匿名函数来想，现在应明确区分：`_` 是占位符，`pipe(...) { ... }` 才是匿名可调用对象

## 1.13.0

### 主题

字段变形管道初步成型。

### 新增

- 新增 `evolve(key, callable, ...)`，对已有字段按可调用目标做值变换
- 新增 `derive(key, callable, ...)`，基于整条记录派生新字段或覆盖已有字段

### 语义说明

- `evolve` 与 `derive` 同时支持单条字典 / 对象，以及“字典 / 对象数组”输入
- `evolve(key, callable, ...)` 会把字段值送入 callable，再把返回值写回该字段
- `derive(key, callable, ...)` 会把整条记录送入 callable，再把返回值写到目标字段
- 结果统一返回 `dict` 或 `dict` 数组，这样记录变形保持纯管道风格，不直接修改对象实例
- 字段名和 callable 名都推荐直接写成裸标识符，例如 `evolve(name, upper)`、`derive(label, badge)`

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力概览和最小示例
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的管道标识符、复合值阶段与集合式管道章节
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的记录数组章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `evolve`、`derive`

### 升级提示

- 如果你之前要先 `pluck(name) |> map(upper)` 再想办法塞回记录，现在优先直接用 `evolve(name, upper)`
- 如果你之前要手写 `map` + `merge` 为记录补派生字段，现在优先改用 `derive(new_key, callable)`

## 1.12.0

### 主题

字段驱动的记录管道进一步成型。

### 新增

- 新增 `sort_by(key)`，按记录字段升序排序数组
- 新增 `sort_desc_by(key)`，按记录字段降序排序数组
- 新增 `distinct_by(key)`，按记录字段去重并保留首次出现项
- 新增 `sum_by(key)`，对记录数组中的整数字段求和

### 语义说明

- 以上阶段都要求输入为字典或对象数组
- `sort_by(key)` 与 `sort_desc_by(key)` 要求字段类型一致，且只能是 `int`、`string`、`bool`
- `distinct_by(key)` 按字段值去重，保留第一次出现的原始记录
- `sum_by(key)` 要求字段值为 `int`
- 这些阶段的字段名仍推荐直接写成裸标识符，例如 `sort_by(score)`、`distinct_by(role)`、`sum_by(score)`

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力概览和最小示例
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的集合式管道章节和内建索引
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的记录数组章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `sort_by`、`sort_desc_by`、`distinct_by`、`sum_by`

### 升级提示

- 如果你之前先 `pluck(score)` 再 `sort`，现在优先直接用 `sort_by(score)`
- 如果你之前要手写“按字段去重”的循环，现在优先改用 `distinct_by(key)`
- 如果你之前通过 `pluck(score) |> sum` 汇总整数字段，现在优先改用 `sum_by(score)`

## 1.11.0

### 主题

管道标识符驱动的数据编排补强。

### 新增

- 新增 `pluck(key)`，用于从“数组中的字典 / 对象”批量提取同名字段
- 新增 `where(key, value)`，用于按字段值筛选“数组中的字典 / 对象”
- 新增 `index_by(key)`，用于把“数组中的字典 / 对象”按字段值重组为字典
- 新增 `count_by(key)`，用于统计“数组中的字典 / 对象”在某个字段上的出现次数
- 新增 `rename(from, to)`，用于在字典或对象视图上重命名字段并返回新字典

### 语义说明

- `pluck(key)` 要求输入为字典或对象数组；缺失键会得到 `nil`
- `where(key, value)` 要求输入为字典或对象数组；比较规则与 `==` 一致
- `index_by(key)` 要求输入为字典或对象数组；字段值必须能作为字符串键，重复键以后项覆盖前项
- `count_by(key)` 要求输入为字典或对象数组；字段值必须能作为字符串键，结果值为整数计数
- `rename(from, to)` 接受字典或对象输入；若 `from` 不存在则保持原样，结果统一返回 `dict`
- 以上阶段的键名和筛选值都可以直接写成裸标识符，例如 `where(role, admin)`、`pluck(name)`、`index_by(name)`、`count_by(role)`、`rename(score, total_score)`
- `name`、`role`、`admin` 这类裸标识符仍然只是符号值；只有 `$name`、`$role` 才表示变量读取

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的语言定位、能力概览和最小示例
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的“变量与裸标识符”、复合值阶段与集合式管道章节
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的字典章节与集合式管道章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `pluck`、`where`、`index_by`、`count_by`、`rename`

### 升级提示

- 如果你之前要先写 `fn` 才能从记录数组里取字段，现在优先改用 `pluck(name)` 这类标识符驱动阶段
- 如果你之前通过 `filter` + 手写字段比较筛选记录数组，现在优先考虑 `where(role, admin)` 这类写法
- 如果你之前通过 `group_by` 之后再手动取单项或计数，现在优先考虑 `index_by(name)` 和 `count_by(role)`
- 如果你之前通过 `set` + `omit` 手动改字段名，现在优先改用 `rename(old_key, new_key)`

## 1.10.0

### 主题

更有管道味的编排阶段补齐。

### 新增

- 新增 `tap(callable, ...)`，让你在管道中观察或触发副作用，同时保留原值继续流动
- 新增 `chunk(size)`，按固定长度切分字符串或数组
- 新增 `zip(other)`，把两路数组按位置配对
- 新增 `flat_map(callable, ...)`，对数组逐项映射并立即展开一层
- 新增 `group_by(callable, ...)`，按可调用结果把数组重新分桶成字典

### 语义说明

- `tap` 接受任意输入，调用完成后仍返回原输入
- `chunk` 支持字符串和数组，且 `size` 必须是正整数
- `zip` 的结果长度取左右两侧数组长度的较小值
- `flat_map` 要求 callable 返回数组
- `group_by` 要求 callable 返回字符串键，每组内保持原始顺序

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力概览和示例
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的复合值阶段与集合式管道章节
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的集合式管道章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `tap`、`chunk`、`zip`、`flat_map`、`group_by`

### 升级提示

- 需要在长管道里看中间结果时，优先用 `tap(emit)`，不要拆成多段临时变量
- 需要“映射后再 flatten”时，优先改用 `flat_map`
- 需要按规则重组数组时，优先考虑 `group_by` 与 `chunk` / `zip`

## 1.9.0

### 主题

标准输入能力补齐。

### 新增

- 新增 `input()`，用于从标准输入读取一整行文本
- 新增 `input(prompt)`，读取前先输出提示文本

### 语义说明

- REPL 模式仍然保持“缓冲区录入后手动输入 `run`”的执行方式
- `input()` 读取结果不包含行尾换行符
- `input()` 读到 EOF 时返回 `nil`
- `input(prompt)` 中的 `prompt` 必须是字符串

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力概览
- 更新 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的可调用对象与 REPL 说明
- 更新 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的常用内建函数章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 验证了 REPL 模式下的 `run` 缓冲执行
- 验证了 `input()` 与 `input(prompt)`

### 升级提示

- 需要读取用户输入时，优先使用 `input()`，不要把交互数据硬编码进源码
- 如果你需要提示文本，使用 `input("prompt> ")`，而不是先 `emit` 再单独读入

## 1.8.0

### 主题

字典与对象结构操作补齐。

### 新增

- 新增 `entries`，将字典或对象展开为 `{key, value}` 数组
- 新增 `pick(key, ...)`，按给定字段名提取子集
- 新增 `omit(key, ...)`，按给定字段名移除子集
- 新增 `merge(other)`，把另一份字典或对象字段合并进当前结构

### 语义说明

- `entries` 接受 `dict` 或 `object` 输入，返回数组
- `pick`、`omit`、`merge` 接受 `dict` 或 `object` 输入，但结果统一返回 `dict`
- `merge(other)` 在键冲突时采用 `other` 覆盖 `input` 的规则

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力概览
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的复合值阶段、索引和示例
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的字典章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `entries`、`pick`、`omit`、`merge`

### 升级提示

- 如果你之前通过 `keys |> map(...)` 或手写循环改造字典，现在优先改用 `entries`、`pick`、`omit`、`merge`
- 如果你把对象当结构记录使用，现在可以直接把对象输入这些阶段，但结果会显式转成 `dict`

## 1.7.0

### 主题

排序、去重、截取阶段补齐。

### 新增

- 新增 `take(count)`，支持字符串和数组截取前 `count` 项
- 新增 `skip(count)`，支持字符串和数组跳过前 `count` 项
- 新增 `distinct`，对数组去重，并保留首次出现顺序
- 新增 `sort`，支持 `int`、`string`、`bool` 数组升序排序
- 新增 `sort_desc`，支持 `int`、`string`、`bool` 数组降序排序

### 文档

- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 能力概览
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 的复合值阶段和示例部分
- 扩写 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md) 的数组与字符串章节

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `take`、`skip`、`distinct`、`sort`、`sort_desc`

### 升级提示

- 如果你之前手写循环做数组截取、去重或排序，现在优先改用内建阶段，代码更短也更一致
- `sort` 与 `sort_desc` 当前只支持元素类型一致，且类型为 `int`、`string` 或 `bool` 的数组

## 1.6.0

### 主题

集合查询、聚合能力补强，新手文档体系补全。

### 新增

- 新增 `index_of(x)`，支持字符串和数组查找首个匹配位置；未找到时返回 `-1`
- 新增 `repeat(count)`，支持字符串和数组重复
- 新增 `sum`，用于整数数组求和
- 新增 `flatten`，用于把数组中的子数组展开一层
- 新增 `find(callable, ...)`，返回第一个满足条件的原始元素
- 新增 `all(callable, ...)`，判断数组元素是否全部满足条件
- 新增 `any(callable, ...)`，判断数组元素是否至少有一个满足条件

### 文档

- 新增 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md)，面向零基础用户，从 REPL、值、变量、管道一路讲到对象、集合式管道与练习题
- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)，补充上述新阶段的语法、输入要求、结果、错误条件与示例
- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力概览和文档导航

### 验证

- 使用 `c++ -std=c++11 main.cpp -o aethe` 编译通过
- 回归验证了 `index_of`、`repeat`、`sum`、`flatten`、`find`、`all`、`any`

### 升级提示

- 如果你之前通过 `map / filter / reduce` 处理数组，现在可以用 `find`、`all`、`any` 写出更直接的查询代码
- 如果你之前需要手写循环求和或展开嵌套数组，现在可以直接使用 `sum` 与 `flatten`

## 1.5.0

### 主题

标准库扩展第一阶段，源码注释规范化。

### 新增

- 新增 `type_of(x)`，返回运行时类型名
- 新增 `contains(x)` / `has(x)`，支持字符串、数组、字典、对象的包含性检查
- 新增 `starts_with(prefix)` 与 `ends_with(suffix)`，补足字符串前后缀判断
- 新增 `replace(from, to)`，进行非重叠全量替换
- 新增 `slice(start, length)`，支持字符串和数组切片
- 新增 `reverse`，支持字符串和数组逆序

### 工程

- 给解释器核心结构补充了 Doxygen 风格注释
- 注释覆盖了词法器、运行时值、解析器、解释器和 REPL 入口
- 注释统一改为中文表述，避免仓库内部出现中英文混杂的接口说明

### 文档

- 扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md) 中的字符串与复合值阶段条目
- 更新 [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 的能力摘要

### 验证

- 使用 REPL 验证了 `contains`、`starts_with`、`ends_with`、`replace`、`slice`、`reverse`、`type_of`

### 升级提示

- 如果你之前用组合表达式手写字符串查找、切片和替换，优先改用新内建阶段，语义更稳定，文档也更完整
- 如果你需要在 REPL 中调试值类型，现在建议直接使用 `type_of(x)`

## 1.4.0

### 主题

交互式运行模型定稿，语言命名与使用方式统一到 Aethe。

### 变更

- 语言名称统一改为 `Aethe`
- 可执行文件名统一改为 `aethe`
- REPL 主提示符改为 `>>>`
- 续行提示符保留为 `...>`
- 输入不再自动执行，只有输入单独一行 `run` 才会统一执行当前缓冲区
- 交互模型从“逐句立刻执行”调整为“缓冲区式执行”
- `_` 的语义固定为“当前管道输入槽位”，明确不再承担匿名函数含义

### 工程

- 彻底移除旧的项目命名残留，把 `F++ / fpp` 相关命名迁移为 `Aethe`
- 调整 REPL 的完整块检测逻辑，支持多行定义后统一执行

### 文档

- [README.md](/Users/armand/Personal/CLion/Aethe/README.md) 重写为仓库入口页
- 新增并持续扩写 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)，将语言说明改成参考手册风格

### 兼容性说明

- 旧的文件执行方式不再支持
- 旧项目名 `F++` 已不再作为当前名称使用
- 依赖 `_` 充当匿名函数的写法不再成立；`_` 只能用于管道右侧

### 迁移建议

- 旧项目文档、示例、构建脚本中的 `F++`、`fpp`、`fpp::` 都应统一替换为 `Aethe`、`aethe`、`aethe::`
- 旧的“输入一行立即执行”使用习惯需要改成“录入缓冲区后输入 `run`”
- 如果历史示例把 `_` 当匿名函数使用，需要改写为普通 `fn`，或者保留在管道右侧作值占位

## 1.3.0

### 主题

语言从基础管道 DSL 扩展为可编程解释型语言。

### 新增

- 新增 `flow` / `fn`，用于定义可复用函数
- 新增 `stage`，用于定义天然适合挂在管道后的阶段
- 新增 `type`，用于定义对象结构和方法集合
- 新增 `$self` 方法上下文和对象构造语义
- 新增 `when / else`
- 新增 `match / case / else`
- 新增 `while`
- 新增 `for item in expr`
- 新增 `break`
- 新增 `continue`
- 新增 `defer`
- 新增 `give` / `return`
- 新增一元与二元运算、成员访问、调用表达式

### 管道语义

- 支持自动首参注入，左侧值会默认成为右侧调用目标的第一个参数
- 支持 `_` 精确占位，让输入值进入指定参数位置
- 支持同一表达式中多次使用 `_`
- 支持裸表达式作为管道目标，例如 `10 |> _ * 3 + 5`

### 标准能力

- 增加 `map`
- 增加 `filter`
- 增加 `each`
- 增加 `reduce`

### 语法兼容层

- 增加 `fn` 作为 `flow` 的常用别名
- 增加 `return` 作为 `give` 的语法糖
- 增加 `let` 作为变量写入语法糖

### 影响

- Aethe 从“只能做管道值变换”的 DSL 进入“可以组织逻辑、建模对象、写循环和方法”的阶段
- 从这一版本开始，参考文档不再只需要列出阶段名称，还必须描述语句、作用域和返回行为

## 1.2.0

### 主题

单文件化、C++11 化，并建立首个纯管道核心。

### 工程

- 将实现收敛为单文件 [main.cpp](/Users/armand/Personal/CLion/Aethe/main.cpp)
- 删除多文件头文件和源文件布局
- 构建目标收敛为标准 C++11 单文件程序

### 语言核心

- 建立 `source |> stage |> stage(...);` 的核心管道形式
- 建立 `$name` 变量读取语法
- 保留裸标识符作为符号字符串，而不是隐式变量
- 支持数组、字典、字符串、整数、布尔值、`nil`

### 初始内建能力

- 增加 `into` / `store`
- 增加 `emit` / `print` / `show`
- 增加 `drop`
- 增加基础数值阶段，如 `add`、`sub`、`mul`、`div`
- 增加基础字符串阶段，如 `trim`、`upper`、`lower`、`concat`
- 增加基础复合值阶段，如 `size`、`get`、`set`、`keys`、`values`

### 设计方向

- 语言中心从传统嵌套调用转向显式数据流
- 优先保证管道语义清晰，而不是兼容旧原型语法

### 影响

- 单文件结构使实现更容易阅读和调试，也让后续 REPL 化和文档收敛更直接
- 这一版奠定了后续所有语法扩展的值模型与执行模型

## 1.1.0

### 主题

从早期原型迈向新语法方向的首次重构。

### 变更

- 明确语言目标为“以全管道标识为核心的数据流语言”
- 代码清理，移除明显臃肿部分，统一语法和数据组织方向
- 注释策略改为只保留 Doxygen 风格注释，不再保留普通实现注释
- 为后续单文件化、REPL 化和参考手册化奠定基础

### 影响

- 旧原型中零散、试验性的语法方向被逐步废弃
- 新版本的语法和运行模型开始围绕管道、一致的值模型和内建阶段展开

## 1.0.0

### 主题

早期原型基线。

### 特征

- 项目仍处于语法试验阶段
- 语言目标和命名尚未完全固定
- 工程结构和运行模型尚未收敛到单文件、REPL、参考手册这一路线

### 说明

- `1.0.0` 主要用于在更新说明里标出“重构前”的起点
- 从 `1.1.0` 开始，Aethe 才进入连续的设计收敛阶段

## 版本阅读建议

- 如果你想快速知道当前能做什么，先看 [README.md](/Users/armand/Personal/CLion/Aethe/README.md)
- 如果你想系统学习 Aethe，先看 [TUTORIAL.md](/Users/armand/Personal/CLion/Aethe/TUTORIAL.md)
- 如果你想查某个语法或内建的精确定义，查 [REFERENCE.md](/Users/armand/Personal/CLion/Aethe/REFERENCE.md)
