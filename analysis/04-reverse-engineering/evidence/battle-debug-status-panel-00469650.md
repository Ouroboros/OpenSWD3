# 战斗调试状态面板 `0x00469650`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469650..0x004698D8`，从proc到endp共283行、196条带机器码和真实助记符的实际指令、18个静态call、7个跳转、6个局部/返回标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于战斗逐帧协调器`0x00453200`，调用时机严格在对话绘制完成之后、两组倒计时之前。

18个callsite由以下调用组成：动作更新1次、矩形1次、九宫格2次、文字2次、共享stage推进1次、颜色打包5次、`lstrcpyA`1次、`wsprintfA`2次、x87向零qword转换1次和颜色渐变矩形1次。`wsprintfA`由寄存器间接调用；它与两个字符串调用点仍计入完整函数审计。

## 2. 面板壳与stage门

函数先以单byte种子初始化8-byte局部文字，其余7 byte清零。只有战斗调试位`0x20`非零才继续；关闭路径不访问动态profile，也不修改共享stage。

开启后复用胜利结算唯一`panel_action_record`，只写动作`0x233B`和base variant零。矩形固定为`x=236,y=86,width=184,height=live_stage+40`，参数尾保持`0,4,4,0`原顺序。第一九宫格资源保留矩形返回EAX高word，只用live动作`field_4a`替换低word，边界为`240,90..416,106`。随后以动态profile token在`304,90`绘制标题，颜色`0xFFC0`、字号16。第二九宫格资源保留标题文字返回ECX高word并替换相同低word，边界为`240,122..416,live_stage+122`。

两处高度分别在原访问点重读共享stage，不缓存入口值。随后直连已关闭`0x00469620`，固定参数为`base=122,target=302,divisor=3`。只有返回EAX精确等于1才访问九项动态状态；非1时保留壳、标题、第二框和stage写回并立即返回。

## 3. 九项signed状态

动态profile对象首部作为标题文字，`+0x92`起九个byte按i8解释。标签token表顺序固定为：

```text
004A718C 004A7174 004A7184 004A7170 004A716C
004A7180 004A7188 004A7168 004A7164
```

九行文字固定X为252，Y从122开始每行增加20；渐变固定X为284，Y从130开始每行增加20。每行保留以下分支与调用顺序：

- 值为0：先查询`16,16,16`颜色，再复制`--`，宽度float为2；
- 值大于0：先查询`28,2,2`颜色，再以`%2d0%%`格式化，宽度为`value*0.1*120`；
- 值小于0：先查询`2,13,28`颜色，再格式化绝对值，宽度为`value*(-0.1)*120`；
- 值小于`-10`：普通负值分支的颜色、格式和float副作用已完成后，再查询`2,28,13`、再次格式化，并以`(value+10)*(-0.1)*120`覆盖宽度。

局部格式文字在本函数中不传给文字callee，但`lstrcpyA/wsprintfA`写入及缺省尾byte仍是可观察栈副作用，不能删除。每行随后固定查询`31,31,31`文字色，按标签表绘字；float宽度经已审`0x00489654`控制字向零qword转换，只取低32位传给已关闭`0x00450A50`，绘制3像素高渐变。循环固定九次，无现代上限或提前终止。

正常循环尾返回EAX为0，EDX为标签表尾`0x004A79CC`，ECX保留最后一次渐变callee返回。动态profile缺失只在标题文字首次真实访问点typed-stop；面板动作、矩形和第一九宫格前缀保留。

## 4. owner与caller回收

`0x0053C4B8`动态profile token和其九个signed状态快照由目标选择runtime唯一承接；共享stage继续使用同一runtime的`transition_stage`。动作记录复用胜利奖励owner，颜色渐变槽复用选择提示owner，framebuffer、像素格式、矩形、九宫格、字体和文字均复用既有typed接口，不建立物理副本。

战斗逐帧协调器已在对话之后直接调用typed面板，并在typed-stop时阻断两组倒计时及全部后续帧尾。旧`post_dialog_stage`枚举槽保留数值但改名为reserved，生产代码零调用；测试不再借该槽伪造面板行为。

## 5. 验证

定向测试覆盖调试位关闭、动态profile首次真实访问停点、壳绘制后stage非1早退、stage商零完整九行、两层资源高低word来源、live stage几何、九个标签与20像素行距、零/正/负/小于`-10`分支、深负值双格式与双颜色副作用、x87宽度、标题及行文字入口寄存器、循环尾寄存器和主帧reserved槽零调用。

定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning；app仅有既有ALSA开发库CMake提示。

原版动态profile、动作/矩形/九宫格、字体/文字、framebuffer、颜色渐变、动态栈地址及主帧联合寄存器捕获后端尚不可用，`original_diff_verified`为`blocked_runtime_oracle`。
