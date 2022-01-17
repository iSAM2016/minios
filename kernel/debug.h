#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
/* 打印文件名 行号 函数名 条件并使程序悬停*/
void painc_spin(char *filename, int line, const char *func, const char *condition)
/***************************  __VA_ARGS__  *******************************  * __VA_ARGS__ 是预处理器所支持的专用标识符
 * 代表所有与省略号相对应的参数。
 * ... 代表定义 的红其参数可变
 * TODO: __VA_ARGS__
 * 
 * __FILE__, __LINE__, __func__ 是预定义的宏，被编译的函数名
 * 被编译的文件名， 被编译文件的行号
 */
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
#define ASSERT(CONDITION) ((void)0)
#else
// 定义多行洪使用\ 换行
#define ASSERT(CONDITION)  \
    \if (CONDITION) {}     \
    else                   \
    { \ 
/* 符号#让编译器将宏的参数转化为字符串字面量  跳转到 panic_spin
PANIC 的形式为 PANIC(#CONDITION) ，即形参为#CONDITION，其中字符’#’的作用是让预处理器把CONDITION转换成字符串常量。
*/                   \
        PANIC(#CONDITION); \
        \  
                                                                                                                                    \
    \   
}
#endif /*NDEBUF*/
#endif //__KERNEL_DEBUG_H