# 战斗角色链表资料应用 `0x004705C0`

历史状态：`platform_adapted`。工作包282修正profile所有权与停止点；旧callee统计与全量门不作为当前发布验收。

## 1. 完整权威范围

权威LST主体为`0x004705C0..0x004707A9`，proc至endp共232行、144条实际指令、3个call、23个跳转、15个局部标签、3个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭索引提交一次、待审资料加载一次和平台字符串复制一次。

## 2. 前缀与扫描

category选择器映射为0到`0x10`、1到`0x0C`、2到`0x1001`、其他保持。occurrence为零时立即返回低word `0xFFFF`，不会发布mode flag、清缓冲或提交索引。

type选择器0时对actor mode byte OR `0x80`并选择type 28；选择器1时OR `0x02`并选择type 31；其他保持原type且不改mode。随后清零40字节profile buffer，提交索引，并从主链筛选mode byte与`0x05`、category flags与mask。type 28接受节点27至30，其他type只接受31。匹配计数先增后比，链尾未找到返回`0xFFFF`并保留前缀副作用。

## 3. 命中应用

命中时先把节点`+0x5C`零扩展写调用者dword，再按节点profile id加载actor profile buffer。随后清零actor `+0x2630`的16字节块，并按原`lstrcpyA`顺序复制节点字符串；typed目标不足时在首次越界字节停止并保留已写前缀。

actor mode byte bit1命中时复制节点`+0x54`、`+0x5C`和`+0x4C`到三个actor模式字段。节点`+0x48`等于零时复制`+0x40/+0x42/+0x44`到三项派生word。随后无条件清profile copy latch；`+0x48` bit9命中时设latch 1并再次复制三项派生word。

最终返回先检查映射mask低byte bit3与节点category bit3，均命中返回1；否则返回映射mask低byte bit4。该规则保留自定义mask和三种内建selector的非对称结果。

## 4. owner、stop与caller

主链复用既有链表状态；profile直接借用组A执行状态的唯一缓冲，16字节前置块仍属于final-processing；mode byte和派生word仍借用物品效果状态。新增三个应用字段仍属于同一final owner。缺失owner、节点或字符串目标时在首次访问处typed-stop。唯一caller位于已关闭目标选择刷新函数，现有边界尚未暴露节点物化owner，本包不复制第二份状态；生产源码无旧地址调用。

## 5. 工作包282资料停止点

profile视图缺失时，不要求final-processing提前存在：先保留已执行的mode BYTE写入，在十DWORD清理处停止，EAX=0、ECX=10、EDX保留输入。profile存在而pre-effect视图缺失时，保留profile清零、索引提交和实际MON加载；`0x004706B3..0x004706C0`已令EAX/EDX=`actor+0x2630`、ECX=0，随后才在第一个pre-effect DWORD写入处停止。

新增测试独立检查这两个前缀及寄存器，不把缺失视图静默替换为零缓冲。core29/ASan19定向`1/1`通过（2.91/4.76秒），无匹配诊断，diff check通过。其他actor投影、跨入口绑定和本包全量发布仍未完成。

## 6. 历史验证状态

测试覆盖零occurrence无副作用、type0 mode flag、双缓冲清零、profile加载、字符串复制、三项派生word和mask bit4返回。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`191/422 = 182 platform_adapted + 9 assembly_exact + 231 pending_audit`，SHA256为`935fb2f925e84d3ec110a903f76519d1105ea2e46df0a9b71598c3cae630ab1c`。动态差分因原版链表、资料加载、字符串目标、actor模式字段与caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
