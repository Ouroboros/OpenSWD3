# 剧情 VM 四项角色资源调整 `0x0042C2C6`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`；后继opcode144保持独立`pending_audit`，不继承本handler完成状态。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C2C6..0x0042C3AF`

opcode：134

## 1. 物理记录

```text
+0  u16 opcode
+2  u16 selector
+4  u16 first delta
+6  u16 second delta
+8  u16 third delta
```

物理长度固定10字节。selector只接受1..4；机器先对u16 selector执行`DEC`，再按无符号`< 4`判断。selector 0、5..65535进入无效分支；诊断参数是回绕后的`selector - 1`。

无效分支只读取selector，不读取三个delta或角色资源owner，随后固定消费10字节、发布normalized previous134并same-call继续。空诊断`nullsub_1`不产生现代业务副作用。

## 2. 四项记录与三段提交

有效selector先转为零基index，再按：

```text
((index * 8) - index) * 8 = index * 0x38
```

定位四项进程级`0x38`角色记录。当前handler只访问：

```text
+0x04 current first
+0x06 current second
+0x08 current third
+0x0A first limit
+0x0C second limit
+0x0E third limit
+0x24 transient word
```

全局数组基址从`0x004AB790`开始，因此LST标签分别显示为`word_4AB794..word_4AB79E`和`word_4AB7B4`。`sub_411030`与`sub_4112B0`对同一`0x38`步长记录提供17种field set/get；战斗和持久化路径也读取这些记录。现代以VM进程状态中的四项中性资源owner承接本handler所需字段，剩余记录字段及真实B10/B11加载不在本工作包伪造。

三个delta严格分阶段读取和提交：

```text
current_first  = u16(current_first  + raw_delta_1)
current_second = u16(current_second + raw_delta_2)
current_third  = u16(current_third  + raw_delta_3)
```

每一步都是16位回绕加法。后续operand截断保留此前已经完成的加法；尚未进入任何上下界夹值、self-modification、transient清理、IP或previous发布。

## 3. signed夹值与自修改

三个delta均提交后，机器按first、second、third顺序把current与各自limit作signed i16比较；仅当`current > limit`时把current覆盖为limit。

三项上界全部处理后才执行下界：

1. first以signed i16判断`<= 0`；成立时先写零，再把物理下一word `[script + 0x0A]`覆盖为基础raw opcode `0x0090`（opcode144）。
2. second以signed i16判断`< 0`；成立时写零。
3. third以signed i16判断`< 0`；成立时写零。
4. 无条件把选中记录的transient word写零。

first的等零条件与后二项严格负条件不相同。所有比较都使用i16；例如u16加法回绕到`0x8000..0xFFFF`后按负值进入下界，不能用无符号夹值代替。

下一word改写发生在first写零之后、second/third下界和transient清理之前。若完整10字节记录恰好结束于`0x8000`且first非正，原版会向窗口外写下一word；现代在该原始危险点返回`operand_out_of_range`，保留三项上界处理与first清零，不执行后续下界、transient、IP或previous。

成功改写后common same-call会立即以新IP执行opcode144。该独立handler在`0x0042C79D`根据下一byte请求特殊模式4/5并清若干模式状态；它仍按workpack顺序保持`pending_audit`。当前测试只证明opcode134已发布previous并same-call抵达未实现的144，不提前实现或继承其模式副作用。

## 4. 控制流

valid与invalid成功路径均固定：

```text
IP += 10
previous = 134
same-call continue
```

handler没有audio service、yield或外部callback。first保持正值时不读取或验证下一word；因此完整记录可精确结束于窗口尾，完成全部状态、IP和previous后由下一fetch返回窗口越界。

## 5. 资产与验证

完整线性TALK目录锁定47条物理记录/47 probes：TALK1/TALK2/TALK3/TALK4分别25/10/4/8条。selector 1/2/3/4分别14/11/11/11条；全部在合法域。

44条记录的三个delta均为10000，用于把三项current夹到各自limit。另三条selector1记录只降低first：两条`-100`、一条`-200`，后二项delta为零。物理下一opcode分布包含33条连续opcode134、4条terminator、4条opcode7及6条其他后继。

四文件基础raw `0x0086`字样为`83/39/23/13`，高位alias `0x4086/0x8086/0xC086`均为零。只有线性目录中的47处作为资产记录。

真实恢复代表从`TALK1.DAT@0x000233AF`开始连续四条selector1..4、三个delta均10000，随后为opcode171；回放确认四项current全部按各自limit恢复、transient清零、previous134发布，并same-call停在尚未实现的171。真实损伤代表`TALK1.DAT@0x000299AF`为selector1、first delta `-100`、后二项零；回放确认first从50降至负值后清零，把原terminator改为opcode144，并same-call抵达该pending handler。

synthetic覆盖四raw alias、u16回绕后i16负值、signed上界、first等/小于零、second/third严格负值、三个operand截断点、invalid selector不读余下record、成功精确窗口尾，以及self-modification恰落窗口外的部分副作用顺序。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
