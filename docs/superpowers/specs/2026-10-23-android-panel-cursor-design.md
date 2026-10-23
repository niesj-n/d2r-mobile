# Android 面板光标停靠设计

基线 d4614a77f9372b09218a26bd20e084e34ece7e8c；原版唯一来源 jhaizhou-ops/d2r-mobile，固定审阅 e3756f8942b40eba6c851e881828a5b1bda17fd6。此项是下一版独立工作，不改当前 root APK，不改 iOS、玩法或 200-float 绘制 ABI。

只在 Android guest 输入采样线程维护说明框期限和上次绑定状态。复用 dm_ui_free_cursor、holding、bind、touchactive、pulse、cursorpending 的现有读数，并由现有 dm_cursor_set_screen 发布。避免共享 helper/改动 UIKit；单独工具提示 View 无法取消游戏光标说明，因此不采用。

原版 front_ios.m:351、467–477、655–661 的精确行为：进入绑定态2时先置 (.155*width,.5*height)；刚进入自由光标面板等待.3s；普通物品单击及技能/属性点选保持说明（-1），直到别处/换目标/关面板；拖放/使用等结束后2.8s；普通界面鼠标点击后.05s。期限到达且没有手指/持物/绑定/未完成点击/脉冲/未完成光标命令时置 (.02*width,.5*height)。暂停页等无自由光标的页不发布停靠。触点取消、失焦、resize、场景变化清停靠状态；不清二次加点 selected_point 以外的现有规则，不发送新购买/使用/排序命令。

文件边界：dm/android/touch_android.c；dm/android/tests/test_touch_snapshot.c（现有真实状态机fixture扩展，宿主边界改可观测）；新增本设计/计划。重点验证精确期限、保持说明、门控恢复、绑定边沿与真实比例、取消/换页、二次点加点、Sort物理DOWN不重复。无设备，所有结果只能称源码fixture与语法验证。

Android 同步采样边界的两项保守适配：进入绑定态时若正有触点/持物/未完成点击或脉冲，延后到安全空闲再进行一次初始停靠；离开自由光标页时撤销本功能自有停车请求并重新等待下一次.3s，不清玩家触点建立的选择/拖动请求。fixture分别覆盖这些边界。39个真实桥用例/19882次检查通过；NDK29 Android31语法检查通过；未验设备。后续由root安排独立审查及下一版集成。
