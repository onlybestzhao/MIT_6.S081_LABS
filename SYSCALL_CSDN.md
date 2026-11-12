# MIT 6.S081学习日记第6篇：实现 xv6 的系统调用追踪

> 本文记录 MIT 6.S081 Lab 2 中 `trace` 系统调用的实现过程，重点整理系统调用从用户态进入内核的路径、trace mask 的作用，以及 `fork` 为什么要继承追踪配置。

## 一、实验目标

本实验要求为 xv6 增加一个 `trace` 系统调用。用户通过一个整数 mask 指定需要追踪的系统调用。

例如：

```sh
trace 32 grep hello README
```

`SYS_read` 的编号是 5，因此：

```text
1 << SYS_read = 1 << 5 = 32
```

这条命令表示只追踪 `read` 系统调用。成功后，内核会在系统调用返回前打印类似：

```text
3: syscall read -> 1023
```

输出内容包括：

1. 进程 ID；
2. 系统调用名称；
3. 系统调用返回值。

实验不要求打印系统调用参数。

## 二、先理解 `trace` 程序

执行：

```sh
trace 32 grep hello README
```

`user/trace.c` 的 `main` 会收到命令行参数：

```text
argc = 5
argv[0] = "trace"
argv[1] = "32"
argv[2] = "grep"
argv[3] = "hello"
argv[4] = "README"
```

程序先执行：

```c
trace(atoi(argv[1]));
```

也就是：

```c
trace(32);
```

然后执行：

```c
exec(nargv[0], nargv);
```

其中 `nargv` 指向：

```text
nargv[0] = "grep"
nargv[1] = "hello"
nargv[2] = "README"
nargv[3] = 0
```

因此，`trace.c` 本身主要是一个用户态启动器：

```text
读取 mask
→ 调用 trace(mask)
→ exec 启动目标程序
```

真正的追踪逻辑在内核中完成。

## 三、系统调用是怎样进入内核的

以 `trace(32)` 为例，用户态 stub 由 `user/usys.pl` 生成，核心汇编类似：

```asm
trace:
  li a7, SYS_trace
  ecall
  ret
```

这里：

- `a7` 保存系统调用编号；
- `a0` 保存第一个参数，也就是 mask；
- `ecall` 是 RISC-V 指令，不是 C 函数；
- `ecall` 会让 CPU 从用户态进入内核态。

进入内核后，xv6 大致经过：

```text
user/usys.S
    ↓ ecall
kernel/trampoline.S：保存用户寄存器
    ↓
kernel/trap.c：usertrap()
    ↓
kernel/syscall.c：syscall()
    ↓
kernel/sysproc.c：sys_trace()
```

用户寄存器会保存到当前进程的 `trapframe` 中，因此内核可以看到：

```text
p->trapframe->a7 = SYS_trace
p->trapframe->a0 = 32
```

`syscall()` 根据 `a7` 找到具体的系统调用函数。`sys_trace()` 中的：

```c
argint(0, &mask);
```

表示读取第 0 个系统调用参数，也就是 `a0`，最终得到：

```text
mask = 32
```

## 四、添加用户态接口

### 1. 在 `user/user.h` 中声明

```c
int trace(int);
```

也可以写成更清晰的形式：

```c
int trace(int mask);
```

### 2. 在 `user/usys.pl` 中添加 stub

```perl
entry("trace");
```

Makefile 会调用这个 Perl 脚本生成 `user/usys.S`。

### 3. 在 Makefile 中加入用户程序

```make
$U/_trace\
```

否则 `trace` 可执行文件不会被打包进 xv6 的文件系统镜像。

## 五、添加内核系统调用

### 1. 分配系统调用编号

在 `kernel/syscall.h` 中添加：

```c
#define SYS_trace 22
```

名称必须和 `usys.pl` 生成的 `SYS_trace` 保持大小写一致。`SYS_trace` 是编号宏，而 `sys_trace` 是内核处理函数，两者不是同一个东西。

### 2. 保存每个进程的 mask

在 `struct proc` 中添加：

```c
int tracemask;
```

这个变量必须放在进程结构体中，而不是定义成全局变量。因为题目要求：

```text
一个进程开启 trace
不能影响其他进程
```

### 3. 实现 `sys_trace`

在 `kernel/sysproc.c` 中实现：

```c
uint64
sys_trace(void)
{
  int mask;

  if(argint(0, &mask) < 0)
    return -1;
  myproc()->tracemask = mask;
  return 0;
}
```

这个函数只负责保存配置：

```text
sys_trace：设置当前进程的 tracemask
```

它不会在这里追踪 `read`、`write` 或其他系统调用。

### 4. 把函数加入系统调用分发表

在 `kernel/syscall.c` 中先声明：

```c
extern uint64 sys_trace(void);
```

然后加入：

```c
[SYS_trace] sys_trace,
```

这里左边是系统调用编号，右边是处理函数：

```text
SYS_trace → 22
sys_trace → 具体的内核函数
```

## 六、在统一入口实现追踪

所有系统调用都会经过 `kernel/syscall.c` 中的 `syscall()`：

```c
num = p->trapframe->a7;
p->trapframe->a0 = syscalls[num]();
```

其中：

- `num` 决定调用哪个系统调用；
- `a0` 在调用完成后保存返回值。

因此，追踪判断应该放在具体系统调用执行完成之后：

```c
if(p->tracemask & (1 << num))
  printf("%d: syscall %s -> %d\n",
         p->pid, syscall_names[num], p->trapframe->a0);
```

还需要一个系统调用名称数组，把编号转换成字符串：

```c
static char *syscall_names[] = {
  [SYS_fork] = "fork",
  [SYS_exit] = "exit",
  [SYS_read] = "read",
  [SYS_write] = "write",
  [SYS_trace] = "trace",
};
```

之所以不能把追踪逻辑全部写在 `sys_trace()` 里，是因为 `sys_trace()` 只在用户调用 `trace(mask)` 时执行一次。之后 `grep` 会继续调用 `open`、`read`、`write`、`close` 和 `exit`，这些调用都会重新进入统一的 `syscall()`。

因此三者的职责是：

```text
sys_trace()：设置规则
fork()：继承规则
syscall()：执行规则并打印
```

## 七、为什么 `fork()` 要复制 `tracemask`

实验要求 trace 对当前进程以及之后创建的子进程生效。

例如：

```sh
trace 2 usertests forkforkfork
```

`SYS_fork` 是 1，所以：

```text
1 << SYS_fork = 2
```

执行 `trace(2)` 后，当前进程状态是：

```text
父进程 tracemask = 2
```

当 `usertests` 调用 `fork()` 时，xv6 会创建新的 `struct proc`：

```c
struct proc *p = myproc();     // 父进程
struct proc *np = allocproc(); // 子进程
```

因此需要显式复制：

```c
np->tracemask = p->tracemask;
```

否则：

```text
父进程 tracemask = 2
子进程 tracemask = 0
```

子进程后续调用 `fork` 时，`syscall()` 检查 mask 会失败，也就不会打印子进程的 trace 信息。

这也是为什么普通的：

```sh
trace 32 grep hello README
```

可能看不出这行代码的重要性：`grep` 主要进行文件读取，不一定创建子进程。只有测试进程继承的命令才能验证它。

## 八、测试过程

### 1. 只追踪 `read`

```sh
trace 32 grep hello README
```

预期看到类似：

```text
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0
```

### 2. 追踪全部常用系统调用

```sh
trace 2147483647 grep hello README
```

可以看到 `trace`、`exec`、`open`、`read`、`close` 等系统调用。

### 3. 验证子进程继承

```sh
trace 2 usertests forkforkfork
```

应当看到多个不同进程 ID 的 `fork` 输出。

### 4. 验证普通程序不受影响

```sh
grep hello README
```

不使用 `trace` 时不应出现追踪日志。这说明 `tracemask` 是进程私有状态，而不是全局开关。

## 九、遇到的问题

### 1. `SYS_TRACE` 和 `SYS_trace` 大小写不一致

`user/usys.pl` 会生成：

```asm
li a7, SYS_trace
```

所以 `kernel/syscall.h` 必须定义：

```c
#define SYS_trace 22
```

不能写成 `SYS_TRACE`，因为 C 和汇编标识符区分大小写。

### 2. `sys_trace` 未声明

如果系统调用表中写了：

```c
[SYS_trace] sys_trace,
```

却没有前面的声明：

```c
extern uint64 sys_trace(void);
```

编译器就会报告 `sys_trace undeclared`。

### 3. GCC 把 `runcmd()` 递归当成错误

Ubuntu 24.04 的 GCC 13 会对 xv6 shell 中设计好的递归 `runcmd()` 报告：

```text
infinite recursion detected
```

由于 Makefile 使用 `-Werror`，警告会被当成错误。为兼容这个编译器，在 Makefile 中加入：

```make
CFLAGS += -Wno-infinite-recursion
```

这只是关闭编译器对 xv6 shell 递归结构的误报，不改变 shell 的执行逻辑。

## 十、总结

这次实验最重要的是理解系统调用的完整路径：

```text
用户程序 trace(mask)
→ user/usys.S 设置 a7 和 a0
→ ecall 进入内核
→ trapframe 保存用户寄存器
→ syscall() 根据 a7 分发
→ sys_trace() 保存 mask
→ exec() 启动目标程序
→ 目标程序的每次系统调用再次进入 syscall()
→ 根据 tracemask 决定是否打印
```

最终可以把实验记成三句话：

```text
trace.c：解析参数并启动目标程序
sys_trace：设置当前进程的追踪规则
fork + syscall：继承并执行追踪规则
```

这次实验表面上是在增加一个系统调用，实际上练习了用户态和内核态之间的参数传递、trapframe、系统调用分发表，以及进程状态在 `fork` 后的继承。

---

# MIT 6.S081学习日记第7篇：实现 xv6 的系统信息查询

> 本文记录 MIT 6.S081 Lab 2 中 `sysinfo` 系统调用的实现过程，重点整理用户地址参数、`copyout()`、空闲物理页链表、进程表统计以及内核锁的作用。

## 一、实验目标

`sysinfo` 用于查询 xv6 当前的系统状态，并返回：

```c
struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};
```

其中：

- `freemem`：当前可以由 `kalloc()` 分配的空闲物理内存，单位是字节；
- `nproc`：当前进程表中状态不是 `UNUSED` 的进程数量。

用户程序的调用形式是：

```c
struct sysinfo info;
sysinfo(&info);
```

内核不能直接把用户指针当作内核地址使用，而是先在内核中填充结构体，再通过 `copyout()` 复制到用户地址空间。

## 二、添加用户态接口

### 1. 声明结构体和函数

在 `user/user.h` 中声明：

```c
struct sysinfo;
int sysinfo(struct sysinfo *);
```

在 `user/usys.pl` 中添加：

```perl
entry("sysinfo");
```

脚本会生成用户态系统调用 stub。用户调用 `sysinfo(&info)` 时，stub 会把系统调用编号放入 `a7`，把 `&info` 放入 `a0`，然后执行 `ecall`。

### 2. 分配系统调用编号

在 `kernel/syscall.h` 中添加：

```c
#define SYS_sysinfo 23
```

同时在 `kernel/syscall.c` 中声明并加入系统调用表：

```c
extern uint64 sys_sysinfo(void);

[SYS_sysinfo] sys_sysinfo,
```

还要在 Makefile 中加入：

```make
$U/_sysinfotest\
```

这样测试程序才会被编译并打包到 xv6 文件系统中。

## 三、`sys_sysinfo()` 的参数和 `copyout()`

`sysinfo` 的参数是用户空间中 `struct sysinfo` 的地址，不是文件描述符，因此应该使用：

```c
uint64 addr;

if(argaddr(0, &addr) < 0)
  return -1;
```

内核中需要创建一个实际的结构体变量：

```c
struct sysinfo st;
```

然后填充字段：

```c
st.nproc = nproc();
st.freemem = freemem();
```

最后复制到用户地址：

```c
if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
  return -1;
```

这里不能写成：

```c
struct sysinfo *st;
st->nproc = nproc();
```

因为这只声明了一个未初始化的指针，没有创建实际的 `struct sysinfo` 对象。直接访问 `st->nproc` 会向随机地址写数据，可能触发 `kerneltrap`。

也不能把指针本身复制出去；必须复制真正的结构体对象，因此使用：

```c
(char *)&st, sizeof(st)
```

## 四、统计空闲物理内存

### 1. `freelist` 保存了什么

`kernel/kalloc.c` 中的 `kmem.freelist` 是空闲物理页组成的链表。初始化分配器时：

```c
freerange(end, (void*)PHYSTOP);
```

`freerange()` 每次移动 `PGSIZE`，并调用 `kfree()` 把这一页加入链表：

```c
for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  kfree(p);
```

因此，链表中的每一个 `struct run` 节点都代表一个空闲物理页：

```text
freelist
   |
   v
[空闲页] -> [空闲页] -> [空闲页] -> 0
```

`kalloc()` 从链表头部取走一个节点，表示分配一页；`kfree()` 把释放的页重新放回链表。

### 2. 为什么要乘以 `PGSIZE`

遍历链表得到的是空闲页数量，而 `sysinfo.freemem` 要求返回字节数：

```text
空闲内存字节数 = 空闲页数量 × 每页字节数
```

RISC-V xv6 中：

```c
PGSIZE = 4096
```

所以如果有 `number` 个空闲页，应该返回：

```c
number * PGSIZE
```

这里统计的是所有可由 `kalloc()` 分配的空闲物理页，不只是页表页。这些页还可能被用于用户内存、内核栈、trapframe 和 pipe 缓冲区。

### 3. 为什么需要 `kmem.lock`

`kmem.freelist` 会被多个 CPU 同时修改：

```text
kalloc()：从链表取出页面
kfree()：把页面放回链表
freemem()：遍历链表统计页面
```

如果 `freemem()` 遍历时不加锁，另一个 CPU 可能同时修改 `freelist` 或某个 `run->next`，导致统计结果不稳定，甚至访问无效链表节点。

因此遍历期间要持有：

```c
acquire(&kmem.lock);
```

遍历完成后再：

```c
release(&kmem.lock);
```

锁保护的是空闲页链表结构，不是说所有物理内存内容都必须由这个锁保护。

## 五、统计进程数量

进程表定义为：

```c
struct proc proc[NPROC];
```

下面的写法会遍历整个数组：

```c
struct proc *p;

for(p = proc; p < &proc[NPROC]; p++) {
  ...
}
```

其中：

- `proc` 等价于 `&proc[0]`；
- `p++` 会移动到下一个 `struct proc`；
- `&proc[NPROC]` 是数组尾后地址，只用于比较，不能解引用。

`nproc()` 应统计：

```c
if(p->state != UNUSED)
  count++;
```

不能统计 `p->state == UNUSED`，因为那是在统计空闲进程表项。

每个进程都有自己的锁，所以锁的使用应当和当前循环项配对：

```c
for(p = proc; p < &proc[NPROC]; p++) {
  acquire(&p->lock);

  if(p->state != UNUSED)
    count++;

  release(&p->lock);
}
```

不能把 `release(&p->lock)` 放到循环外。循环体执行完后，`for` 的 `p++` 会让 `p` 变成 `&proc[NPROC]`；此时它已经不是最后一个进程。另外，前面每轮获取的锁也没有释放，可能导致死锁或 `panic: release`。

## 六、`.` 和 `->` 的区别

两者都是访问结构体成员，但左侧类型不同：

```c
struct sysinfo st;
st.nproc = 1;          // st 是结构体变量，使用 .

struct proc *p;
p->state = UNUSED;     // p 是结构体指针，使用 ->
```

指针形式：

```c
p->state
```

等价于：

```c
(*p).state
```

例如：

```c
kmem.freelist
```

中 `kmem` 是结构体变量，所以使用 `.`；而：

```c
r->next
```

中 `r` 是 `struct run *`，所以使用 `->`。

## 七、测试过程

运行：

```sh
sysinfotest
```

测试程序主要验证以下内容：

### 1. 正常调用和非法地址

```c
sysinfo(&info);
sysinfo((struct sysinfo *)0xeaeb0b5b00002f5e);
```

非法用户地址必须让系统调用返回 `-1`，不能让内核崩溃。

### 2. 空闲内存统计

测试先通过连续的 `sbrk(PGSIZE)` 耗尽可用物理页，然后检查：

```text
freemem == 0
```

之后释放内存，检查空闲字节数恢复；再次分配一页后，`freemem` 应减少 `PGSIZE`。

### 3. 进程数量统计

测试记录 `fork()` 前的进程数量。子进程执行 `sysinfo` 时，数量应该增加 1；子进程退出并被父进程回收后，数量应该恢复。

成功时会输出：

```text
sysinfotest: OK
```

## 八、遇到的问题

### 1. 把 `argfd()` 用在 `sysinfo()` 中

`argfd()` 用于读取文件描述符参数，例如 `fstat(fd, &st)`。`sysinfo()` 只有一个用户结构体地址参数，所以应该使用 `argaddr()`。

### 2. 把未初始化指针当成结构体使用

```c
struct sysinfo *st;
st->nproc = nproc();
```

这不是创建结构体，而是创建一个没有目标地址的指针。应该使用结构体变量：

```c
struct sysinfo st;
```

### 3. 锁没有一一配对

`acquire()` 和 `release()` 必须针对同一个锁成对出现。遍历进程表时，每次读取一个 `p->state` 后就释放当前 `p->lock`；遍历空闲页链表时，则在整个链表遍历期间持有 `kmem.lock`。

## 九、总结

`sysinfo` 的完整流程可以概括为：

```text
用户程序准备 struct sysinfo
→ 用户态 stub 传入结构体地址
→ sys_sysinfo() 使用 argaddr() 读取地址
→ freemem() 统计空闲物理页
→ nproc() 统计有效进程
→ copyout() 把内核结构体复制到用户空间
```

这次实验最重要的几个概念是：

```text
freelist 中的一个节点代表一个空闲物理页
freemem 返回页数 × PGSIZE 得到的字节数
nproc 统计 state != UNUSED 的进程
acquire 和 release 必须严格配对
copyout 负责把内核数据安全复制到用户地址空间
```
