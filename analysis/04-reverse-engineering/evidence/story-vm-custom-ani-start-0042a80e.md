# 剧情 VM 自定义 ANI 启动 `0x0042A80E`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A80E..0x0042AD37`；帧更新 `0x004154A0..0x00415B70`；首帧loader `0x004158C0`

opcode：`96`

## 1. 记录解析与先行副作用

机器先以`sub_40DD30/sub_40DD20`把帧间隔改为70，再分配并清零两个0x400临时buffer，随后固定消费opcode两字节。filename前允许两个按顺序独立的单字节前缀：

```text
'%' -> dword_4B7AA0 |= 2   // skip reveal，首相位直接为1
'*' -> dword_4B7AA0 |= 1   // ending color effect
filename bytes
25 51 (%Q)
```

原版从prefix之后无条件复制32 bytes到临时buffer，再在其中无界扫描首个`%Q`；找到后把`%`改零，并把context IP推进到terminator之后。modern不伪造unchecked malloc或相邻内存：保持frame interval先写，在原32-byte复制域内bounded扫描；缺terminator返回`ani_filename_terminator_not_found`，保留interval70但不推进IP/previous。opcode后无首字节或prefix后截断返回`operand_out_of_range`，同样保留interval副作用。

完整记录恰好结束在`0x8000`合法；filename、prefix、IP和后续start均先完成。

## 2. 路径、CD与audio顺序

原版先尝试`<root>\Video\name`，失败时调用CD helper并可改用`<cd>\swd3\Video\name`；CD helper返回2时，记录已经消费且至少一次`AIL_serve`已发生，但直接从解释器返回0，不进入common previous join，两个临时buffer泄漏。

SDL配置data root替代CD介质；`prepare_story_ani()`正常恒ready，`Video/`子目录按ASCII大小写不敏感解析脚本名。modern synthetic preflight-false路径保持：interval与记录消费完成、一次audio service、无start、previous不发布、非fatal跨帧返回。preflight成功路径在actual start前执行第二次audio service，完成后经common join发布previous并执行第三次audio service；原header/palette读取间的额外maintenance封装在typed backend边界。

## 3. 实际 ANI backend

SDL端口持有真实`asset_runtime::LegacyAniActivity`并调用：

```text
activity.start(resolved Video path,
               parsed flags,
               current process flags,
               current scene low flags,
               current RGB565 conversion)
```

`LegacyAniActivity::start`先关闭旧activity，再通过`LegacyAniArchive`打开/校验0x24 header、分配framebuffer backup、装载frame1与palette，写入header display width、flags、scene flags和process active bit；`%`对应phase1，否则phase=-13；`*`保留ending effect。成功后SDL同步外部process flags。

archive/open/header/frame1/allocation失败在原版均已消费记录并最终common join或崩溃；modern backend把失败类型化，VM仍按原open-failure业务路径发布previous96并yield。机器final file-open失败发生在清旧ANI节点之前；SDL resolver因此在missing path时不调用会先关闭activity的typed start，保留旧activity/process bit。进入typed start后的header/frame1/allocation失败则同步backend已清理的process状态。最终LST复审确认scene flags不是opcode96的staged读取：机器在异步ANI ending更新`0x004155AD`才读取scene global。VM因此不增加scene owner stop；SDL activity port持有live scene reference，并在建立typed activity时传入当前低位状态。

原版`%`分支还直接清一次目标surface。opcode97闭环包现已在成功start后按`%`清黑，并把world-frame `ani_activity_004154a0` stage接入实际update；normal ending使用start前world snapshot适配原递归scene redraw，finalize后下一帧恢复live world composition。因此本证据升级为`sdl_runtime_integrated`。

## 4. 资产锁与验证

线性TALK目录锁定16条物理记录/16 probes，全部raw `0x0060`：

```text
TALK1/2/3/4 = 2/2/9/3
prefix: %*=14, *=1, %=1
physical length: 14..18 bytes
```

13种脚本拼写覆盖`expv.ani`、`memory.ani`、`LiLiaDie.ani`、`chaoswar.ani`、`withdraw.ani`、`getsword.ani`等；全部在配置root的`Video/`中找到大小写不敏感匹配。

real Story VM回放：

- `TALK1.DAT@0x000043FA`：`%*expv.ani%Q`，flags3；
- `TALK2.DAT@0x0000D39F`：`*memory.ani%Q`，flags1。

两条均置于精确窗口尾，验证interval70、两次handler内部audio与一次common audio（合计三次）、filename/flags传递、IP=`8000`、previous96和yield。synthetic另覆盖四raw alias、四prefix组合、32-byte terminator边界、missing terminator、opcode/prefix截断、preflight退出、archive-open失败，以及VM scene owner为空仍不引入机器中不存在的staged stop。

独立backend门：`asset_runtime.legacy_ani_archive`、`legacy_ani_archive_real`、`legacy_ani_activity`为3/3；real archive测试实际打开并校验`expv.Ani`和`memory.Ani`等资产。Story VM三项为3/3，linux-app integration编译通过。

分类：`platform_adapted`。记录长度/prefix/flags、frame interval先行、消费点、audio/preflight分支、open失败common join、无人工scene staged读取、previous/yield和实际ANI state保持；Win32 CD/路径、unchecked分配、无界扫描、DirectDraw surface与裸文件对象由配置root、case-insensitive resolver、bounded scan和`LegacyAniActivity` typed owner替代。
