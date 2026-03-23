#pragma once

#include "FileSystemProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsFileSystemProvider
//
//  Real Windows file system implementation of IFileSystemProvider.
//
////////////////////////////////////////////////////////////////////////////////

class WindowsFileSystemProvider : public IFileSystemProvider
{
public:

    DWORD GetSystemDirectory (LPWSTR  lpBuffer,
                              UINT    uSize) override
    {
        return GetSystemDirectoryW (lpBuffer, uSize);
    }


    DWORD GetFileAttributes (LPCWSTR lpFileName) override
    {
        return GetFileAttributesW (lpFileName);
    }


    DWORD GetLastError () override
    {
        return ::GetLastError();
    }


    BOOL DeleteFile (LPCWSTR lpFileName) override
    {
        return DeleteFileW (lpFileName);
    }


    DWORD GetLongPathName (LPCWSTR lpszShortPath,
                           LPWSTR  lpszLongPath,
                           DWORD   cchBuffer) override
    {
        return GetLongPathNameW (lpszShortPath, lpszLongPath, cchBuffer);
    }
};
