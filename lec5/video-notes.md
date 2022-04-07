
1. 内核里的`printf`不会使用到系统调用，所以syscall-lab里面的trace不会因为printf调用write而递归trace以至于爆栈
2. 函数栈 stack
   1. 向下生长
   2. `return address` 和 `frame pointer`(指向前一个栈帧的指针) 一定在最前
   3. `sp` -> 栈最低端，  `fp` -> 当前栈帧最顶端
   4. 函数调用链`a->b->c`, b会在调用c的时候把 *返回地址*寄存器`ra`的值 压到栈里，然后在调用完后重新将 *返回地址* 加载到`ra`
3. 所以寄存器是分为 `caller saved` 和 `callee saved`
   1. `caller` 在下层函数调用时可能被覆盖
   2. `callee` 在下层函数调用时会保留
4. gdb - `struct Person{}`, `struct Person *p`, 可以 `p *p`查看结构体内容


caller callee saved register：
```
reg    | name  | saver  | description
-------+-------+--------+------------
x0     | zero  |        | hardwired zero
x1     | ra    | caller | return address
x2     | sp    | callee | stack pointer
x3     | gp    |        | global pointer
x4     | tp    |        | thread pointer
x5-7   | t0-2  | caller | temporary registers
x8     | s0/fp | callee | saved register / frame pointer
x9     | s1    | callee | saved register
x10-11 | a0-1  | caller | function arguments / return values
x12-17 | a2-7  | caller | function arguments
x18-27 | s2-11 | callee | saved registers
x28-31 | t3-6  | caller | temporary registers
pc     |       |        | program counter
```