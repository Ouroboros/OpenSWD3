# 剧情 VM 视频启动 `0x0042A611`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序CD检查与Bink backend动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A611..0x0042A66E`，helper `0x00484730..0x00484910`

opcode：`85`

## 1. handler固定顺序

原handler在调用字符串/helper前固定执行：

1. 以`dword_4A0E7C * dword_4A0E74`字节数清零软件framebuffer；
2. 调用primary surface入口，以flags `0x01000000`提交整个软件surface；
3. `_AIL_serve()`；
4. 调用`sub_484730(&script_pointer, &instruction_offset)`；
5. 进入`loc_42B0AA`公共join，发布normalized previous85；
6. `var_28|ESI==0`，再调用`_AIL_serve()`一次并yield。

现代成功顺序为`clear_story_framebuffer -> present_story_framebuffer -> service_audio -> prepare_story_video -> begin_story_video -> common service_audio`。测试以独立callbacks固定六步顺序；成功和预检拒绝都发布previous85并以两次audio yield。

## 2. CD预检路径

helper先分配/清零0x400路径缓冲，再用`sub_4118B0("<CD marker>",0,1,&drive)`执行CD checker。返回2时原版：

- OR进`dword_4B7A9C` bit2（process close requested）；
- 不读取脚本文件名，不推进IP；
- 泄漏刚分配的0x400缓冲；
- 返回caller并在common join发布previous85/yield。

SDL使用配置data root替代CD介质获取，`prepare_story_video()`恒成功。测试port可返回false，锁定“不消费filename但previous85/yield”的流控；process-close bit和泄漏不在VM owner内，属于平台适配。

## 3. `%Q`解析与exact-tail

预检成功后helper从`ip+2`逐byte复制，直到当前word为little-endian`0x5125`（`%Q`）。每复制一byte即递增脚本指针和16位IP；命中后再推进2，最终IP位于terminator之后。

现代bounded扫描在缺terminator时于清屏、提交、audio service和预检之后返回`operand_out_of_range`，IP/previous保持；替代原版越界扫描。

独立重审发现旧C++以`end == window.size()`判断“未找到”，把合法记录恰好结束在`0x8000`误判失败。现改为checked bool：`%Q`结束位置可等于窗口边界，并在全部副作用后正确发布previous85/yield。四raw alias精确尾测试锁定该修复。

## 4. 路径、扩展与视频启动适配

原helper建立`video\swd3\`前缀，把script filename拼入0x400缓冲；随后对第一个case-sensitive `.avi`和第一个`.mpg`分别原地改成`.bik`。它分配0x484 wrapper、调用Bink open/volume入口、释放路径缓冲，并OR process bit `0x20`。

SDL port保留raw filename边界，把配置data directory映射到`<root>/Video/<filename>`；`legacy_bink_filename`以相同case-sensitive首匹配规则转换`.avi/.mpg`。配置目录替代原固定`swd3`根；`LegacyVideoPlayer`对open失败/立即完成采用typed backend状态，只在playing时设置现代video-active位。这些属于已测试的平台适配，不改变VM解析/IP/yield合同。

## 5. 资产锁与测试

线性TALK目录含11条物理记录/11 probes：

```text
TALK1.DAT 6
TALK2.DAT 2
TALK3.DAT 1
TALK4.DAT 2
```

全部raw `0x0055`并以`%Q`结束。长度分布为10、12、13、14、15×4、16×2、177；常规文件名为1条`.bik`和9条`.mpg`。`TALK1.DAT@0x00004231`是一条177-byte opaque记录，包含嵌入NUL/脚本样式字节，但线性合同仍把后方`%Q`作为终点；不把它改写成常规文件名。

real CTest独立回放：

- `TALK1.DAT@0x000044FF`：`OPENING.bik`，长度15；
- `TALK1.DAT@0x00004634`：`Demo.mpg`，长度12。

两条均置于精确尾，验证raw filename、六步side-effect owner、两次audio、IP=`0x8000`、previous85与yield。既有Story100继续端到端经过OPENING视频边界。synthetic还覆盖`.avi`、四alias、CD预检拒绝和缺terminator typed failure。剧情VM三项为3/3。

分类：`platform_adapted`。VM合法域清屏/提交/audio/预检/解析/begin顺序、terminator、IP、previous与yield保持；CD acquisition、固定路径、Bink wrapper和unsafe扫描由配置目录、typed backend与bounded失败替代。
