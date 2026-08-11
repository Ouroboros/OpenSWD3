# 当前地图私有 TSW 帧加载证据

状态：静态汇编顺序、现代实现与真实首帧集成均已验证

来源：`swd3.exe.lst` 完整汇编。汇编是唯一行为真值。

## 1. `0xFFFF` 不是第七个 TSW 包

`sub_431C50` 在 `0x00431CC9` 比较资源号低 16 位。资源号等于 `0xFFFF` 时，
`0x00431CF9..0x00431D0F` 把变体号低 16 位传给 `sub_40AD10`；其他资源才进入六个
普通 TSW 包的分段路径。因此 B6 的 `LegacyTswRuntime` 只拥有消费端口，实际生产者
属于当前地图会话。

## 2. 目录来自当前 LMF 地图

`sub_425BE0` 在 `0x00426256..0x0042626A` 读取数量并复制相对偏移到
`0x004CDEA8`；随后对每个相对偏移执行：

```text
seek(map_base + relative_offset + 0x0C)
read_u32(compressed_size)
```

数量保存在 `0x004CDEA4`，每项压缩长度保存在 `0x004CEAAC[index]`，地图块基址
保存在 `0x004CEAA8`。这正是现有 `LegacyLmfReferencedRecordDirectory`，不是
`header.offset_18` 指向的 0x1A 字节 indexed-object 目录。

## 3. `sub_40AD10` 的固定读取和转换顺序

`0x0040AD10..0x0040AE1D` 的有效资产路径为：

1. 清空输出帧主指针；变体号大于目录数量才失败。
2. 按目录压缩长度分配 `compressed_size + 0x10` 字节。
3. 从 `map_base + relative_offset` 读取完整 0x10 字节头和压缩载荷。
4. 头字段 `+0x02/+0x04` 左移四位后暂存宽高，`+0x06` 是深度，`+0x08` 是
   解压目标大小，`+0x0C` 是传给解压器的压缩长度。
5. 调用 `sub_426820` 解压 `record + 0x10`。
6. 深度等于 16 时调用 `sub_401B70`，只转换 word 命令流中的 literal 像素；其他
   深度调用 `sub_401E50`，把前 512 字节内嵌调色板和 indexed 命令流重建为 word
   命令流。
7. 两个转换函数都从命令流头重新发布最终宽高；函数返回 1。

原版的 `index == count` 会继续访问全局数组下一槽。现代实现只在无效资产边界拒绝
该越界访问，不改变当前游戏数据有效记录的结果。

## 4. 实现与验证

- `LegacyWorldSpecialFrameLoader` 绑定 `huge.lmf`、当前地图块偏移、referenced-record
  目录和当前像素转换状态，并实现 B6 预留的特殊帧端口。
- 新地图 owner 建立前先解除旧 loader 并清除 TSW 缓存，防止相同 `0xFFFF/variant`
  键跨地图错误复用；新会话稳定后再绑定 loader。
- 合成 UT 覆盖 direct-16、512 字节内嵌调色板 indexed 路径以及无效变体边界。
- 当前游戏数据的新游戏地图 81 真实 `MAPS + LMF + ACT + TSW + framebuffer` 会话已完成
  一次普通世界帧，抵达唯一画面提交槽和两次音频维护槽。初始地图角色使用的 20 个
  `0xFFFF` 私有帧均不再落入 `special_loader_unavailable`。

未恢复的世界帧 stage 仍在原调用槽显式转交；本单元只证明当前已接线路径和第一帧，
不据此宣称所有地图切换或世界逻辑已经完成。
