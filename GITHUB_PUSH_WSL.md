# WSL 中配置 GitHub Push

## 自动提交并推送脚本

仓库根目录提供了 [github_push.py](./github_push.py)。每次提交前只需要编辑脚本开头的配置：

```python
ADD_PATHS = [
    "user/find.c",
]
COMMIT_MESSAGE = "描述本次修改"
PUSH_REMOTE = "origin"
PUSH_BRANCH = "lab1"
```

脚本会自动解析 `github.com` 的 IPv4 地址，并逐个测试 HTTPS 443 端口；如果 DNS 返回的地址不可达，会继续尝试内置的备用地址。找到可用地址后，脚本只对本次 `git push` 使用该地址，不会永久修改系统 DNS 或 Git 全局配置。

然后在仓库根目录执行：

```bash
python3 github_push.py
```

脚本会依次执行：

```text
解析并测试 GitHub 地址
git add
git status
git commit
git push
```

脚本不会自动添加未列出的文件，也不会自动生成 commit message。运行前请确认 `ADD_PATHS` 和 `COMMIT_MESSAGE` 正确；如果某一步失败，脚本会停止并显示失败的 Git 命令。

> 脚本会继承 Git 的全局配置，包括 GitHub CLI 凭据。网络地址只通过本次 push 的 `http.curloptResolve` 临时指定。

本教程适用于 WSL 中使用 HTTPS 推送 GitHub 仓库。配置分为两部分：

1. GitHub 身份认证；
2. WSL 到 GitHub 的网络连接。

## 1. 检查仓库远程地址

在仓库目录执行：

```bash
git remote -v
```

HTTPS 远程地址应类似：

```text
https://github.com/<用户名>/<仓库名>.git
```

## 2. 登录 GitHub CLI

执行：

```bash
gh auth login
```

交互选择：

1. `GitHub.com`
2. `HTTPS`
3. `Yes`（使用 GitHub 凭据认证 Git）
4. `Login with a web browser`

登录后检查：

```bash
gh auth status
```

应看到当前账号已登录，并且 Git 操作协议是 `https`。

## 3. 检查网络

```bash
curl -4 -I --connect-timeout 10 https://github.com
```

如果返回 `HTTP/2 200`，说明当前网络可以直接访问 GitHub。

如果超时，检查 GitHub 的解析结果：

```bash
getent ahostsv4 github.com
```

## 4. 配置 Git 使用可达的 GitHub 地址

如果 DNS 返回的地址无法连接，可以为 Git 配置一个可达的 GitHub IP。下面的 IP 是本机测试时可用的示例：

```bash
git config --global --add http.curloptResolve github.com:443:140.82.112.3
```

验证配置：

```bash
git config --global --get-all http.curloptResolve
git ls-remote origin
```

`http.curloptResolve` 只对 Git 的 HTTP(S) 请求生效，并且仍然使用 `github.com` 作为 TLS 和 HTTP 主机名，不需要修改远程 URL。

> GitHub 的 IP 可能变化。如果这个地址以后失效，先测试新的可达 IP，再替换旧配置。不要把 IP 永久写入 `/etc/hosts`。

## 5. 推送代码

先查看工作区：

```bash
git status
```

确认要提交的修改后：

```bash
git add <文件>
git commit -m "描述本次修改"
git push origin <分支名>
```

如果当前分支已经设置了 upstream，之后可以直接：

```bash
git push
```

## 6. 常用排查

### 检查认证

```bash
gh auth status
```

### 检查 Git 凭据助手

```bash
git config --global --get-regexp 'credential\..*helper'
```

GitHub CLI 通常会配置：

```text
!/usr/bin/gh auth git-credential
```

### 检查 Git 代理

```bash
git config --global --get http.proxy
git config --global --get https.proxy
```

如果设置了错误代理，可以删除：

```bash
git config --global --unset http.proxy
git config --global --unset https.proxy
```

### 删除失效的固定 IP

```bash
git config --global --unset-all http.curloptResolve
```

删除后，Git 会恢复使用系统 DNS。

## 7. 最短工作流

配置完成后，日常使用：

```bash
git status
git add .
git commit -m "描述本次修改"
git push
```

如果 `git push` 报网络连接错误，依次检查：

```bash
curl -4 -I --connect-timeout 10 https://github.com
getent ahostsv4 github.com
gh auth status
```

网络错误和认证错误是两类不同问题：

- `Failed to connect`、`Connection timed out`：网络或线路问题；
- `Authentication failed`、`403`：认证或仓库权限问题。
