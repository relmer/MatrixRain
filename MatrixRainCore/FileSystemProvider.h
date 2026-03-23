#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  IFileSystemProvider
//
//  Abstraction for file system operations to enable mock-based testing.
//  Production code uses the real Windows file system; tests inject a mock.
//
////////////////////////////////////////////////////////////////////////////////

class IFileSystemProvider
{
public:
    virtual ~IFileSystemProvider() = default;

    virtual DWORD GetSystemDirectory (LPWSTR  lpBuffer,
                                      UINT    uSize) = 0;

    virtual DWORD GetFileAttributes  (LPCWSTR lpFileName) = 0;

    virtual DWORD GetLastError       () = 0;

    virtual BOOL  DeleteFile         (LPCWSTR lpFileName) = 0;

    virtual DWORD GetLongPathName    (LPCWSTR lpszShortPath,
                                      LPWSTR  lpszLongPath,
                                      DWORD   cchBuffer) = 0;
};
