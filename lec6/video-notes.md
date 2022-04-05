
1. qemu 能够查看当前页表
   1. `ctrl a + c` 进入qemu的console
   2. `info mem` 查看page table内容
2. RISC-V每个页表的PTE项包含 A D 两个flag，用来辅助OS淘汰页面
   1. A `1 << 6` 表示该 PTE 是否被 access 过 -> access
   2. D `1 << 4` 表示是否被 write 过 -> dirty
3. `csrrw a0,sscratch,a0` 交换 scratch 和 a0 的值
   1. 交换前：`sscratch` 包含的是 trapframe地址, `a0` 包含的是系统调用的参数
   2. 交换后：`a0`是trapframe，用来保存其它寄存器，`sscratch`是参数，后续会再被换到a0
   3. 在回到用户态前的最后一步`usertrapret`中会调用`userret(trampoline.S)`将 `sscratch` 设置为trapframe - 因此sscratch总是包含trapframe地址
4. `tp`寄存器保存当前的CPU核心hart
5. 切换pagetable为何不会crash？ trampoline在user和kernel的pagetable都有映射
6. 在uservec中，`stvec`会被写为`kernelvec`，用来处理内核的trap
7. `sret`会启用中断，将pc设置为sepc
