# 剧情 VM 商店请求 `0x0042C234`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`；商店UI与商品链物化归后续B9 owner。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C234..0x0042C2C5`

opcode：133

## 1. 物理记录与长度

记录是零终止u16 item id列表：

```text
+0          u16 opcode
+2          u16 item_id[0]
...
+2 + 2*N    u16 zero terminator
```

物理长度固定为：

```text
4 + 2 * item_count
```

机器从`+2`开始逐word扫描；首word为零时item count为0。扫描只在读到u16零时停止，不因ID范围、高位或固定buffer容量提前停止。count在扫描结束后以`AND 0xFFFF`保留低16位；当前`0x8000`窗口内不会达到回绕域。

## 2. owner替换与复制顺序

handler在读取任何脚本word前固定执行：

```text
free(dword_4B7928)
dword_4B7928 = malloc(0x100)
```

旧owner无论是否为空都先交给通用delete wrapper。新owner分配后立即发布全局指针，但不清零。

扫描结束后，机器把`item_count * 2`字节从脚本`+2`复制到新buffer；先按dword复制，再复制0..3个尾字节。最后在目标`item_count * 2`写u16零终止。复制不包含脚本中的terminator，但会写入等价终止word。

`0x100`字节只能安全容纳127个非零u16 ID和一个零终止word。原版没有容量检查：128项及以上在terminator已找到后发生目标buffer越界。现代保持“先释放旧owner、先分配新owner、完整扫描、最后判断容量”的顺序，并在原copy/write危险点返回`shop_item_list_out_of_range`；不发布mode、IP或previous。缺terminator同样保留已替换为空的新owner。

## 3. 请求与控制流

复制完成后按顺序执行：

```text
dword_4B8740 = 0x80000002
script_pointer += 4 + 2 * item_count
instruction_ip += 4 + 2 * item_count
```

随后恢复ESI为该dispatch轮入口保存的零，经`0x0042B0AE` common join发布normalized previous133并yield。handler没有audio service、action callback或same-call continuation。

共享`dword_4B8740`已由现代frame coordinator持有；低28位模式2进入`shop_mode_2`分派。VM成功请求后世界剧情在同帧停止，下一接受帧进入商店分支。

若现代special-mode binding缺失，失败发生在完整商品ID列表已经提交之后；mode、IP和previous保持不变。这对应原固定全局写入点的typed owner边界。

## 4. 商店consumer与生命周期

原商店状态在`0x0044E566..0x0044E621`读取`dword_4B7928`：逐u16 ID调用：

```text
sub_44D2D0(shop_root, item_id, 99, mode=1)
```

以此建立临时商品哨兵链。consumer在首个零终止word停止，随后进入商店排序与UI状态。普通item定义仍由MON loader提供；该外部物化和商店UI属于B9，不在opcode133内伪造。

商店状态1退出时，`0x0044E9E2..0x0044E9F7`调用`sub_44DA40`；后者先释放商品ID buffer并把`dword_4B7928`清零，再释放商店临时链。`sub_40E0B0`世界全局初始化不重置该owner。

现代以VM进程期`vector<u16>`承接商品ID owner。每次opcode133先用空vector交换释放旧容量，再reserve原物理128 words，列表只保存非零ID；vector长度承担终止边界。模式2退出的最终B9 consumer将负责消费并清理该owner。unchecked malloc、无界source scan和固定buffer写以RAII与typed-stop隔离，合法域的item顺序、宽度和请求时点不变。

## 5. 资产与验证

完整线性TALK目录锁定22条物理记录/22 probes：TALK1 15条、TALK3 7条，TALK2/TALK4无记录。item count范围1..13，共196个item引用、127种不同ID，ID范围501..1059；物理长度6..30。全部记录raw word为`0x0085`。

四文件基础raw `0x0085`字样为`32/33/21/7`，高位alias `0x4085/0x8085/0xC085`均为零。只有线性目录中的22处作为资产记录；其余字样不冒充入口。

真实代表`TALK1.DAT@0x00007AEE`长度16，item列表为：

```text
501, 502, 503, 521, 851, 855
```

synthetic覆盖四raw alias、旧owner替换、空列表、127项最大合法容量、128项越界、缺terminator精确窗口尾、完整列表后缺special-mode owner，以及一项列表在`IP=0x7FFA`精确结束于`0x8000`的yield。真实代表记录回放通过。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
