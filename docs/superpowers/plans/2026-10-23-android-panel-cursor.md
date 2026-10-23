# Android 面板光标停靠实现计划

目标：按固定原版的真实手势接 Android 面板光标停靠与绑定初始停靠。只在专用 worktree 推进；不请求额外用户确认，既有授权已明确范围和原版策略。
架构：touch_android.c 状态+guest采样推进；真实桥/共用状态机fixture检验，无新ABI或共享实现。
技术栈：C、Zig 0.14.1 host fixture、NDK29 Android31 syntax。

- [x] 扩展宿主读数与光标发布观察fixture，新增开面板.3s与绑定边沿失败测试；实际运行红测。
- [x] 最小增加Android期限/绑定状态，发布初始/空闲停靠；验证第一组绿测。
- [x] TDD扩展真实触点释放分类：普通按钮.05s、拖放/使用2.8s、plain item与point selection保持；补安全门控与取消边界。
- [x] 运行整套现有touch fixture，重点保持200 ABI、Quick三圆与原生Sort物理DOWN latch；NDK29语法检查。
- [x] 检查diff边界，记录证据到仓外cursor-parity-next/verification.md；提交clean分支返回root。实机说明可读性/布局与游戏行为留待设备验收。

