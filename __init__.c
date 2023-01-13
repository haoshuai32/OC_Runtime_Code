#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

typedef unsigned long uintptr_t;



union isa_t
{
    uintptr_t bits;
    
    char *cls;

    // 对于上传的数据 = 一下对于一下赋值
    struct {
      uintptr_t nonpointer        : 1; 
      uintptr_t has_assoc         : 1; 
      uintptr_t has_cxx_dtor      : 1; 
      uintptr_t shiftcls          : 44;
      uintptr_t magic             : 6; 
      uintptr_t weakly_referenced : 1; 
      uintptr_t unused            : 1; 
      uintptr_t has_sidetable_rc  : 1; 
      uintptr_t extra_rc          : 8;
    };


};


int main(int argc, char const *argv[])
{
    printf("hello the world\n");
    union isa_t a;
    printf("data %d\n",a.bits);
    printf("cls %d\n",a.cls);
    a.nonpointer = 1;
    a.has_assoc = 1;
    printf("size %lu \n", sizeof(a));
    printf("data %d\n",a.bits);
    printf("cls %d\n",a.cls);
    return 0;
}
