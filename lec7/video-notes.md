
**Page Fault** 的应用

1. virtual memory 好处
   1. 隔离 - 进程之间的隔离，内核与用户态隔离
   2. 间接性(Indirection) - va->pd 的映射
      1. trampoline 
      2. guard page
   3. 静态的 - 不需要改变
2. page fault 使vm具有 动态性 - change indiretion on the fly
3. page fault所需的信息
   1. fault的页面 - `stval`寄存器
   2. fault的类型 - R/W/Instrution, `scause`寄存器
   3. 造成fault的instruction/address - `sepc`寄存器，保存在 `trapframe` 
4. lazy allocation简单实现
   1. sbrk的时候直接把 p->sz 增大
   2. usertrap里面检查`r_scause==15`，15表示store page fault
   3. 如果为15则kalloc分配内存，然后`uvmmappages`添加虚拟地址映射（`stval`保存的是虚拟地址，注意要PGROUNDUP）
   4. 但是这样freeproc的时候，因为 `p->sz` 包含了没有分配的va，所以会出现 `not mapped` 问题
   5. 所以修改 `uvmunmap` 在 `*pte & PTE_V == 0` 的时候直接 continue
   6. 问题 - sz小于0（shrink）情况会崩；需要检查 p->sz 和 va 的大小关系
5. zero filled on demand
   1. BSS 数据段应当为0
   2. 实现
      1. 分配一个全0的物理页，把虚拟地址全映射到这个物理页；映射为 **READ-ONLY**
      2. 出现page fault的时候再映射
   3. 优势-与 lazy allocation 类似
6. page fault也是要付出代价 - 内核到用户态转换
7. Copy-on-write Fork （COW Fork）
   1. 简单实现
      1. fork的时候不分配物理地址
      2. 把父子进程的虚拟地址都映射为 **READ-ONLY**
      3. page fault的时候，分配新物理地址，然后把他们映射为 **READ-WRITE**
   2. RISC-V PTE 的 RSW flag保留给supervisor模式，可以用来标志 COW 的page
   3. 可能有多个进程共享一个 page，所以需要 ref count 引用计数
8. Demand Paging - exec
   1. 程序并不总是使用所有代码
   2. 简单实现
      1. 不预先加载代码段
      2. 标记这些代码段的文件
      3. page fault的时候加载文件并映射
9. 物理地址不够的时候需要 淘汰/替换 页面到磁盘上
   1. pte 的 access 标志可以用来实现 lru
   2. 但是需要定期查看这些页面的 access 状态，然后重设他们
10. Memory Map File - mmap映射，munmap写回文件