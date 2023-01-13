在一个64位的机器中OC的指针长度是47位。且最后三位是站位，不是真正的游泳的数据

64位一个机器 应该是 8个字节 8 x 8 = 64

1字节 == 8位

OC extension == 类别

OC category == 分类

extension看起来很像一个匿名的category，但是extension和有名字的category几乎完全是两个东西。 


# Runtime

// 
typedef struct objc_class *Class;
struct objc_class : objc_object { // Class ISA;
      objc_class * superclass;
}

struct objc_object {
<!-- Class _Nonnull isa OBJC_ISA_AVAILABILITY; -->
objc_class *isa;
    
};

typedef struct objc_class *Class; 
typedef struct objc_object *id;
typedef struct method_t *Method;
typedef struct ivar_t *Ivar;
typedef struct category_t *Category; typedef struct property_t *objc_property_t;


 