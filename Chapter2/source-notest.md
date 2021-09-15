
1. `kernel/syscall.c`中定义`uint64 (*syscalls[])(void)`数组出现的`[SYS_fork]`是什么东西？

参考[Array_initialization](https://en.cppreference.com/w/c/language/array_initialization)与[GNU_Designated_Lints](https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html)，C99引入的一个初始化数组的便捷方式，可以直接规定某(几)个下标对应的值。规范的语法是
```c
int a[6] = { [4] = 29, [2] = 15 };
// is eequivalent to
int a[6] = { 0, 0, 15, 0, 29, 0 };
```
