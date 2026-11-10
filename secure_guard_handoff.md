# secure_guard 修复交接（2026-11-10）

## 位置与状态

- 用户要求：修 `oplus_secure_guard_new` 已发现的接口问题，使用自己的内核仓库，不在官方 vendor 仓库落修改。
- 仓库：`https://github.com/1jsuwgsjw/Action-Build.git`；本地 `D:/手机项目/pulls/Action-Build`。
- 分支：`fix/kpm-unload-lifetime`；修改基线 `c5d9120f172fdb86a0a5fd8573c002319f0a74b6`。
- 源码检查阶段尚未提交/推送、未编译完整内核、未操作手机；用户随后已要求进入 Ace 6 CI 编译，构建状态以新提交的 Actions 运行结果为准。上一会话创建的 `pulls/OnePlus-secureguard` 没有用于交付，也没有改动它。

## 修改

补丁：`kernel_patches/56_restrict_secure_guard_receiver_android15_6.6.patch`。
目标：固定 common `c1802dd6f3343e71c166a3c67c50554c81880b6e` 的 `net/netlink/genetlink.c`。

现有构建输出 common/Image，不重编原厂 vendor_dlkm `.ko`，因此在 Generic Netlink 分发入口补接收检查，仅匹配 family `secure_guard`、命令 SENDPID（1）：

1. 为本次分发设置 `GENL_ADMIN_PERM`，复用内核 `netlink_capable(skb, CAP_NET_ADMIN)` 检查；不是按进程名放行。
2. 按严格 native-endian `NLA_U32` 解析 MSG（属性1）和可选 OPT（属性2），拒绝错误长度，不再用字符串策略解析整数。
3. MSG 必须存在，且等于非零 `NETLINK_CB(skb).portid`。验证的是厂商回调将读取的同一份解析结果，重复属性也不能造成前后检查不同对象。失败在厂商 pre_doit/doit 之前返回并释放解析数组。

没有新增 Hook 模块、导出符号或改变模块 ABI；不禁用 secure_guard、不改事件内容、不修改其他 family。
这是 common 接收路径的补偿修复，**不是替换或全面修复厂商模块二进制**。有权限的服务仍可注册或重新注册；未改模块接收者生命周期及上报路径，不宣称阻断所有厂商信息通道。

工作流：`.github/workflows/Build Kernel OnePlus.yml` 新增 `Restrict Oplus secure_guard receiver`。
该步在编译之前执行，门控 `VM_BRIDGE_BUILD`（当前由 `FILE == oneplus_ace_6` 得到）。补丁来自本仓库按 `GITHUB_SHA` 检出的 `kernel_workspace/susfs-snapshot-source/kernel_patches`，不是中间克隆的上游 Action-Build，也不依赖 SUSFS 补丁应用分支。执行 `git apply --check` 后应用。

## 已检查与未检查

已通过：
- 本地源码副本与固定 common 提交的原始文件一致。
- 在临时目录执行新增工作流步骤，真实应用补丁；`git apply --reverse --check` 通过。
- YAML 解析、Ace 6 门控、步骤顺序、新增 Bash 段语法、仓库 diff 空白检查。
- 源码核对：权限检查早于属性解析/厂商回调，地址检查早于 pre_doit；固定内核的严格 NLA_U32 验证拒绝非4字节载荷。

未检查：完整内核编译、启动、设备请求处理、厂商 daemon 兼容性、游戏效果。上述静态及补丁应用检查不是内核运行测试。

合法客户端现在必须通过 CAP_NET_ADMIN 检查，携带4字节 MSG，内容为其实际绑定 socket 的 portid；不能假定一定等于进程 PID。厂商服务实际能力和报文格式尚未取得，不承诺无兼容性影响。

## 手机与后续

上一会话的实机证据：模块 `oplus_secure_guard_new` 已加载，bootconfig 为 orange/unlocked，SELinux Enforcing；在与 PID1 相同 netns 完整枚举53个 Generic Netlink family，**没有 secure_guard**。这不是本轮重新查询。证据位于项目根 `_ai2_tmp/vendor-interface-source/findings.md` 及 `live-family-query.txt`。

所以当前不能把本补丁当作已证实的游戏检测修复。手机 KPM 上次已回退 p1；activity2 是未部署候选，与这次内核补丁分开。

下一步若继续交付可刷包：先把本轮三个文件提交到自有仓库并完成 Ace 6 CI 编译。刷机和厂商服务兼容性验证尚未执行；不要为验证而在当前手机盲目发送 SENDPID、卸载模块或制造提权事件。
