set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(SWD3_LLVM_ROOT "/opt/llvm" CACHE PATH "LLVM install root")

set(CMAKE_C_COMPILER "${SWD3_LLVM_ROOT}/bin/clang")
set(CMAKE_CXX_COMPILER "${SWD3_LLVM_ROOT}/bin/clang++")
set(CMAKE_AR "${SWD3_LLVM_ROOT}/bin/llvm-ar")
set(CMAKE_RANLIB "${SWD3_LLVM_ROOT}/bin/llvm-ranlib")
set(CMAKE_C_COMPILER_TARGET x86_64-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET x86_64-linux-gnu)

set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
