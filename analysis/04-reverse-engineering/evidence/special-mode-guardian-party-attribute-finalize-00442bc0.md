# 护驾party属性字段展开 `0x00442BC0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442BC0..0x00442C9F`，81行，无FUNCTION CHUNK，无callee；callers为442AA0与442B10，现均直接回收。

## 十七项映射

源为scratch signed i16，目标为cache signed i32。源offset依次为：

`04,06,08,0A,0C,0E,10,12,14,16,18,1A,1C,1E,20,26,28`

目标offset依次为：

`00,04,08,0C,10,14,18,1C,20,24,28,2C,30,34,38,3C,40`

每项严格执行i16符号扩展到i32，再以little-endian dword写入。前十五项连续取scratch `+4..+0x20`；最后两项刻意跳过`+0x22/+0x24`并取B10专门清零或44D6E0累计的`+0x26/+0x28`。

目标只需`0x44`字节，不覆盖`0x44..0x4F`。越界typed-stop位于原首个destination dword写入点之前，不产生任何目标写。

原返回值是目标裸pointer。modern端以`attribute_cache_token + destination_offset`的u32回绕值表达，并bit-cast为i32；业务层不暴露host pointer。

AA0与B10原`finalize_guardian_party_attribute_record`端口已删除。两者成功时直接调用BC0并保留17次merge/16次merge后的helper计数；BC0目标越界分别映射为`party_finalization_stopped`和`selected_finalization_stopped`。

UT写入`-1`、`INT16_MIN`及正值，核对目标`FFFFFFFF`、`FFFF8000`和17项末值、token+offset返回及越界无写；同时重验AA0/B10和40630。修正旧fixture“原样复制scratch”的错误假设：真实cache首dword来自scratch `+4`，不是scratch `+0`。定向测试通过。

workpack双生成稳定为`93/227`，SHA256均为`18dd9c9ff2e4f471f3a6e6be9c07a360502a45973fc33209efc94a0fa39e3fd0`；下一单元`0x00442CA0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
