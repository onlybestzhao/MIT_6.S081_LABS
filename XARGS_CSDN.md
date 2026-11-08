# MIT6.S081学习日记第5篇：实现 xv6 的 xargs

> 本文记录 MIT 6.S081 Lab 1 中 `xargs` 的实现过程，重点整理自己的思考、踩过的坑，以及如何一步步定位问题。

## 一、实验目标

实验要求在 `user/xargs.c` 中实现一个简化版的 UNIX `xargs`：

1. 从标准输入读取内容；
2. 按行读取输入；
3. 把每一行追加到命令原有参数的后面；
4. 使用 `fork` 和 `exec` 执行命令；
5. 父进程使用 `wait` 等待子进程结束。

例如：

```sh
echo hello too | xargs echo bye
```

逻辑上相当于执行：

```sh
echo bye hello too
```

`xargs` 不是把 `echo` 和管道符 `|` 混在一起实现。`|` 是 xv6 shell 提供的管道操作符，`xargs` 是从标准输入读取数据，再重新组织参数并执行其他程序。

## 二、先理解 `argv` 的排列

执行：

```sh
xargs grep hello
```

程序收到的参数大致是：

```text
argv[0] = "xargs"
argv[1] = "grep"
argv[2] = "hello"
argv[3] = 0
```

但 `exec` 执行 `grep` 时，参数数组应该变成：

```text
new_args[0] = "grep"
new_args[1] = "hello"
new_args[2] = "从标准输入读取的一行"
new_args[3] = 0
```

注意，`xargs` 自己的程序名不能传给 `grep`。因此需要把 `argv[1]` 开始的命令参数复制到新的数组中。

执行：

```sh
xargs -n 1 echo line
```

时，`-n 1` 是 `xargs` 自己的选项，不应该传给 `echo`。真正执行的命令参数应该从 `argv[3]` 开始：

```text
new_args[0] = "echo"
new_args[1] = "line"
```

## 三、第一次误区：`read` 一次不等于读取一行

最开始容易写成：

```c
count = read(0, buf, sizeof(buf));
```

然后假设 `buf` 里就是一整行。实际上，`read` 只保证读取若干字节：

- 可能一次读到多行；
- 可能只读到一行的一部分；
- 也可能把输入分成多次读取。

所以真正重要的是遍历本次读取到的字节：

```c
while((count = read(0, buf, sizeof(buf))) > 0){
  for(i = 0; i < count; i++){
    // 逐个处理 buf[i]
  }
}
```

实验提示中说“read a character at a time”，本质上是要求程序能够逐个识别字符，尤其是换行符。使用数组批量读取后再逐个遍历也可以，但不能只处理 `buf[0]` 或假设一次 `read` 就是一行。

## 四、真正的换行和普通字符 `\n`

这是这次排错中最容易混淆的地方。

### 1. 真正的换行

真正的换行字符是：

```c
'\n'
```

它的 ASCII 值是 `10`。

例如 `find` 中的代码：

```c
printf("%s\n", buf);
```

这里的 `\n` 会被 C 编译器转换成真正的换行字符。因此：

```sh
find . b | xargs grep hello
```

时，xargs 读到的文件名后面确实有 ASCII 10。

### 2. 两个普通字符 `\` 和 `n`

xv6 当前的 `echo` 程序只是把参数原样写出，并不会把字符序列：

```text
\n
```

解释成换行。所以：

```sh
echo "1\n2"
```

在这个 xv6 环境中，可能得到的是普通字符：

```text
1 \ n 2
```

而不是两行：

```text
1
2
```

因此 xargs 处理标准输入时，应该优先实现实验要求的真实 `'\n'` 分行逻辑，不要把 `\\` 和 `n` 当成标准 xargs 的换行。

## 五、参数切分的基本思路

我使用了一个 `flag` 来表示当前是否已经处于一个参数中。

### 遇到普通字符

如果当前不在参数中，就记录这个参数的起始地址：

```c
else if(!flag){
  input[a] = &buf[i];
  a++;
  flag = 1;
}
```

例如读取：

```text
./b
```

`input[0]` 会指向 `buf` 中的 `'.'`。

### 遇到空格

空格表示当前参数结束，需要把它改成字符串结束符：

```c
if(buf[i] == ' '){
  buf[i] = '\0';
  flag = 0;
}
```

这样：

```text
hello world
```

会被切成两个 C 字符串：

```text
hello\0world\0
```

### 遇到换行

换行表示当前这一行已经收集完成。处理顺序应该是：

```text
把换行改成 '\0'
把 input 中保存的参数追加到 new_args
给 new_args 补 0
fork + exec
父进程 wait
清空当前行状态
```

核心代码结构如下：

```c
else if(buf[i] == '\n'){
  buf[i] = '\0';

  for(j = 0; j < a; j++){
    new_args[o] = input[j];
    o++;
  }

  new_args[o] = 0;
  fork_again(new_args);

  a = 0;
  flag = 0;
  o = base_argc;
}
```

这里不能只把 `'\n'` 识别出来就立刻结束整个程序。它只表示当前这一行结束，后面还可能有下一行输入。

## 六、`fork`、`exec` 和 `wait`

我把执行命令的部分单独整理成了函数：

```c
void
fork_again(char *again_args[])
{
  int pid = fork();

  if(pid == 0){
    exec(again_args[0], again_args);
    exit(1);
  }

  wait(0);
}
```

这里有几个关键点。

### 1. 子进程执行 `exec`

```c
if(pid == 0){
  exec(again_args[0], again_args);
}
```

`exec` 成功后，子进程的代码会被新程序替换，不会返回。

### 2. 父进程执行 `wait`

父进程不能在第一次命令执行后直接退出，否则 xargs 只能处理第一行输入：

```c
wait(0);
```

等待结束后，父进程应该返回主循环，继续读取下一行。

### 3. `exec` 失败后要退出

如果 `exec` 失败，它会返回。因此子进程中应该处理失败：

```c
exec(again_args[0], again_args);
fprintf(2, "xargs: exec failed\n");
exit(1);
```

## 七、踩过的几个坑

### 坑一：把 `'n'` 当成换行

错误写法：

```c
buf[i] == 'n'
```

这只是判断字母 `n`。

正确写法：

```c
buf[i] == '\n'
```

### 坑二：把 `\n` 分支写在 `!flag` 后面

如果代码顺序是：

```c
else if(!flag){
  ...
}
else if(buf[i] == '\n'){
  ...
}
```

当遇到换行且 `flag == 0` 时，会先进入 `!flag` 分支，后面的换行分支根本不会执行。

应该优先判断特殊字符：

```c
if(buf[i] == ' ')
else if(buf[i] == '\n')
else if(!flag)
```

### 坑三：在内外两个 `for` 中重复使用同一个下标

外层循环使用 `i` 遍历输入字符时，不能在复制参数时再次使用 `i`：

```c
for(i = 0; i < count; i++){
  for(i = 0; i < a; i++){
    ...
  }
}
```

内层应该使用另一个变量，例如 `j`：

```c
for(j = 0; j < a; j++){
  new_args[o] = input[j];
  o++;
}
```

否则外层的字符遍历位置会被破坏。

### 坑四：把 `fork` 放在父进程等待之后直接 `exit`

如果 `fork_again` 的末尾无条件写：

```c
exit(0);
```

那么父进程 `wait` 后也会退出，后续输入行无法处理。

正确逻辑是：

```text
子进程 exec
父进程 wait
父进程返回主循环
```

### 坑五：调试输出访问了不存在的参数

例如：

```c
printf("xargs[4]=%s\n", xargs[4]);
```

如果命令只有：

```text
xargs grep hello
```

那么有效参数只有 `xargs[0]` 到 `xargs[2]`，`xargs[3]` 是结束标记。访问 `xargs[4]` 就越界，可能造成 `usertrap`。

调试完成后，应该删除所有会混入测试输出的 `printf`。

## 八、用 `find` 和 `grep` 验证

`grep` 是搜索文本的用户程序，`|` 是 shell 的管道操作符，两者不是同一个东西：

```text
grep：搜索文件内容
|：把左边程序的标准输出连接到右边程序的标准输入
```

例如：

```sh
find . b | xargs grep hello
```

执行过程是：

```text
find . b
    输出 ./b\n

管道
    把 ./b\n 传给 xargs

xargs grep hello
    构造 grep hello ./b

grep
    在 ./b 中搜索 hello
```

如果 `b` 是空文件，grep 没有匹配内容时没有输出是正常的，并不代表 xargs 没有执行。

## 九、补充自己的测试脚本

由于当前代码库中没有配套的 `xargstest.sh`，我自己创建了一个测试脚本：

```sh
echo a > b
echo hello >> b
echo c > d
echo hello >> d
mkdir e
echo hello > e/f

find . b | xargs grep hello
find . d | xargs grep hello
find e f | xargs grep hello
```

并在 `Makefile` 中把脚本加入文件系统镜像：

```make
fs.img: mkfs/mkfs README xargstest.sh $(UPROGS)
	mkfs/mkfs fs.img README xargstest.sh $(UPROGS)
```

重新构建后，在 xv6 shell 中执行：

```sh
sh < xargstest.sh
```

这样可以验证 xargs 是否能够：

1. 接收 `find` 输出的真实换行；
2. 正确去掉路径末尾的换行；
3. 把路径追加到 `grep hello` 后面；
4. 对每一行输入分别执行一次命令。

## 十、总结

这次实验最重要的不是把字符串“硬拆出来”，而是理解几个边界：

```text
read 返回的字节块 ≠ 一行输入
普通字符 '\\' 'n' ≠ 真正的换行 '\n'
xargs 的 argv ≠ 被执行命令的 argv
fork 之后父子进程的职责不同
exec 的参数数组必须以 0 结尾
```

实现 xargs 的主线可以总结为：

```text
读取输入
→ 逐个检查字符
→ 按空格和换行切分参数
→ 拼接 new_args
→ new_args 末尾补 0
→ fork
→ 子进程 exec
→ 父进程 wait
→ 继续处理下一行
```

这次最大的收获是：遇到输出异常时，不能只盯着最终结果，要回到字节、指针、数组下标和父子进程执行路径上逐层确认。
