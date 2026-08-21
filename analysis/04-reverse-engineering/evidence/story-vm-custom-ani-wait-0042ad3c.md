# 剧情 VM 自定义 ANI 完成等待 `0x0042AD3C`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042AD3C..0x0042AD70`；ANI帧更新：`0x004154A0..0x00415749`

opcode：`97`

## 1. handler谓词与两条join路径

机器只读取`dword_4CAE8C`。该全局在opcode96成功读取ANI header后由header `+0x0E display_width`写入，ANI finalize在`0x00415715`清零；handler不读取file object、phase、flags或任何operand。

- owner非零：IP不推进、frame interval不变，ESI保持0；直接进入common join，发布previous97并yield。
- owner为零：IP先`+2`，ESI设1，再以`sub_40DD30/sub_40DD20`把frame interval恢复为`0x23`（35）；进入common join发布previous97并在同一VM调用继续。

完整两字节记录结束在`0x8000`时，完成路径的IP/interval/previous先提交，紧接的same-call fetch返回`instruction_out_of_range`；此前副作用不回滚。

modern port使用`LegacyAniActivity::is_active()`等价承载header-derived active extent，不增加nullable VM owner或机器中不存在的失败分支。

## 2. SDL ANI frame runtime

opcode97只有在activity owner被逐帧推进后才能完成。本包把ordinary-world composition的`ani_activity_004154a0` stage从deferred hook接入实际`LegacyAniActivity::update`：

- 每帧composition前以`is_active()`同步`LegacyWorldFrameState::ani_activity_active`；
- 以真实0x96000 framebuffer byte span和0x500 pitch执行reveal、playback、ending与finalize；
- 三个机器blocker分别映射dialog消息链、packed-row效果链和role-head action链；activity内部flag10继续参与阻塞；
- update前读取live scene low flags，update后同步scene/process flags；finalize service `0x23`恢复frame interval35；
- ending color路径复用assembly-audited `adjust_legacy_rgb_channels`与read-guard像素owner；
- frame load、span或颜色失败使frame stage显式失败，不伪造activity完成。

原版normal ending临时把active extent清零并递归重绘live world；当前composition stage不能安全递归进入完整world coordinator。SDL在opcode96 actual start前保存当前world framebuffer，`%`成功start按机器先清黑，normal ending恢复该snapshot并执行edge reveal，activity finalize后的下一帧恢复live world composition。这是有界、确定性的最小平台适配；不会让opcode97永久等待。

missing ANI path仍在调用会先close旧activity的typed start之前失败，保留机器file-open failure尚未清旧节点/process bit的顺序。进入typed start后的header/frame/allocation failure则同步backend已关闭的process状态。

## 3. 资产锁与验证

线性TALK目录锁定14条物理记录/14 probes，全部raw `0x0061`、长度2：

```text
TALK1/2/3/4 = 2/2/8/2
```

前序分布包括opcode96两条、opcode99六条、opcode59四条，以及opcode82/98各一条；这只作资产观察，不继承任何handler结论。

synthetic覆盖四raw alias的active等待、inactive完成、interval35、两路previous、精确尾和正常same-call继续。real回放使用`TALK1.DAT@0x00004408`：先以active owner验证原地yield，再清active验证IP=`8000`、interval35、previous97及后续fetch失败。

依赖门继续运行`asset_runtime.legacy_ani_archive`、`legacy_ani_archive_real`、`legacy_ani_activity`，覆盖真实archive/header/palette/frame与完整reveal/playback/ending/finalize状态机。Story VM三项与Linux core/app完整门共同验证handler和SDL集成；未启动原版或OpenSWD3游戏EXE。

分类：`platform_adapted`。active extent谓词、IP、frame interval35、previous、yield/same-call和真实activity完成均保持；Win32裸全局/DirectDraw递归重绘由typed activity、live owner同步和snapshot reveal替代。
