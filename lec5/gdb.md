
gdb一些常用的commands

- gdb识别无歧义的缩写： c=co=continue, s=step, si=stepi
- `step` 每行，进入function
- `next` 每行，不进入函数
- `stepi` `nexti` 跟一个整数参数，执行多行
- `continue` 继续直到断点
- `finish` 直到函数结束
- `advance <location>` 直到某个位置
- `break <location>` 断点，`delete disable enable` 管理断点
- `break <location> if <condition>` 条件断点
- `cond <number> <condition>` 在 number 指定的断点加条件
- `watch <expression>` 如果表达式值改变就暂停
- `watch -l <address>` 如果地址内的值改变就暂停
- `rwatch <expression>` 如果表达式被读取就暂停
- `x` 打印， `x/x x/i` 以16进制、汇编 打印
- `print` 打印C表达式的值，比`x`有用：`p *((struct elfhdr *) 0x10000)`
- `info registers/frame/locals/<location>` 打印 寄存器/栈帧/局部变量/源代码
- `backtrace`
- `layout`: `tui + layout asm`
- `help`
