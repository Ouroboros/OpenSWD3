#pragma once

#include "swd3/w32.h"

namespace swd3 {

class File {
public:
    File();
    ~File();

    int Open(LPCSTR fileName, DWORD creationDisposition, DWORD desiredAccess);
    int Close();
    int CreateMapping();
    int CloseMapping();
    LPVOID MapView(DWORD offset, SIZE_T bytes);
    int UnmapView(LPCVOID view);
    DWORD GetSize();
    BOOL SetEnd();
    int Read(LPVOID buffer, DWORD* bytes);
    int Write(LPCVOID buffer, DWORD* bytes);
    DWORD Seek(DWORD moveMethod, LONG distance);
    DWORD SeekCurrent(LONG distance);
    DWORD SeekBegin(LONG distance);
    DWORD SeekEnd(LONG distance);
    int Tell(DWORD* position);
    int WriteExact(LPCVOID buffer, int bytes);
    BOOL GetTime(LPFILETIME creation, LPFILETIME lastAccess, LPFILETIME lastWrite);
    int WriteByte(DWORD value);
    int WriteDword(DWORD value);
    int ReadExact(LPVOID buffer, int bytes);
    int ReadByte(LPVOID buffer);
    int ReadDword(LPVOID buffer);
    DWORD SetError(DWORD messageId);
    char* ErrorText();

    HANDLE m_file;
    HANDLE m_mapping;
    LPVOID m_view;
    char* m_name;
    char m_error[0x40];
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(File) == 0x50);
#endif

}  // namespace swd3
