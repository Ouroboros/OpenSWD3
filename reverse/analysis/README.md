# reverse/analysis 入口

| 文件 | 内容 |
| --- | --- |
| `swd32.i64` | 唯一正式 IDAT 工作数据库（无 FLIRT） |
| `work/swd32.exe` | 数据库输入工作副本（与原 `swd32.exe` 哈希一致） |
| `input-baseline.md` | 输入、哈希、PE 信息、导入、IDAT 设置与重开核对 |
| `structure-overview.md` | 主流程、地址分区、初步模块划分 |
| `scope-and-gaps.md` | 扫描范围、函数边界补全过程、未闭合缺口 |
| `module_assignment.tsv` | 各模块函数计数 |
| `talk-script.md` | 剧本文件 Talk<n>.dat 的载入与全部指令（解释器 0x00427920） |
| `maps-dat.md` | maps.dat 的偏移表与各表记录格式 |
| `globals.tsv` | 架构 v3 的全局变量对照：原地址、还原它的成员（类型::成员）、类型、声明位置与说明；由 `scripts/list_globals.sh` 从 src/ 头文件的成员注释生成，提交前重新生成 |
| `data_map.tsv` | 架构 v2 旧表（生成已删除的 globals.h），不再维护，仅作参考；以 `globals.tsv` 为准 |
| `../functions.tsv` 的 notes 列 | 已实现函数的行为说明，取自源码中函数定义前的 `// swd32.exe 0x<地址>:` 注释，由 `scripts/register.sh` 写入；原有备注保留为 `[prior: …]` |
| `ida/` | IDAT 导出：`functions_raw.tsv`、`func_refs.tsv`、`indirect_sites.tsv`、`code_drefs.tsv`、`text_pointer_scan.tsv`、`nonfunc_gaps.tsv`、`unowned_*.tsv`、`sp_audit.tsv`、`meta_*.json`、各步日志与 `dump*.txt` |
| `scripts/` | IDAPython 与辅助脚本 |

## IDAT 调用方式（WSL）

只使用指定 IDAT；`-S` 参数整体作为一个 shell 参数传入，脚本参数用空格分隔；路径使用 Windows 形式。

```sh
cd reverse/analysis
TVHEADLESS=1 WSLENV=TVHEADLESS/w /mnt/d/Dev/Crack/IDA/idat.exe -A \
  '-SE:\Game\swd3\swd3-rebuild\reverse\analysis\scripts\dump.py E:\Game\swd3\swd3-rebuild\tmp\dump.txt 401000-401100' \
  'E:\Game\swd3\swd3-rebuild\reverse\analysis\swd32.i64'
```

按项目规则，IDAT 与脚本通过托管后台进程运行。脚本结束时调用 `idc.qexit()`；修改数据库的脚本先 `save_database` 再退出。试验性修改先在工作区 `tmp/` 复制一份临时库，结束后删除。

| 脚本 | 用途 |
| --- | --- |
| `scripts/analyze.py analyze <outdir>` | 新建库时清除 AF_FLIRT、等待自动分析、导出全部清单并保存 |
| `scripts/analyze.py reopen <outdir>` | 重开核对（AF_FLIRT、unknown_libname、输入哈希）并重新导出 |
| `scripts/analyze.py <tag> <outdir>` | 只导出（`meta_<tag>.json` 与各 TSV） |
| `scripts/boundary.py <outdir> <pass>` | 函数外代码区域起点建函数并逐项校验 |
| `scripts/fix_boundaries*.py <outdir>` | 已执行的边界修正（记录用） |
| `scripts/scan_ptrs.py <outdir>` | 数据段指向 `.text` 的指针扫描 |
| `scripts/func_refs.py <outdir>` | 每函数 API／字符串／全局读写引用 |
| `scripts/dump.py <outfile> <s-e,...>` | 按地址范围导出带归属与交叉引用的反汇编 |
| `scripts/classify_modules.js` | 由导出生成 `../functions.tsv` 初始模块（node 运行；会覆盖函数表，仅用于初始化） |
| `scripts/fingerprints.py <asm 目录> <exe> <out.jsonl> <funclets.tsv>` | 由 `asm/` 清单计算函数指纹，供与外传 swd3e.exe 配对（WSL 中 `python3 -I` 运行，不需 IDAT）：call 位移、本函数内跳转（标签序号）、尾调用、引用目标与落在本程序段内的其余 4 字节值规范化后的函数／基本块 SHA-1，形状（数字与名字一律为 N）的函数／基本块 SHA-1，被调用者（经导入桩解析到 DLL!名称）、字符串、导入、全局地址与 `.rdata` 常量；switch 按表内顺序加入 case 标签；EH funclet 并入父函数。环境变量 `FINGERPRINT_TOKENS=<目录>` 另写每函数的记号、形状与原文。产出 `export/swd32_fingerprints.jsonl`、`export/swd32_funclets.tsv` |
| `scripts/match_functions.js <a.jsonl> <a 记号目录> <b.jsonl> <b 记号目录> <输出目录>` | 两个程序的函数配对（node 运行）：hash、形状、字符串／导入锚点、调用图传播、相似度互为最佳、邻接填空；由形状相同的配对得出全局地址与结构偏移对照。输出 functions.tsv（含可复用判定）、globals.tsv、offsets.tsv、summary.txt |
