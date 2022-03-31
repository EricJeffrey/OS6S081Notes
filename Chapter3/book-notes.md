Page tables 页表

3.1 Paging hardware 提供分页能力的硬件

- RISC-V指令的地址操作数都为虚拟地址，page table hardware负责虚拟地址到物理地址的转换
- xv6运行在Sv39的RISC-V上：64位虚拟地址只有低39位用到
- 寻址过程：64位虚拟地址，高25位未使用，低39位作为地址
  - 低39位的高27位
    - `9+9+9`的PTE(Page Table Entry)树形结构，最终定位到一个**44位**的PPN(Physical Page Number)
    - 树形结构中，每层通过对应的9位得到下一层的物理地址（最上层地址固定）
  - 低39位的**低12位**：与上面的44位结合得到**56位**的实际物理地址
    - 低12位抽象出大小为`2^12=4096(Byte)`的页表
  - 若寻址失败则引发`page-fault`异常，由内核处理
  - 每个PTE包含一些标志位
    - PTE_V 是否存在/允许访问
    - PTE_R PTE_W 是否允许读写
    - PTE_X 是否为可执行指令
    - PTE_U usermode的指令是否可访问
    - 根page table的地址在`satp`寄存器中
    - 每个CPU都有自己`satp`寄存器，所以不同CPU可以同时运行不同进程
- 概念
  - 物理内存：指实际的DRAM内存
  - 物理地址：指实际的地址，发送给DRAM控制器读写内存
  - 虚拟地址：抽象概念，用于实现地址隔离，最终被翻译成物理地址

![RISC-V地址翻译过程](RISC-V%20address%20translation%20details.png)

3.2 Kernel address space

- xv6为每个进程维护一个page-table，描述进程的地址空间
- xv6为整个内核维护一个page-table，描述内核的地址空间
- QEMU模拟的内存从0x80000000，该地址以下的为设备接口，会被映射到对设备的操作
- 内核使用直接映射获取RAM或者设备的地址
  - 比如内核的地址空间在`KERNBASE=0x8000000`，虚拟地址和物理地址皆如此
  - 也有一些不是直接映射的
    - trampoline page
    - 内核栈pages：每个进程有自己的kernel stack，并且stack下有个guard page，溢出的话会引发异常
- 内核对trampoline和kernel text的映射分别授予了PTE_R和PTE_X权限，其它是PTE_R和PTE_W

![xv6 kernel address space](xv6%20kernel%20address%20space.png)

3.3 Code: creating an address space 创建地址空间

- `kernel/vm.c`文件包含了主要的虚拟内存操作
  - 主要数据结构`pagetable_t`，实际上是个指向`page-table page`的指针
  - `walk`函数用来寻找虚拟地址的页表入口`PTE`
  - `mappages`函数添加一个新的映射（创建PTE）
  - `copyin/copyout`用于拷贝字符串(到/自)用户空间
- 启动过程，main会调用`kvminit`(kernel/vm.c:22)
  - 它首先分配一个page
  - 然后使用`kvmmap`添加内核的指令和数据地址映射
  - kvmmap调用`mappages`，调用`walk`添加映射
- `walk`的时候虚拟地址需要是直接映射到物理地址
- main会调用`kvminithart`，将根`page-table page`写入到`satp`寄存器
- `procinit`(kernel/proc.c:26)会分配每个进程的内核栈
- 每个RISC-V CPU会将PTE(page table entries)缓存到`Translation Look-aside Buffer(TLB)`中，所以页表发生更改时需要通知CPU。
- RSCV-V的`sfence.vma`指令会刷新CPU的TLB，因此`kvminithart`中在更新`stap`寄存器后会使用该指令，以及在`kernel/trampoline.S:79`中切换到用户页表的时候

3.4 物理内存分配

- 以4096B的页为单位
- 使用链表存储可用的页

3.5 Code: Physical memory allocator

- `kalloc.c`(kernel/kalloc.c)
- 数据结构：`struct run`，被自旋锁保护
- 启动时主函数使用`kinit`初始化分配器，会将内核结束到物理地址结束之间的页初始化到链表
- xv6认为内存是固定的128MB
- 物理地址需要对齐页的大小4096，所以需要使用`PGROUNDUP`确保地址合法
- 每个run对象实际就是对应的页，run的指针就是那个页的地址，页大小为4096，会被强制转换为`run`的类型，并将其中的`next`指针指向下一个页的地址

3.6 进程地址空间

- 每个进程有独立的页表`page table`，系统在切换进程时需要切换页表
- 进程需要内存时，内核使用`kalloc`分配
- 虚拟地址一些优点
  - 每个进程有独立地址空间
  - 虚地址连续，物理地址可能不连续
  - `trampoline`代码可以映射到每个进程地址空间（只需要一个trampoline物理内存page）
- 进程地址空间如图，栈stack是个独立的页，包含exec执行的时候的一些参数内容
- 栈下面有个guard页，确保参数过大溢出时会产生错误

![process user address space](process%20user%20address%20space.jpg)

3.7 Code: sbrk

- sbrk用来扩张/缩小进程的内存，具体实现在`growproc`(kernel/proc.c:239)
- 使用`uvmalloc, uvmdealloc`分配/释放内存
  - 分配`uvmalloc`时会使用`kalloc`分配内存并使用`mappages`添加页表入口PTE
  - 释放`uvdealloc`时使用`walk`寻找PTE并使用`kfree`释放

3.8 Code: exec

- exec从文件系统创建地址空间中用户的部分
- 使用`namei`获得打开`elf`文件，并读取其中内容，`kernel/elf.h`定义了ELF文件格式
  - `struct elfhdr`
  - `struct proghdr` 一个ELF段Segment
- 过程
  1. 检查`elfhdr`内容
  2. 使用`proc_pagetable`(kernel/exec.c:38)分配Segment的内存
  3. 使用`loadseg`将Segment加载到内存，使用`walk`获得物理地址，使用`readi`读取段内容
  4. `proghdr`的`filesz`可能比`memsz`小，表示那些gap区域需要填充为0，作为C的全局变量区域
  5. 拷贝参数字符串，并记录在`ustack`中
  6. `ustack`的前三个entry是`fake return program counter`, `argc` 和 `argv指针`
  7. exec会在栈下添加一个空page作为guard，`copyout`溢出的话会发现不可访问返回-1
  8. 执行失败会跳转到bad，释放镜像image
  9. 一旦新镜像image创建完，exec就可以更新`new page table`(kernel/exec.c:113)，并释放旧的
1.  exec会将ELF文件内容加载到指定的区域，非常危险，需要许多地址检验

3.9 Real world 现实世界

- 许多操作系统会结合page-fault异常，使用更复杂的分页机制
- xv6内核地址固定起始于0x80000000，但真实的机器其RAM是不可预测的
- xv6不支持物理地址的保护（RISC-V支持）
- RISC-V支持`super page`超大页（MB级的page），但是xv6不支持
- xv6缺少像`malloc`这样的分配器
- 真实的操作系统需要能分配各种大小内存块，而不是只支持4096B的page
