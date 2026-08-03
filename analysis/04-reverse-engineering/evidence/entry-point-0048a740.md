# 进程入口汇编证据：0x0048A740

状态：已从原 EXE 字节复核

来源：`swd3.exe`

SHA-256：`0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c`

PE 入口：`0x0048A740`

提取命令：

```text
objdump -d -M intel --start-address=0x48a740 --stop-address=0x48a897 swd3.exe
```

IDA 总汇编中的 `start` 被折叠，不能提供这一段的逐指令正文。本证据直接反汇编原 EXE 的 `.text` 字节，不使用伪码补全。

## 原始反汇编

```asm
0048A740  push  ebp
0048A741  mov   ebp, esp
0048A743  push  0FFFFFFFFh
0048A745  push  0049BC08h
0048A74A  push  00490924h
0048A74F  mov   eax, fs:[0]
0048A755  push  eax
0048A756  mov   fs:[0], esp
0048A75D  sub   esp, 5Ch
0048A760  push  ebx
0048A761  push  esi
0048A762  push  edi
0048A763  mov   [ebp-18h], esp
0048A766  call  dword ptr [00499194h]  ; GetVersion
0048A76C  mov   [0053D164h], eax
0048A771  mov   eax, [0053D164h]
0048A776  shr   eax, 8
0048A779  and   eax, 0FFh
0048A77E  mov   [0053D170h], eax
0048A783  mov   ecx, [0053D164h]
0048A789  and   ecx, 0FFh
0048A78F  mov   [0053D16Ch], ecx
0048A795  mov   edx, [0053D16Ch]
0048A79B  shl   edx, 8
0048A79E  add   edx, [0053D170h]
0048A7A4  mov   [0053D168h], edx
0048A7AA  mov   eax, [0053D164h]
0048A7AF  shr   eax, 10h
0048A7B2  and   eax, 0FFFFh
0048A7B7  mov   [0053D164h], eax
0048A7BC  push  0
0048A7BE  call  0048B440h              ; __heap_init
0048A7C3  add   esp, 4
0048A7C6  test  eax, eax
0048A7C8  jne   0048A7D4h
0048A7CA  push  1Ch
0048A7CC  call  0048A8D0h              ; _fast_error_exit
0048A7D1  add   esp, 4
0048A7D4  mov   [ebp-4], 0
0048A7DB  call  00491620h              ; __ioinit
0048A7E0  call  dword ptr [00499170h]  ; GetCommandLineA
0048A7E6  mov   [0053E7C4h], eax
0048A7EB  call  00491400h              ; ___crtGetEnvironmentStringsA
0048A7F0  mov   [0053D1A8h], eax
0048A7F5  call  00490EF0h              ; __setargv
0048A7FA  call  00490DA0h              ; __setenvp
0048A7FF  call  0048A330h              ; __cinit
0048A804  mov   [ebp-30h], 0
0048A80B  lea   ecx, [ebp-5Ch]
0048A80E  push  ecx
0048A80F  call  dword ptr [0049916Ch]  ; GetStartupInfoA
0048A815  call  00490CE0h              ; __wincmdln
0048A81A  mov   [ebp-64h], eax
0048A81D  mov   edx, [ebp-30h]
0048A820  and   edx, 1
0048A823  test  edx, edx
0048A825  je    0048A834h
0048A827  mov   eax, [ebp-2Ch]
0048A82A  and   eax, 0FFFFh
0048A82F  mov   [ebp-6Ch], eax
0048A832  jmp   0048A83Bh
0048A834  mov   [ebp-6Ch], 0Ah
0048A83B  mov   ecx, [ebp-6Ch]
0048A83E  push  ecx                    ; nCmdShow
0048A83F  mov   edx, [ebp-64h]
0048A842  push  edx                    ; lpCmdLine
0048A843  push  0                      ; hPrevInstance
0048A845  push  0                      ; GetModuleHandleA(NULL) 参数
0048A847  call  dword ptr [0049918Ch]  ; GetModuleHandleA
0048A84D  push  eax                    ; hInstance
0048A84E  call  00409EC0h              ; _WinMain@16
0048A853  mov   [ebp-60h], eax
0048A856  mov   eax, [ebp-60h]
0048A859  push  eax
0048A85A  call  0048A370h              ; _exit
0048A85F  mov   ecx, [ebp-14h]
0048A862  mov   edx, [ecx]
0048A864  mov   eax, [edx]
0048A866  mov   [ebp-68h], eax
0048A869  mov   ecx, [ebp-14h]
0048A86C  push  ecx
0048A86D  mov   edx, [ebp-68h]
0048A870  push  edx
0048A871  call  00490AD0h              ; __XcptFilter
0048A876  add   esp, 8
0048A879  ret
0048A87A  mov   esp, [ebp-18h]
0048A87D  mov   eax, [ebp-68h]
0048A880  push  eax
0048A881  call  0048A390h              ; __exit
0048A886  mov   ecx, [ebp-10h]
0048A889  mov   fs:[0], ecx
0048A890  pop   edi
0048A891  pop   esi
0048A892  pop   ebx
0048A893  mov   esp, ebp
0048A895  pop   ebp
0048A896  ret
```

## 导入槽复核

- `0x0049916C`：`GetStartupInfoA`
- `0x00499170`：`GetCommandLineA`
- `0x0049918C`：`GetModuleHandleA`
- `0x00499194`：`GetVersion`

这些名称同时由 PE 导入表和 IDA 导入段支持。

## 可直接成立的结论

- 进程从 `0x0048A740` 进入 Visual C++ 运行库启动代码。
- 运行库完成版本、堆、I/O、命令行、环境、参数和 C 初始化后才调用游戏 `WinMain`。
- `WinMain` 的四个参数按 Win32 约定从右向左压栈。
- `hInstance` 来自 `GetModuleHandleA(NULL)`；`hPrevInstance` 固定为零。
- `lpCmdLine` 来自 `0x00490CE0` 对完整命令行的处理结果。
- `nCmdShow` 在 `STARTF_USESHOWWINDOW` 位存在时取 `STARTUPINFOA.wShowWindow` 的低 16 位，否则使用 `10`，即 `SW_SHOWDEFAULT`。
- 游戏入口是 `0x00409EC0`；该函数末尾 `ret 10h`，与 `_WinMain@16` 四个 32 位参数一致。
- `WinMain` 返回值被传给 `0x0048A370` `_exit`，正常路径不会回到游戏逻辑。

## 仍未包含的结论

本证据只确认 CRT 入口链。窗口注册、窗口创建、消息泵、单帧更新和退出条件属于 P1.2 之后的证据块。
