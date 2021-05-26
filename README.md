# OC Runtime 学习

[苹果源码开放平台](https://opensource.apple.com/)

当前学习的代码 [源码objc4-824](https://opensource.apple.com/source/objc4/objc4-824/)

Objective-C

主题我们现在需使用的Objective-C代码是Objective-C2.0。网络上搜索的大部分教程都是Objective-C1.0的教程。其实他们的实现思路大致是一样的。



## isa指针

神图镇压

![isa](/images/isa.png)

**instance 对象的 isa 的值是 class 对象，class 对象的 isa 的值是 meta-class对象。**

1. isa
    - instance 的 isa 指向 class
    - class 的 isa 指向 meta-class
    - meta-class 的 isa 指向根类的 meta-class

2. superclass
    - class 的 superclass 指向父类的 class，如果没有父类，superclass 为 nil
    - meta-class 的 superclass 指向父类的 meta-class，根类的 meta-class 的 super 指向根类的 class

3. 方法调用轨迹 **消息传递**
    - 实例对象， 调用实例方法: 通过isa指针找到class，先查方法缓存列表，然后在查方法列表。如果没有，通过superclass去到父类的方法缓存列表，然后在查方法列表。一直找到根class，根class没有，进入消息动态解析阶段，还是没有处理，就直接死掉。

    - 元类对象，调用类方法: 通过isa指针找到meta-class，先查方法缓存列表，然后在查方法列表。如果没有，通过superclass去到父类的方法缓存列表，然后在查方法列表。一直找到根meta-class，根meta-class没有，进入消息动态解析阶段，还是没有处理，就直接死掉。

    *在查找方法的时候，找了到调用的方法，就会去缓存到最开始查找的class、meta-class缓存列表中*

在源码分析上,需要清楚制度 **类 元类 类创建的对象**

在objc.h文件中对象的定义

```Objective-C

/// An opaque type that represents an Objective-C class.
typedef struct objc_class *Class;

/// Represents an instance of a class.
struct objc_object {
    Class _Nonnull isa  OBJC_ISA_AVAILABILITY;
};

/// A pointer to an instance of a class.
typedef struct objc_object *id;

```

在[objc-private.h](https://opensource.apple.com/source/objc4/objc4-824/runtime/objc-private.h)文件中对`objc_object`的定义。

```c++
struct objc_object {
private:
    isa_t isa;
public:
    /*
    more functoin
    ...
    */
}
```

相同文件中联合体 `isa_t` 定义。isa_t中包含了 对象引用计数 弱引用管理 是否存在关联对象

```c++
#include "isa.h"

union isa_t {
    isa_t() { }
    isa_t(uintptr_t value) : bits(value) { }

    uintptr_t bits;

private:
    // Accessing the class requires custom ptrauth operations, so
    // force clients to go through setClass/getClass by making this
    // private.
    Class cls;

public:
#if defined(ISA_BITFIELD)
    struct {
        ISA_BITFIELD;  // defined in isa.h
    };

    bool isDeallocating() {
        return extra_rc == 0 && has_sidetable_rc == 0;
    }
    void setDeallocating() {
        extra_rc = 0;
        has_sidetable_rc = 0;
    }
#endif

    void setClass(Class cls, objc_object *obj);
    Class getClass(bool authenticated);
    Class getDecodedClass(bool authenticated);
};
```



着重分析[isa.h](https://opensource.apple.com/source/objc4/objc4-824/runtime/isa.h)文件

下面我们来看下在arm64架构中关于isa的定义

OC中所有的实例对象、类对象和元类对象中都一个名为isa的成员变量，他们通常把它叫isa指针，既然是指针，那里面存储的应该就是一个地址。在以前的32位系统中，isa确实就是存储的一个地址，实例对象的isa存储的是其对应的类对象的地址，类对象的isa存储的是其对应的元类对象的地址，元类对象的isa存储的是根元类对象的地址。

但是在现在的64位系统(arm64架构)中，苹果对isa做了优化，里面除了存储一个地址外还存储了很多其他信息。一个指针占8个字节，也就是64位，苹果只用了其中的33位来存储地址，其余31位用来存储其他信息。


下面我们来看下在arm64架构中关于isa的定义：


```c++
#     define ISA_MASK        0x0000000ffffffff8ULL
#     define ISA_MAGIC_MASK  0x000003f000000001ULL
#     define ISA_MAGIC_VALUE 0x000001a000000001ULL
#     define ISA_HAS_CXX_DTOR_BIT 1
#     define ISA_BITFIELD                                                      \
        uintptr_t nonpointer        : 1;                                       \
        uintptr_t has_assoc         : 1;                                       \
        uintptr_t has_cxx_dtor      : 1;                                       \
        uintptr_t shiftcls          : 33; /*MACH_VM_MAX_ADDRESS 0x1000000000*/ \
        uintptr_t magic             : 6;                                       \
        uintptr_t weakly_referenced : 1;                                       \
        uintptr_t unused            : 1;                                       \
        uintptr_t has_sidetable_rc  : 1;                                       \
        uintptr_t extra_rc          : 19
#     define RC_ONE   (1ULL<<45)
#     define RC_HALF  (1ULL<<18)
```

上面信息中定义的像ISA_MASK这种常量我们不用管，这些都是程序在操作isa的过程中要用到的，比如我们将isa和ISA_MASK进行按位与运算isa & ISA_MASK就可以得到isa中存储的地址值。

我们主要关注一下ISA_BITFIELD定义数据类型：

- nonpointer：(isa的第0位(isa的最后面那位)，共占1位)。为0表示这个isa只存储了地址值，为1表示这是一个优化过的isa。
- has_assoc：(isa的第1位，共占1位)。记录这个对象是否是关联对象,没有的话，释放更快。
- has_cxx_dtor：(isa的第2位，共占1位)。记录是否有c++的析构函数，没有的话，释放更快。
- shiftcls：(isa的第3-35位，共占33位)。记录类对象或元类对象的地址值。
- magic：(isa的第36-41位，共占6位)，用于在调试时分辨对象是否完成初始化。
- weakly_referenced：(isa的第42位，共占1位)，用于记录该对象是否被弱引用或曾经被弱引用过，没有被弱引用过的对象可以更快释放。
- deallocating：(isa的第43位，共占1位)，标志对象是否正在释放内存。
- has_sidetable_rc：(isa的第44位，共占1位)，用于标记是否有扩展的引用计数。当一个对象的引用计数比较少时，其引用计数就记录在isa中，当引用计数大于某个值时就会采用sideTable来协助存储引用计数。
- extra_rc：(isa的第45-63位，共占19位)，用来记录该对象的引用计数值-1(比如引用计数是5的话这里记录的就是4)。这里总共是19位，如果引用计数很大，19位存不下的话就会采用sideTable来协助存储，规则如下：当19位存满时，会将19位的一半(也就是上面定义的RC_HALF)存入sideTable中，如果此时引用计数又+1，那么是加在extra_rc上，当extra_rc又存满时，继续拿出RC_HALF的大小放入sideTable。当引用计数减少时，如果extra_rc的值减少到了0，那就从sideTable中取出RC_HALF大小放入extra_rc中。综上所述，引用计数不管是增加还是减少都是在extra_rc上进行的，而不会直接去操作sideTable，这是因为sideTable中有个自旋锁，而引用计数的增加和减少操作是非常频繁的，如果直接去操作sideTable会非常影响性能，所以这样设计来尽量减少对sideTable的访问。


在[objc-runtime-new.h](https://opensource.apple.com/source/objc4/objc4-824/runtime/objc-runtime-new.h)对`objc_class`定义。

```c++
struct objc_class : objc_object {
    // Class ISA;
    Class superclass;
    cache_t cache;             // formerly cache pointer and vtable
    class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags
}
```




结构体分析

### 实例

### 类

### 元类

## 消息

### 消息传递

### 消息解析

- 消息动态解析

- 消息对象转发

- 消息动态解析

## 内存管理

### 引用计数

### 弱引用