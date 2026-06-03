# SSH 密钥配置笔记

> 配置日期：2026-06-03
> 环境：Windows 11 / Git Bash
> GitHub 账户：Moqi-ui

---

## 一、SSH 是什么

SSH（Secure Shell）是一种加密网络协议。在 Git 场景下，SSH 密钥对用来**代替密码**向 GitHub 证明身份。

核心原理：
- **私钥**留在本地电脑，永远不要分享
- **公钥**上传到 GitHub
- 推送/拉取代码时，本地用私钥签名，GitHub 用公钥验证

---

## 二、配置流程（共 6 步）

### 步骤 1：检查是否已有 SSH 密钥

```bash
ls -al ~/.ssh/
```

命令拆解：
- `ls`：list，列出目录下的文件
- `-a`：显示所有文件（包括 `.` 开头的隐藏文件）
- `-l`：以长格式显示（权限、大小、日期等）
- `~`：用户主目录，Windows 上即 `C:\Users\用户名`
- `.ssh/`：SSH 配置目录（隐藏文件夹）

如果看到 `id_ed25519` 和 `id_ed25519.pub`，说明已有密钥，可跳过步骤 2。
如果只有 `known_hosts` 或目录为空，说明没有密钥，继续下一步。

---

### 步骤 2：生成新的 SSH 密钥

```bash
ssh-keygen -t ed25519 -C "你的GitHub邮箱"
```

命令拆解：
- `ssh-keygen`：SSH 密钥生成工具
- `-t ed25519`：指定算法为 Ed25519（比 RSA 更短更安全更快，目前最推荐）
- `-C "邮箱"`：注释，方便日后识别密钥归属

执行后两个交互提示：
1. **保存位置** → 直接回车，使用默认路径 `~/.ssh/id_ed25519`
2. **passphrase（口令）** → 可留空回车，也可设密码保护私钥

生成结果：`.ssh/` 下出现两个文件：
- `id_ed25519` → 私钥（不要分享）
- `id_ed25519.pub` → 公钥（上传到 GitHub）

---

### 步骤 3：启动 ssh-agent 并添加私钥

```bash
# 启动 ssh-agent（后台密钥管理器）
eval "$(ssh-agent -s)"

# 将私钥添加到 agent
ssh-add ~/.ssh/id_ed25519
```

命令拆解：
- `ssh-agent`：后台运行的密钥管理器，管理你的私钥，避免每次 git 操作都重复认证
- `-s`：输出适合 shell 使用的格式
- `eval "$(…)"`：将 ssh-agent 输出的环境变量在当前 shell 中生效
- `ssh-add`：将私钥交给 agent 管理

成功标志：
- `Agent pid XXXX`（agent 已启动）
- `Identity added: /c/Users/…/.ssh/id_ed25519`（私钥已加载）

---

### 步骤 4：复制公钥

```bash
cat ~/.ssh/id_ed25519.pub
```

命令拆解：
- `cat`：concatenate，将文件内容输出到屏幕

会输出一行以 `ssh-ed25519` 开头的字符串，完整复制这一行，下一步要用。

> ⚠️ 注意复制的是 `.pub` 公钥文件，不要复制私钥！

---

### 步骤 5：将公钥添加到 GitHub

浏览器操作：
1. 打开 https://github.com/settings/keys
2. 点击 **New SSH key**
3. 填写：
   - **Title**：随便起名，如 `My-Laptop`（标识哪台电脑）
   - **Key type**：保持默认 **Authentication Key**
   - **Key**：粘贴步骤 4 复制的公钥
4. 点击 **Add SSH key**，GitHub 可能要求确认密码

---

### 步骤 6：测试 SSH 连接

```bash
ssh -T git@github.com
```

命令拆解：
- `ssh`：SSH 客户端
- `-T`：不分配终端（只测试认证）
- `git@github.com`：以 git 用户身份连接 GitHub

第一次连接会提示 `Are you sure you want to continue connecting?`，输入 `yes` 回车。

成功标志：
```
Hi Moqi-ui! You've successfully authenticated, but GitHub does not provide shell access.
```

---

## 三、配置完成后如何使用

### 克隆仓库

```bash
# SSH 方式（现在可用）
git clone git@github.com:Moqi-ui/仓库名.git

# HTTPS 方式（之前的做法）
git clone https://github.com/Moqi-ui/仓库名.git
```

### 已有仓库从 HTTPS 切换到 SSH

```bash
# 查看当前远程地址
git remote -v

# 切换为 SSH 地址
git remote set-url origin git@github.com:Moqi-ui/仓库名.git
```

---

## 四、常见问题

### Q1：一个密钥可以访问多个仓库吗？
**可以。** SSH 密钥绑定的是整个 GitHub 账户，不是单个仓库。添加公钥后，该账户下所有仓库都可以通过 SSH 访问。

### Q2：多台电脑怎么办？
每台电脑各自生成一对密钥，将各自的公钥都添加到 GitHub 即可（GitHub 支持添加多个 SSH key）。

### Q3：SSH 和 GPG 有什么区别？
| 维度 | SSH 密钥 | GPG 密钥 |
|------|----------|----------|
| 用途 | 身份认证（push/pull/clone） | 给 commit 签名 |
| 效果 | 替代 HTTPS 密码 | commit 旁显示绿色 "Verified" 标签 |
| 是否必须 | 用 SSH 协议时必须 | 完全可选 |

### Q4：ssh-agent 每次打开 Git Bash 都要重新启动吗？
是的。`eval "$(ssh-agent -s)"` 只在当前 shell 会话生效。关闭 Git Bash 后 agent 就停止了。如果不希望每次手动执行，可以在 `~/.bashrc` 或 `~/.bash_profile` 中添加自动启动的配置。

### Q5：passphrase 忘了怎么办？
无法找回。只能重新生成密钥，然后更新 GitHub 上的公钥。

### Q6：SourceTree 提示 SSH 密钥认证失败？
**原因：** SourceTree 默认使用 PuTTY/Plink 作为 SSH 客户端，而我们生成的密钥是 OpenSSH 格式，两者不兼容。

**解决方法：** 切换 SourceTree 的 SSH 客户端为 OpenSSH，无需重新生成密钥。

操作步骤：
1. 打开 SourceTree → 菜单栏 **工具** → **选项**
2. 选择 **一般** 选项卡
3. 找到 **SSH 客户端配置** 区域
4. 将 SSH 客户端从 `PuTTY/Plink` 改为 **`OpenSSH`**
5. 点击 **确定**

> 💡 补充说明：
> - **PuTTY/Plink**：使用 `.ppk` 格式的密钥（PuTTY 专用格式），密钥由 PuTTYgen 生成，由 Pageant 管理
> - **OpenSSH**：使用 `id_ed25519` 格式的密钥（标准 SSH 格式），密钥由 `ssh-keygen` 生成，由 `ssh-agent` 管理
> - 我们在步骤 2 中用 `ssh-keygen` 生成的是 OpenSSH 格式密钥，所以 SourceTree 也需要切换为 OpenSSH 客户端

---

## 五、命令速查表

```bash
# 检查密钥
ls -al ~/.ssh/

# 生成密钥
ssh-keygen -t ed25519 -C "邮箱"

# 启动 agent
eval "$(ssh-agent -s)"

# 添加私钥
ssh-add ~/.ssh/id_ed25519

# 查看公钥
cat ~/.ssh/id_ed25519.pub

# 测试连接
ssh -T git@github.com

# 切换远程地址为 SSH
git remote set-url origin git@github.com:用户名/仓库名.git
```
