
Trap and system calls

- 三种 trap 
  - 系统调用 ecall
  - 异常：除0，非法内存访问
  - 设备中断：如硬盘读写完成
- trap应当是透明的
- trap不应当传播到用户态
- trap处理四个阶段
  1. 硬件操作 CPU
  2. 汇编代码准备C执行环境
  3. C函数分配处理方式
  4. 系统调用/设备驱动等处理trap
- 三种处理trap的情况
  - 用户态的trap
  - 内核态的trap
  - 定时器中断
- trap首个handler也叫vector，中断处理向量

4.1 RISC-V trap Machinery

- `riscv.h`定义了xv6处理trap时使用的寄存器
  - `stvec`：trap处理函数的地址，会赋值给PC
  - `sepc`：保存原来的PC；`sret`：将`sepc`的值拷贝给PC
  - `scause`：trap的原因
  - `sscratch`：handy
  - `sstatus`：其SIE位决定 *设备中断* 是否启用；其SPP位表示中断来自于 用户态 还是 内核态，也控制了 sret 返回哪
- 这些寄存器用户态访问不了；这些寄存器在 machine mode 有一套类似的；处理器每个核都有这样一套寄存器
- RISC-V处理trap的流程
  - 如果是设备中断但是SIE位为0，什么都不干退出
  - 清空SIE位屏蔽设备中断
  - 将`pc`拷贝到`sepc`
  - 保存当前状态（用户态/内核态）到SPP
  - 设置`scause`
  - 设置为内核态
  - 将`stvec`拷贝到`pc`
  - 执行PC的内容，即中断向量stvec
- 处理器干了最少的活：没切换的内核页表、内核栈，没保存寄存器；这些活内核干；提高灵活性（某些系统可能要高性能trap处理）
  
4.2 Traps from user space

- 用户态的三种情况会发生trap：系统调用、非法操作、设备中断
- xv6处理trap的路径：uservec -> usertrap -> usertrapret -> userret
- `trampoline`：保证用户/内核页表都有中断向量`stvec`
  - RISC-V不切换页表，所以用户态中断要能够执行
- `trampoline`有PTE_U标志，用户态也可以执行
- `uservec`处理过程
  - 交换`sscratch`和a0；`sscratch`预先放置了进程的trapframe；`trapframe`(kernel/proc.h)包含了`usertrap`
  - 保存其它用户寄存器
  - 保存用户的a0到trapframe中
  - 将trapframe的内核栈指针加载到当前sp，的hartid放到tp中；的kernel_trap（实际是user_trap代码）放到t0
  - 将trapframe中的内核页表地址放到t1，使用csrw和sfence启用内核页表
  - 跳转到t0也就是trapframe的kernel_trap，实际上指向了`user_trap`(kernel/trap.c)
- `usertrap`处理过程
  - 把kernel_vec写到stvec，保证这个时候的中断由内核处理
  - 保存sepc内容（sepc保存的是trap之前的PC）
  - 如果syscall
    - epc+4保证回到系统调用的下一条指令
    - 中断打开然后开始执行系统调用
  - 如果设备中断 devintr
    - 如果是timer，就让出CPU
  - 否则就是exception，打印消息然后kill
  - 调用`usertrapret`返回
- `usertrapret`过程
  - 关中断
  - stvec的内容写为trampoline，让用户的uservec处理中断
  - 设置好进程trapframe内容 kernel_satp=satp; kernel_sp=p->stack+PGSIZE; kernel_trap=usertrap; kernel_hartid=tp;
  - 清除SPP，设置SIE；sepc=trapframe->epc；获取到进程的页表satp，调用`userret`
- `userret`过程
  - 切换到进程页表
  - 恢复寄存器，恢复a0
  - sret返回用户态

4.3 Code：Calling system calls

- initCode.S将exec的参数放到 a0 和 a1 两个寄存器，然后将 exec 的系统调用序号放到 a7
- ecall发起系统调用，执行uservec usertrap 然后系统调用
- `syscall`(kernel/syscall.c)从a7得到对应系统调用，然后执行sys_exec
- `sys_exec`将`p->trapframe->a0`设为返回值
- 返回负数-1表示错误

4.4 Code: System call arguments

- syscall的wrapper使用寄存器存储参数（和C一样）
- 指针参数的问题
  - 程序可能给系统调用恶意的参数
  - 内核pagetable和用户的pagetable不一样
- `copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)`(kernel/vm.c)实现用户态字符串到内核字符串
  - 通过`walkaddr`找到pagetable上srcva对应的物理地址pa
  - 内核页表虚拟地址和物理地址一样，所以可以直接拷贝
  - `copyout`与之相反

4.5 Traps from kernel space

- 内核中断的时候stvec指向kernelvec
  - satp为内核页表
  - sp为内核栈指针
- kernelvec将寄存器保存在栈上
  - 如果trap造成执行线程切换，则原来的线程还会正常恢复
- kernelvec执行kerneltrap，处理两种错误
  - 中断；由设备中断处理
  - 异常；直接崩 panic
  - 如果是timer中断，就yield
  - 最后将之前读取的sepc和sstatus恢复：yield时候可能嵌套中断，所以这里要恢复两个寄存器
  - 然后回到 kernelvec:48 执行，恢复其它寄存器值，然后sret
- usertrap中实际有个区间 kernel在执行但是stvec仍然是uservec
  - 如果这个时候出现设备中断就可能出问题：RISC-V开始处理trap的时候总会禁用中断
  - xv6直到设置好stvec后才会启用中断

4.6 Page-fault exceptions 缺页中断

- 三种缺页中断：
  - 加载中断：无法读取
  - 存储中断：无法存储
  - 指令中断：无法翻译虚拟地址
- 缺页中断实现Copy-on-write COW fork
  - 将fork的子进程的页表标记只读：PTE_W不可用
  - 如果存储中断，就拷贝
  - 将拷贝的改成可写
  - COW需要记录页的引用数目，知道哪个可以释放
    - 可能有多个子进程执行完成退出
    - 记录后有个优化点：如果进程缺页中断，但是当期页只有一个引用，不用拷贝
  - COW fork 更快，fork通常跟着exec，很多页不需要
  - COW fork是透明的
- 缺页中断实现 懒加载 lazy allocation
  - 进行用`sbrk`扩内存的时候不扩，中断的时候再扩
  - sbrk申请大量内存是耗时巨大，按需扩张可以分散时间消耗
- 缺页中断实现 请求式页面加载
  - exec执行很大程序，不一次全部加载，中断才加载
- 利用 paging to disk 实现 *虚拟内存*，进程需要的内存大于实际RAM
  - 将超过的部分放到磁盘的 页存储区 paging area
  - 中断了再加载
  - 如果内存满了需要 淘汰 其它的页
  - 通常根据局部性原理按照频率淘汰
- 自动扩展栈大小，内存映射文件也结合了缺页中断 ？？

4.7 Real world

- Trampoline有点过度复杂？
  - RISC-V干的活太少，xv6想实现高效trap处理
  - kernel的trap处理程序不得不在用户态执行，可能有问题
    - xv6利用了RISC-V的特性：`sscratch`寄存器
- Trampoline可以不用如果
  - 进程的页表有内核内存的映射
  - 也会消除 用户态/内核态 切换时的 页表切换：允许直接解引用syscall的指针参数
- 实际操作系统会尽量用物理内存：允许应用 / 缓存

