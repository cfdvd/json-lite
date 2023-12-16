轻量级 JSON 解析器

* 使用 variant 容器作为 JSON 数据的存放类型
* 采用重载 lambda 的形式以及 std::visitor() 实现了 variant 数据的读取
* 在读取过程中, 使用 if constexpr 特化对 JSON 数组和 JSON 对象的处理
* 使用正则表达式对"数字 (整数以及浮点数)"进行解析
* JSON 文本采用递归的形式进行解析
* 使用 optional 容器来包装返回值, 用以表示特殊情况
* 使用 std::string_view 作为函数参数, 防止了字符串拷贝
