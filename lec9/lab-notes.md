
lab copy-on-fork

- `uvmcopy`拷贝页表时不分配物理内存，父子进程都要清空PTE_W标志
- `usertrap`里面对`page fault`进行单独处理
  - `kalloc`分配新page，将旧的page拷贝到新的page，然后设置page table里面的PTE_W标志
- 确保每个page在最后一个引用他的进程退出后正确释放
  - 每个page需要引用计数
  - 内核里面一个数组保存这些引用计数
    - 数组大小：物理page数目最大值 MAXVA
      - `KERNEL_BASE = 0x80000000`, `PHYSTOP=(KERNBASE + 128*1024*1024)`
      - 物理地址 `KERNEL_BASE -> PHYSTOP`
      - 大小 `PHYSTOP - KERNEL_BASE = 128*1024*1024 Byte = 128*256 Page = 32768 Page`
    - 数组下标：`(phy_addr - KERNEL_BASE) / PGSIZE`
    - kalloc的时候ref设置为1
    - fork的时候每个page的ref加1
    - 每个进程释放内存`freeproc`的时候ref减1
    - `kfree`只在page的ref为0时将其放回`freelist`中
- `copyout`的时候要检查是否为COW的page，是的话也要同上述page fault一样处理
- 可以使用PTE的`RSW`寄存器来记录是否为COW页
- 如果出现了COW页但是没有物理内存了，kill进程

实现

- PTE增加COW标志
  - RSW寄存器包含 8/9 两个，使用8 `PTE_COW 1 << 8`
- page引用计数
  - 增加`int refcnt[]` 大小 `(PHYSTOP - KERNEL_BASE) / PGSIZE = 32768`
  - `kalloc()`将 `refcnt[]` 设置为1
  - fork只拷贝页表
    - `uvmcopy`: 父子进程都清除PTE_W，并设置PTE_COW，pa的`refcnt += 1`
    - `uvmunmap`的时候判断将page的`refcnt[] -= 1`，若`refcnt[] == 0`才`kfree`
- copy-on-write
  - page fault的时候 分配+拷贝+设置可读
    - `scause == 15` 表示 store page fault, `stval`为虚拟地址
    - 如果`r_scause() == 15/store fault`
      - 使用`walk(p->pgtbl, stval)`获取相应的PTE
      - 如果PTE_COW位设置了，则表示为cow页
        - cow-alloc，如果失败 myproc()->killed=1 
  - copyout的时候，如果目标物理地址是PTE_COW，分配+拷贝+设置可读
    - walk获得PTE
    - 如果设置了PTE_V PTE_U PTE_COW
      - `cow-alloc`，失败返回-1
- cow-alloc
  - kalloc分配，如果失败，返回-1
  - memmove拷贝，设置 PTE_W，取消PTE_COW
  - 将原pa的`refcnt[] -= 1 `，如果refcnt为0，kfree掉
  - 返回 0

