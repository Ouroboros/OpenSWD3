# 入战FFD读取与旧记录保留

本批接通SDL与核心startup的FFD读取。FIGTALK、完整角色帧及实际续玩未验收。

## 原始指令与调用方

`451E60`先调用FIGTALK加载；`451E97`读取FFD头，`451EAD`读取FFD记录。
三处返回均不作为caller成功门。随后`451EB2..451ECF`从live记录发布两个
人数，零敌人数的已有提前返回合同不变。

头读取`45F130..45F1A2`：只读、独占、OPEN_EXISTING打开。
句柄FFFFFFFF仍传CloseHandle后返回0，不写对象头和输出表地址。
其他句柄读取2714h到this+4，忽略ReadFile返回与短读，发布this+1F48，
关闭并返回1。未读到的头部字节保留原值。

记录读取`45F1B0..45F29D`：再次打开、再次读头，battle参数截低WORD。
count按signed BYTE解释；count<=0或signed variant>count时关闭并返回0。
从1到battle-1累加signed BYTE，与signed variant按DWORD相加；查偏移表，
以低DWORD的`2714h + ordinal*10Ch`定位，读10Ch并关闭返回1。
不因seek失败、读失败或短读清空旧记录。count拒绝也不写记录。
内存访问typed-stop仍阻断后缀，不把越界伪装成普通返回0。

LST物理范围：caller157195..157285，头181330..181392，记录181400..181527。
完整指令已从原始LST读取，独立推导记录在
`build/tmp/runtime/battle-startup-data-load-audit.md`。

## 当前接线

`load_legacy_battle_startup_definition`共用已有的头/记录实现。
核心startup和SDL按同一顺序调用，只在三类真实表访问typed-stop时停止。
头、表地址和记录由startup原存储持有；普通失败后直接投影旧记录。

SDL不再用严格的总资产加载器控制FFD成功门，也不复制FFD记录到assets。
建场新增接受definition的入口，与便捷assets入口共用同一坐标及镜像实现。
SDL诊断改读实际加载结果。便捷总加载器仍用于离线资产测试，不能再作为
生产FFD的错误行为规格。它的旧严格检查未被冒充原caller合同。

文件名查找复用既有大小写适配，保持Linux对原资产名称的读取能力。
[实际文件所有权迁移](battle-definition-archive-owned-files.md)删除文件
FilePort、Runtime、编号句柄及寄存器协议。共享文件集合持有真实文件，
加载函数直接读、寻址；短读和关闭失败保留原行为，没有新增业务失败门。
发布头部索引使用相对同一绑定对象的字节偏移，避免残留旧地址编号。

## 验证

- 实际文件：双打开失败保留旧记录并继续解码；头部和记录短读保留
  未读后缀。目录读取的实际失败仍发布头部索引；没有模拟API回显。
- 旧模拟首开失败、次开成功用例删除；两次加载的代码顺序已按LST复核。
  两次失败与各次实际加载分别验证，未将它们冒充跨两次打开的故障注入。
- count拒绝继续使用旧记录；偏移表越界阻断解码；完整参数保留低WORD语义。
- 实际文件：混合大小写FFD、六字节记录、再次进入时两字节头、EOF及文件缺失。
  检查同一startup记录逐字节保留，没有另一个生产缓存。
- 实际battle98：新文件入口的头与记录逐字节匹配原资产读取，敌方400，
  坐标175/303；建场消费新定义后原队伍绑定断言继续通过。
- 历史接线core/ASan setup各1/1、SDL构建通过，日志为
  `build/tmp/runtime/battle-definition-binding-*.log`。
- 当前文件所有权接口的最终验证记录见所有权迁移文档。
  文件名断言检查实际文件身份，适用于大小写敏感或不敏感文件系统。

未启动游戏，未新增原版动态差分。B10保持315/422，316 pending_audit。

## 留给下一批的边界

后续[脚本文件批次](battle-script-file-runtime-binding.md)已修正持久句柄、
开关、定位和普通文件失败的继续行为；原栈缺失与分配仍有宿主边界。
完整startup仍有准备callee端口；不能声称该脚本callee已直接完整接通。
SDL对非法敌人数的建场检查与原逐角色停止顺序仍须独立核对。
