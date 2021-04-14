# Runtime源码学习

## `runtime.h`文件

路径 runtime/runtime.h

-----

## 基础知识点

### 

### Selector, Method 和 IMP 的区别与联系

[引用](https://www.jianshu.com/p/84d1771e9792)


**Selector**

定义：typedef struct objc_selector *SEL

翻译成中文叫做**选择子或者选择器**，选择子代表方法在 Runtime 期间的标识符。为 SEL 类型，虽然 SEL 是 objc_selector 结构体指针，但实际上它只是一个 C 字符串。在类加载的时候，编译器会生成与方法相对应的选择子，并注册到 Objective-C 的 Runtime 运行系统。

> 常见的有两种方式来获取/创建选择子:

```Objective-C
SEL selA = @selector(setString:);
SEL selB = sel_registerName("setString:"); 
```

在控制台测试：

```sh
(lldb) p selA
(SEL) $1 = "setString:"
(lldb) p selB
(SEL) $2 = "setString:"
```

两者打印出来的都是字符串。

我们从 sel_getName() 方法的源码可以看出 SEL 和 const char * 是可以相互转化的：

```c
const char *sel_getName(SEL sel) {
   return sel ? (const char *)sel : "<null selector>";
}
```

如果将 selA 和 selB 强转为为 const char *:

```sh
(lldb) p (const char *)$2
(const char *) $3 = 0x00007fff9a3794b5 "setString:"
(lldb) p (const char *)$1
(const char *) $4 = 0x00007fff9a3794b5 "setString:"
(lldb) 
```

上面的结果可以看出，selA 和 selB 指向相同的地址，代表同一个字符串。

如果某个类实现了 setString：方法，那么以上两个选择子传入 respondsToSelector() 方法返回的结果都将为 YES。

*不同类中相同名字的方法所对应的方法选择子是相同的，即使方法名字相同而变量类型不同也会导致它们具有相同的方法选择子。*


**Implementation(IMP):**

定义：typedef id (*IMP)(id, SEL, ...)

代表函数指针，即函数执行的入口。该函数使用标准的 C 调用。第一个参数指向 self（它代表当前类实例的地址，如果是类则指向的是它的元类），作为消息的接受者；第二个参数代表方法的选择子；... 代表可选参数，前面的 id 代表返回值。

**Method**

Method 对开发者来说是一种不透明的类型，被隐藏在我们平时书写的类或对象的方法背后。它是一个 objc_method 结构体指针，objc_method 的定义为：

```Objective-C
struct objc_method {
    SEL method_name; 
    char *method_types;
    IMP method_imp;
};
```

- 方法名 method_name 类型为 SEL，前面提到过相同名字的方法即使在不同类中定义，它们的方法选择器也相同。
- 方法类型 method_types 是个 char 指针，其实存储着方法的参数类型和返回值类型，即是 Type Encoding 编码。
- method_imp 指向方法的实现，本质上是一个函数的指针，就是前面讲到的 Implementation。


> Selector，Method，IMP 它们之间的关系可以这么解释：
**一个类（Class）持有一个分发表，在运行期分发消息，表中的每一个实体代表一个方法（Method），它的名字叫做选择子（SEL），对应着一种方法实现（IMP）。**