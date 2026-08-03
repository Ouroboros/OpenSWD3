# 启动 UI PE 资源证据

状态：DIALOG 模板与 BITMAP 映射已确认；运行时资产已从原 EXE 固化

来源：`swd3.exe`

SHA-256：`0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c`

## 1. 提取与保存位置

PE `.rsrc` 目录由 `objdump -x swd3.exe` 复核，资源用以下命令提取：

```text
7z x swd3.exe '.rsrc/BITMAP/*' '.rsrc/DIALOG/*' -o<temporary-directory>
```

7-Zip 为 RT_BITMAP 的 DIB 数据补入 14 字节 BMP 文件头。运行时 BMP 保存于
`assets/ui/startup/`；原始 DIALOG 字节保存于
`analysis/04-reverse-engineering/artifacts/pe-resources/`。OpenSWD3 运行时不再读取原 EXE。

## 2. DIALOG 资源

| ID | 用途 | 原始大小 | 保存文件 | SHA-256 |
|---:|---|---:|---|---|
| 106 | 其他启动相关模板，当前只留证 | 586 | `dialog-0106.bin` | `85653a7befb7ec908fc84f06299e603fd7306892162c0a94565a592d4bf57655` |
| 107 (`0x6B`) | 主启动对话框 | 274 | `dialog-0107.bin` | `9bd9f282bc78a572abab0e591ceab3ae4409f943d3c7960b117bbcda52530d1f` |
| 112 (`0x70`) | 辅助文本对话框 | 208 | `dialog-0112.bin` | `adcc7ebfdbbb3eee6b7b1b8cd0bf5362268dcc1f17c8188f03f96701a5aecfb8` |

DIALOG 107 的模板声明六个静态 bitmap 控件：背景控件 `0x405` 引用 BITMAP
113，悬停控件 `0x40E..0x412` 依次引用 BITMAP 114..118。这与
`0x00424440` 在初始化时移动的六个控件和五个显示状态字节逐项一致，不是按资源编号猜测。

## 3. BITMAP 资源

| ID | 控件 | 尺寸/位深 | 保存文件 | SHA-256 |
|---:|---:|---|---|---|
| 113 | `0x405` | 640×480×8 | `bitmap-0113-background.bmp` | `8e3ac9b8dbd90c75816bf46f0dd50a658c3b95dc3869d46ff21318cdcac24905` |
| 114 | `0x40E` | 40×182×8 | `bitmap-0114-hover-0.bmp` | `b8ffc7af7791c5396e889f08a2622acaf40fa1a41b80afba9791b1e328ad0e5a` |
| 115 | `0x40F` | 40×182×8 | `bitmap-0115-hover-1.bmp` | `c32b967840e75ea04c2986bfa15823bc52e0a766dac6ccdf85a63aa1febacae5` |
| 116 | `0x410` | 40×182×8 | `bitmap-0116-hover-2.bmp` | `015e3ecc11e58942027044c421b05e6547f1d187f52db1c9cf5bf6bef37aae3a` |
| 117 | `0x411` | 40×182×8 | `bitmap-0117-hover-3.bmp` | `51e4c8af2d499ac50d86b0b700136749c0f2abd474295036692940e6adcc2b81` |
| 118 | `0x412` | 40×182×24 | `bitmap-0118-hover-4.bmp` | `82a9025f94eb60452f7ac691df9d65f15fd89e09c00a1695202d948e5d2fe001` |

## 4. SDL3 映射

`SdlStartupDialog` 从可执行文件旁的 `assets/ui/startup/` 载入六张 BMP，背景按
640×480 绘制，五张悬停图按汇编恢复的 `kStartupDialogControlLayouts` 和显示值
零/五绘制。鼠标移动、左键和关闭事件继续调用平台无关状态机；官网与 Readme
动作进入 `SdlExternalLaunchPorts`。

原资源加载失败时，SDL 平台层返回启动结果六并退出。这是为避免现代系统进入
无界面、无输入的挂起状态而隔离的 `platform_adapted` 启动兼容行为，不改变任何
游戏逻辑分支。悬停声音属于音频模块，存档存在扫描属于持久化模块，辅助文本框
属于输入/UI 接线；三者尚不能由本资源映射宣称完成。
