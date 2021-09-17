一些问题

1. LEC3的[视频](https://www.youtube.com/watch?v=o44d---Dk4o&ab_channel=DavidMorejon)中提到指令集的用户模式和特权模式通过一个标志位(flag)来区分，CPU根据这个状态位确定当前的模式，而更改这个标志位是**特权操作**，也就是需要内核模式才能修改。那么，一旦CPU设定为用户模式，不就无法再进入内核模式了吗？
    
    RISC-V指令集提供了`ecall`指令，该指令使CPU从用户模式转为内核模式(Supervisor mode)，并执行到内核指定的入口点程序。
