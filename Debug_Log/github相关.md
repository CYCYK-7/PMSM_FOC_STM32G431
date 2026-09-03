# 一直push失败解决方式

今天git push，一直失败显示.....reset，然后就让AI帮忙解决，开始是一直判断是不是HTTPS出了问题，但是花了很久也没用，我感觉是网的问题，所以后面就换了SSH连接的方式

## 一、先确认是不是 HTTPS 出问题

如果你执行：

```bash
git push
```

出现：

```text
fatal: unable to access 'https://github.com/...':
Recv failure: Connection was reset
```

先检查远程地址：

```bash
git remote -v
```

如果看到：

```text
origin  https://github.com/用户名/仓库.git
```

说明当前走的是 HTTPS。

然后测试远程仓库读取：

```bash
git ls-remote origin
```

如果这里也出现：

```text
Recv failure: Connection was reset
```

说明**不只是 push 有问题，而是整个 Git HTTPS 通道都有问题**。

可以进一步诊断：

```bash
GIT_TRACE=1 GIT_CURL_VERBOSE=1 git ls-remote origin
```

如果看到类似：

```text
TLS handshake, Client hello
...
Recv failure: Connection was reset
```

说明已经连到 GitHub 443 端口，但 **TLS 握手阶段被网络重置**。

还可以直接测试：

```bash
curl -Iv https://github.com/
```

如果这里也是：

```text
Recv failure: Connection was reset
```

基本就不是仓库、commit 或 GitHub 权限问题，而更可能是：

```text
当前网络
路由器
校园网/公司网
VPN/代理
安全软件
Windows 网络环境
```

这时候最有价值的测试是：

```text
换手机热点 → 再执行 git ls-remote origin
```

如果换网络后恢复正常，基本就可以确定是原网络的问题。

------

## 二、如果想改用 SSH

先检查电脑有没有 SSH 密钥：

```bash
ls ~/.ssh
```

如果已经有：

```text
id_rsa
id_rsa.pub
```

或者：

```text
id_ed25519
id_ed25519.pub
```

**不要重新生成，也不要覆盖。**

其中：

```text
id_rsa        私钥，绝对不要发给别人
id_rsa.pub    公钥，可以添加到 GitHub
```

如果完全没有密钥，才需要生成，例如：

```bash
ssh-keygen -t ed25519
```

------

## 三、测试 SSH 能不能连接 GitHub

执行：

```bash
ssh -T git@github.com
```

第一次一般会看到：

```text
The authenticity of host 'github.com ...' can't be established.
Are you sure you want to continue connecting?
```

确认指纹无误后输入：

```text
yes
```

然后 GitHub 会加入：

```text
~/.ssh/known_hosts
```

以后一般不会再询问。

------

## 四、如果出现 Permission denied (publickey)

例如：

```text
git@github.com: Permission denied (publickey).
```

说明：

```text
电脑 → GitHub SSH服务器   ✅
网络连接                  ✅
SSH协议                   ✅
你的身份验证              ❌
```

这时候先检查 SSH 到底用了哪把钥匙：

```bash
ssh -vT git@github.com
```

重点找：

```text
Offering public key: .../.ssh/id_rsa
```

如果确实提供了你的 `id_rsa`，但最后还是：

```text
Permission denied (publickey)
```

说明：

> GitHub 账号里没有登记这把公钥。

------

## 五、把公钥添加到 GitHub

如果你用的是 RSA：

```bash
cat ~/.ssh/id_rsa.pub
```

如果是 Ed25519：

```bash
cat ~/.ssh/id_ed25519.pub
```

复制输出的**完整一行**。

然后进入：

**GitHub → Settings → SSH and GPG keys → New SSH key**

填写：

```text
Title:
My Windows PC
```

`Key type`：

```text
Authentication Key
```

把 `.pub` 文件内容粘进去，然后保存。

注意：

**只能复制：**

```bash
cat ~/.ssh/id_rsa.pub
```

不要把：

```bash
cat ~/.ssh/id_rsa
```

的内容发给别人。后者是私钥。

------

## 六、重新测试 SSH

执行：

```bash
ssh -T git@github.com
```

如果看到：

```text
Hi 你的用户名! You've successfully authenticated,
but GitHub does not provide shell access.
```

就表示 SSH 已经成功。

其中：

```text
GitHub does not provide shell access
```

**不是错误。**

真正关键的是：

```text
You've successfully authenticated
```

------

## 七、把仓库从 HTTPS 切成 SSH

先看当前地址：

```bash
git remote -v
```

假设原来是：

```text
https://github.com/CYCYK-7/PMSM_FOC_STM32G431.git
```

改成：

```bash
git remote set-url origin git@github.com:CYCYK-7/PMSM_FOC_STM32G431.git
```

然后检查：

```bash
git remote -v
```

应该变成：

```text
origin  git@github.com:CYCYK-7/PMSM_FOC_STM32G431.git (fetch)
origin  git@github.com:CYCYK-7/PMSM_FOC_STM32G431.git (push)
```

------

## 八、以后正常使用

SSH 配好后，你以后 Git 的使用习惯基本不变：

```bash
git add -A
git commit -m "本次修改说明"
git push
```

你不需要每次重新配 SSH。

可以把整个流程记成一句：

```text
HTTPS push失败
    ↓
git ls-remote origin
    ↓
curl -Iv https://github.com
    ↓
确认是不是网络/TLS问题
    ↓
ssh -T git@github.com
    ↓
Permission denied？
    ↓
检查 ~/.ssh 公钥
    ↓
把 *.pub 加到 GitHub
    ↓
ssh -T 再测试
    ↓
git remote set-url origin git@github.com:用户名/仓库.git
    ↓
git push
```

你这次最终真正解决 SSH 的关键点就是：**电脑本身已有 `id_rsa`，SSH 也能连接 GitHub，但 GitHub 账号没有认可这把公钥；重新把 `id_rsa.pub` 加到 GitHub 后，身份验证链路才完整。**