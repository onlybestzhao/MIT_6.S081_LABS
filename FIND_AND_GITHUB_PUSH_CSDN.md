# MIT 6.S081 实战记录：从实现 xv6 `find` 到解决 WSL 推送 GitHub

> 本文记录一次 MIT 6.S081 xv6 实验中的真实排错过程，主要包含两部分：
>
> 1. 在 xv6 中实现递归查找文件；
> 2. 在 WSL 中解决 GitHub `push` 失败，并写一个自动提交脚本。

## 一、实验目标：实现 xv6 版 `find`

希望在 xv6 shell 中执行：

```sh
find . b
```

表示从当前目录开始，递归查找名字为 `b` 的文件，期望输出类似：

```text
./b
./a/b
```

这里两个参数的含义不同：

```text
argv[1] = ".": 搜索起点
argv[2] = "b": 要查找的名字
```

递归过程中，搜索起点会变化：

```text
find(".", "b")
find("./a", "b")
find("./a/c", "b")
```

但目标名字始终不变。

## 二、先理解 xv6 的目录结构

xv6 中目录本质上也是一种可以读取的文件。目录中的每一项可以用
`struct dirent` 表示：

```c
struct dirent {
  ushort inum;
  char name[DIRSIZ];
};
```

两个字段的含义是：

- `inum`：inode 编号；
- `name`：目录项名称。

读取目录时，典型代码是：

```c
struct dirent de;

while(read(fd, &de, sizeof(de)) == sizeof(de)){
  if(de.inum == 0)
    continue;
}
```

`inum == 0` 表示这个目录槽位没有有效目录项，需要跳过。

当前目录项的名字在：

```c
de.name
```

文件类型则需要通过 `stat` 或 `fstat` 获取，例如：

```c
struct stat st;
stat(path, &st);
```

`st.type` 用来判断路径对应的是目录、普通文件还是设备文件。

## 三、第一次遇到的问题：`argv` 不是只有用户输入的参数

执行：

```sh
find . b
```

程序实际收到的参数大致是：

```text
argc    = 3
argv[0] = "find"
argv[1] = "."
argv[2] = "b"
argv[3] = 0
```

因此 `main` 必须按照两个参数接收：

```c
int
main(int argc, char *argv[])
```

不能把它写成：

```c
int
main(char *argv[])
```

否则程序会把本来表示参数个数的整数误当成指针，访问
`argv[0]` 时就可能产生非法内存访问。

同时，不能把整个 `argv` 和某个单独的参数混为一谈：

```c
argv       // 参数数组
argv[1]    // 一个字符串："." 
argv[2]    // 一个字符串："b"
```

递归函数更适合表达为：

```c
find(current_path, target_name);
```

而不是让递归函数每次重新解释整个 `argv`。

## 四、第二个问题：`if` 后面多写一个分号

曾经出现过这样的代码：

```c
if(de.inum == 0);
  continue;
```

在 C 语言中，分号本身就是一条空语句。上面的代码等价于：

```c
if(de.inum == 0){
  // 什么也不做
}

continue;
```

于是无论 `inum` 是否为零，都会执行 `continue`，后面的路径拼接代码永远到不了。

正确的写法是：

```c
if(de.inum == 0)
  continue;
```

或者使用大括号：

```c
if(de.inum == 0){
  continue;
}
```

`continue` 的作用是跳过当前这一轮循环，直接进入下一轮；
`break` 才是结束整个循环。

## 五、第三个问题：路径拼接不是给指针赋值

`ls.c` 中常见的路径拼接代码如下：

```c
strcpy(buf, path);
p = buf + strlen(buf);
*p++ = '/';
memmove(p, de.name, DIRSIZ);
p[DIRSIZ] = 0;
```

假设：

```text
path    = "."
de.name = "b"
```

执行过程是：

```text
strcpy(buf, path)       -> "."
p = buf + strlen(buf)   -> p 指向字符串结尾
*p++ = '/'              -> "./"
memmove(p, de.name, ...) -> "./b..."
p[DIRSIZ] = 0           -> "./b"
```

这里 `p` 和 `buf` 指向同一块字符数组的不同位置。
通过 `p` 写入字符，就是在修改 `buf` 数组内部的内容，所以不需要写：

```c
buf = "./b";
```

数组名不能这样重新赋值。`p` 只是一个游标，负责指出当前要写入的位置。

如果错误地写成：

```c
p = buf;
*p = '/';
```

就会覆盖 `buf` 的第一个字符，而不是在字符串末尾追加 `/`。

## 六、第四个问题：`de.name != target` 不是字符串比较

下面的代码比较的是两个地址：

```c
if(de.name != target)
```

即使两个字符串内容都是 `"b"`，它们也可能位于不同的内存地址。
比较字符串内容必须使用：

```c
strcmp(de.name, target) == 0
```

因此：

```c
st.type == T_FILE && strcmp(de.name, target) == 0
```

才表示“当前目录项是普通文件，并且名字匹配目标”。

## 七、第五个问题：递归调用不能共享会变化的状态

递归函数中的这些变量都属于一次调用的状态：

```c
int fd;
char buf[512];
char *p;
struct dirent de;
struct stat st;
```

如果把它们定义在函数外，所有递归层都会共享同一份变量。子目录递归时会覆盖父目录的路径、文件描述符和目录项状态。

另外，即使写成：

```c
static char buf[512];
```

只要它是函数内的 `static` 变量，所有递归调用仍然共享这一份数组。
`static` 不是“每次函数调用创建一份新变量”的意思。

递归函数中的局部变量应该是普通局部变量：

```c
void
find(char *path, char *target)
{
  int fd;
  char buf[512];
  char *p;
  struct dirent de;
  struct stat st;
}
```

这样每次递归调用都有自己的变量。

## 八、第六个问题：`DIRSIZ` 不是完整路径长度

`DIRSIZ` 限制的是单个目录项名字的长度：

```c
char name[DIRSIZ];
```

它不代表完整路径的长度。

完整路径可能是：

```text
./b
./a/b
./a/c/d/b
```

因此下面的数组不适合作为递归路径缓冲区：

```c
char buf[DIRSIZ + 1];
```

即使单个文件名不超过 14 个字符，完整路径仍然可能超过 15 个字节。
路径缓冲区应该参考 xv6 的路径限制分配足够空间，并在拼接前检查剩余容量：

```c
if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
  fprintf(2, "find: path too long\n");
  return;
}
```

数组越界会覆盖栈或全局区域中的其他数据，造成：

- 路径变成 `/b`；
- 递归结果丢失；
- 加一条 `printf` 后行为改变；
- 创建文件顺序变化后结果变化。

这类现象不是 `printf` 产生了逻辑，而是程序已经存在未定义行为。

## 九、递归查找的正确思路

递归函数可以抽象成：

```text
find(current_path, target_name)
```

每层执行：

1. 打开当前路径；
2. 读取目录项；
3. 跳过无效目录项；
4. 跳过 `.` 和 `..`；
5. 拼出子路径；
6. 用 `stat` 判断子路径类型；
7. 如果是目录，递归进入；
8. 如果是普通文件且名字匹配，输出完整路径。

伪代码如下：

```text
find(path, target):
    打开 path
    读取 path 中的每个目录项 de
        如果 de 无效，跳过
        如果 de.name 是 "." 或 ".."，跳过

        child = path + "/" + de.name
        获取 child 的类型

        如果 child 是目录：
            find(child, target)
        否则如果 child 是普通文件且 de.name == target：
            输出 child
```

这个过程中：

```text
current_path 会变化
target_name 始终不变
```

## 十、第二部分：WSL 中 `git push` 失败

完成 xv6 代码后，还遇到了 GitHub 推送问题。典型报错是：

```text
Failed to connect to github.com port 443
```

排查后发现：

```text
github.com -> 20.205.243.166
```

这个地址当时无法连接，但其他 GitHub 地址可以连接。

所以问题分成两层：

1. GitHub 认证是否成功；
2. WSL 到 GitHub 的 HTTPS 网络是否可达。

认证成功并不代表网络线路一定正常；网络可达也不代表 GitHub 账号已经认证。

## 十一、GitHub CLI 认证

在 WSL 中执行：

```bash
gh auth login
```

交互选择：

```text
GitHub.com
HTTPS
Yes
Login with a web browser
```

登录后检查：

```bash
gh auth status
```

Git 应该使用 GitHub CLI 提供的凭据助手。

## 十二、自动选择可达的 GitHub IP

为了避免每次手动测试 IP，编写了 `github_push.py`。

脚本的核心做法是：

1. 通过 `socket.getaddrinfo` 获取 `github.com` 的 IPv4 地址；
2. 对每个地址建立 TCP 连接；
3. 在 443 端口上执行 TLS 握手；
4. 使用 `github.com` 作为 TLS 的 `server_hostname`；
5. 找到可用地址后，仅为本次 `git push` 添加：

```text
http.curloptResolve=github.com:443:<可用IP>
```

这样不会修改远程 URL，也不会永久修改系统 DNS。

脚本顶部只需要修改：

```python
ADD_PATHS = [
    "GITHUB_PUSH_WSL.md",
    "github_push.py",
]
COMMIT_MESSAGE = "Add documentation and push helper"
PUSH_REMOTE = "origin"
PUSH_BRANCH = "lab1"
```

运行：

```bash
python3 github_push.py
```

脚本依次执行：

```text
自动寻找可达 GitHub 地址
git add
git status
git commit
git push
```

关键代码如下：

```python
def find_reachable_github_ip() -> str:
    context = ssl.create_default_context()

    for address in github_candidates():
        try:
            with socket.create_connection(
                (address, 443), timeout=5
            ) as connection:
                with context.wrap_socket(
                    connection,
                    server_hostname="github.com",
                ):
                    return address
        except (OSError, ssl.SSLError):
            continue

    raise SystemExit("No reachable GitHub HTTPS address found.")
```

这里不能直接对 IP 执行普通 HTTPS 请求，因为 GitHub 依赖域名和 TLS SNI。
使用 `server_hostname="github.com"` 可以保证 TLS 握手仍然针对正确的域名。

## 十三、脚本的边界

这个脚本可以自动处理：

- DNS 返回多个 IPv4 地址；
- 某些 GitHub IP 443 端口不可达；
- 备用 IP 需要轮换；
- GitHub CLI 已经配置好认证。

但它不能保证：

- 本机完全断网时仍然能 push；
- GitHub 账号没有权限时仍然能 push；
- 没有登录 GitHub 时自动完成认证；
- 所有网络阻断都能通过换 IP 解决。

如果所有候选地址都失败，应检查：

```bash
curl -4 -I --connect-timeout 10 https://github.com
gh auth status
git remote -v
```

## 十四、最终经验

这次实验最重要的收获不是某一行代码，而是排查问题的顺序：

```text
先确认输入参数
再确认指针和字符串
再确认路径拼接
再确认递归状态
最后检查内存边界
```

Git 推送问题也要分层排查：

```text
认证问题
网络问题
DNS 问题
Git 配置问题
仓库权限问题
```

只有把问题拆成这些层次，才能避免把所有错误都归因于一个地方。

