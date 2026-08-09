# 旧 waveOut 协商与 SDL3 sample 输出边界

最后更新：2026-08-09

状态：协商状态机为 `assembly_exact`；SDL3 设备、PCM 转换和混音为
`platform_adapted`、`asset_verified`

完整 LST 是唯一行为真值；IDA 伪码只用于定位。平台实现没有启动原版 EXE。

## 1. `0x004859B0` 的协商初值

SND 表打开成功后，原函数固定写入：

| 对象偏移 | 初值 | 语义 |
|---:|---:|---|
| `+0x60` | `44100` | sample rate |
| `+0x64` | `16` | bits per sample |
| `+0x68` | `2` | channels |

随后先调用 `AIL_set_preference(15, 0)`。每次尝试都在全局 16 字节
`WAVEFORMATEX` 前缀重建 PCM 格式：format tag 为一，声道和采样率取上述字段，
每样本字节数使用 `(bits + 7) / 8` 的有符号整数路径，block align 为其乘声道数，
average bytes per second 再乘采样率。

`AIL_waveOutOpen(&driver, -1, 0, &format)` 返回零才是成功；核心端口只在边界把它
规范化为 `open_output() == true`，没有把返回约定传播到业务调用者。

## 2. 精确重试与回退阶梯

成功打开后，函数先取得 digital configuration 文本，再读取 preference 15：

- preference 非零时直接接受；
- preference 为零且配置文本不含大小写精确的 `Emulated` 时直接接受；
- preference 为零且包含 `Emulated` 时关闭刚打开的 driver，连续两次执行
  `set_preference(15, 1)`，然后以完全相同格式重试。

打开失败时先复制 `AIL_last_error()`：

- preference 15 仍为零：改为一，以相同格式重试；
- preference 15 已为一：采样率按正数有符号除二；16-bit 降到 11025 以下时重置为
  44100 并切到 8-bit，8-bit 再降到 11025 以下才失败。

因此全部失败时的实际尝试序列为：

```text
44100/16, 44100/16, 22050/16, 11025/16,
44100/8,  22050/8,  11025/8
```

第一次同格式重复来自 preference 0→1，不得误写成直接降采样率。曾经出现的错误文本
在后续成功时仍留在原对象错误缓冲；核心结果也保留该历史文本。

## 3. 成功状态与 sample pool

接受 driver 后，`+0x54` 和 `+0x58` 同时写一。函数随后读取 preference 一，将结果减
八写入 `+0x5C`；只有大于 16 时才钳至 16，零和负数不做下限钳制。pool 分配和部分
handle 失败语义由 sample-manager 证据固定。

销毁 `0x00485C20` 的尾部顺序为：释放 active 节点、释放 free 节点、关闭 SND 表和
文件，最后对 `+0x50` 调用 `AIL_waveOutClose`。重写的 manager 按同一边界最后调用
backend `close_output()`；未完成初始化的宿主资源只在 C++ 进程退出安全边界回收。

## 4. SDL3 平台替代

兼容核心不包含 SDL 类型。`SdlLegacySampleBackend` 同时实现协商端口和 sample 端口：

1. 按协商得到的 8/16-bit、双声道和采样率请求 SDL3 默认逻辑播放设备；宿主设备可由
   SDL3 转换到实际 WASAPI、DirectSound、ALSA 等格式。
2. 建立 F32 双声道混音流；每个旧 sample handle 保存独立的 PCM、游标、循环次数、
   volume、pan、user-data 和 status。
3. `set_sample_file` 读取原加载器返回值固定 `+0x14..+0x2B` PCM 头，以声明 data 长度
   为消费边界，再由 SDL3 转为协商采样率的 F32 stereo。
4. callback 按当前 volume/pan 动态混合；播放中 status 为四，完成或 end 后为二，供
   `0x00486080` 的原回收条件消费。

当前 `all.snd` 的 664 项全是类型零 RIFF。真实资产扫描确认 13 种
channels/sample-rate/byte-rate/block-align/bits 组合都能转换；ID 506/507 的块标签仍为
`00 00 74 61`，平台解析器按原固定字段接受，没有修复资产字节。两个跨块长 view 的
字节和声明长度也没有改写。

当前包没有会进入 named `.mp3` 分支的样本，因此 SDL3 backend 在没有独立 MP3 decoder
时让该 setup 返回失败，继续触发原 `0x00485CE0` 的 buffer 释放和 free-list 回滚。
该分支状态为 `unreachable_current_assets`，不能据此从兼容核心删除。

Miles 内部重采样器、混音饱和和 pan 曲线不在 EXE 汇编内。SDL3 的线性声像与格式转换
明确标记为 `platform_adapted`；核心测试只锁定传入参数、调用顺序和 PCM 边界，不把不同
宿主设备的模拟波形冒充 `original_diff_verified`。

## 5. 验证

- fake backend 逐事件锁定 preference、open、configuration、error、close 和七次回退
  格式；包括 `Emulated` 的 close 与两次相同 preference 写入。
- SDL dummy audio 测试覆盖逻辑设备、handle、PCM setup、user-data、无限循环、
  status 四/二和关闭。
- 真实 `all.snd` 测试覆盖全部 13 种 PCM 格式组合及异常 data 标签。
- Linux Clang core 为 69/69；Linux Clang app 与 Windows LLVM app 均为 73/73 CTest
  通过。没有启动原版或重写 EXE。
