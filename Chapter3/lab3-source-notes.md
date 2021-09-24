1. `panic`的时候会死循环，所以调试的时候最好使用`make qemu-gdb`配合`gdb-multiarch`把断点放到`panic`(kernel/printf.c:119)函数
2. 注意`allocproc`函数中释放进程的锁：`release(&p->lock)`
3. `kvminithart()`应当在`intr_on()`之前