Page tables 页表

3.1 Paging hardware 提供分页能力的硬件

- RISC-V指令的地址操作数都为虚拟地址，page table hardware负责虚拟地址到物理地址的转换
- xv6运行在Sv39的RISC-V上：64位虚拟地址只有低39位用到
- 寻址过程：64位虚拟地址，搞25位未使用，低39位作为地址
  - 低39位的高27位
    - `9+9+9`的PTE(Page Table Entry)树形结构，最终定位到一个**44位**的PPN(Physical Pag Number)
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
- 内核使用直接映射或者RAM或者设备的地址
  - 比如内核的地址空间在`KERNBASE=0x8000000`，虚拟地址和物理地址皆如此
  - 也有一些不是直接映射的
    - trampoline page
    - 内核栈pages：每个进程有自己的kernel stack，并且stack下有个guard page，溢出的话会引发异常
- 内核对trampoline和kernel text的映射分别授予了PTE_R和PTE_X权限，其它是PTE_R和PTE_W

![xv6 kernel address space](xv6%20kernel%20address%20space.png)

3.3 Code: creating an address space 创建地址空间