//--------------------------------------------------------------------------------------
// File: SDKmisc.cpp
//
// Various helper functionality that is shared between SDK samples
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#ifndef UNICODE
#error "DXUT requires a Unicode build."
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <sdkddkver.h>
#include <Windows.h>
#include <cstdio>
#include "SDKmisc.h"

//--------------------------------------------------------------------------------------
// Error codes
//--------------------------------------------------------------------------------------
#define DXUTERR_MEDIANOTFOUND MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0903)

//--------------------------------------------------------------------------------------
// Internal functions forward declarations
//--------------------------------------------------------------------------------------
bool DXUTFindMediaSearchTypicalDirs(_Out_writes_(cchSearch) WCHAR *strSearchPath, _In_ int cchSearch, _In_ LPCWSTR strLeaf, _In_ const WCHAR *strExePath, _In_ const WCHAR *strExeName);
bool DXUTFindMediaSearchParentDirs(_Out_writes_(cchSearch) WCHAR *strSearchPath, _In_ int cchSearch, _In_ const WCHAR *strStartAt, _In_ const WCHAR *strLeafName);

//--------------------------------------------------------------------------------------
// Returns pointer to static media search buffer
//--------------------------------------------------------------------------------------
WCHAR *DXUTMediaSearchPath()
{
    static WCHAR s_strMediaSearchPath[MAX_PATH] =
        {
            0};
    return s_strMediaSearchPath;
}

//--------------------------------------------------------------------------------------
LPCWSTR WINAPI DXUTGetMediaSearchPath()
{
    return DXUTMediaSearchPath();
}

//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTSetMediaSearchPath(_In_z_ LPCWSTR strPath)
{
    HRESULT hr;

    WCHAR *s_strSearchPath = DXUTMediaSearchPath();

    hr = wcscpy_s(s_strSearchPath, MAX_PATH, strPath);
    if (SUCCEEDED(hr))
    {
        // append slash if needed
        size_t ch = 0;
        ch = wcsnlen(s_strSearchPath, MAX_PATH);
        if (SUCCEEDED(hr) && s_strSearchPath[ch - 1] != L'\\')
        {
            hr = wcscat_s(s_strSearchPath, MAX_PATH, L"\\");
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Tries to find the location of a SDK media file
//       cchDest is the size in WCHARs of strDestPath.  Be careful not to
//       pass in sizeof(strDest) on UNICODE builds.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_ HRESULT WINAPI DXUTFindDXSDKMediaFileCch(WCHAR *strDestPath, int cchDest, LPCWSTR strFilename)
{
    bool bFound;
    WCHAR strSearchFor[MAX_PATH];

    if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] = {0};
    WCHAR strExeName[MAX_PATH] = {0};
    WCHAR *strLastSlash = nullptr;
    GetModuleFileNameW(nullptr, strExePath, MAX_PATH);
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr(strExePath, TEXT('\\'));
    if (strLastSlash)
    {
        wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr(strExeName, TEXT('.'));
        if (strLastSlash)
            *strLastSlash = 0;
    }

    // Typical directories:
    //      .\
    //      ..\
    //      ..\..\
    //      %EXE_DIR%\
    //      %EXE_DIR%\..\
    //      %EXE_DIR%\..\..\
    //      %EXE_DIR%\..\%EXE_NAME%
    //      %EXE_DIR%\..\..\%EXE_NAME%

    // Typical directory search
    bFound = DXUTFindMediaSearchTypicalDirs(strDestPath, cchDest, strFilename, strExePath, strExeName);
    if (bFound)
        return S_OK;

    // Typical directory search again, but also look in a subdir called "\media\"
    swprintf_s(strSearchFor, MAX_PATH, L"media\\%ls", strFilename);
    bFound = DXUTFindMediaSearchTypicalDirs(strDestPath, cchDest, strSearchFor, strExePath, strExeName);
    if (bFound)
        return S_OK;

    WCHAR strLeafName[MAX_PATH] =
        {
            0};

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    wcscpy_s(strLeafName, MAX_PATH, strFilename);
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, L".", strLeafName);
    if (bFound)
        return S_OK;

    // Search all parent directories starting at the exe's dir and using strFilename as the leaf name
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, strExePath, strLeafName);
    if (bFound)
        return S_OK;

    // Search all parent directories starting at .\ and using "media\strFilename" as the leaf name
    swprintf_s(strLeafName, MAX_PATH, L"media\\%ls", strFilename);
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, L".", strLeafName);
    if (bFound)
        return S_OK;

    // Search all parent directories starting at the exe's dir and using "media\strFilename" as the leaf name
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, strExePath, strLeafName);
    if (bFound)
        return S_OK;

    // On failure, return the file as the path but also return an error code
    wcscpy_s(strDestPath, cchDest, strFilename);

    return DXUTERR_MEDIANOTFOUND;
}

//--------------------------------------------------------------------------------------
// Search a set of typical directories
//--------------------------------------------------------------------------------------
_Use_decl_annotations_ bool DXUTFindMediaSearchTypicalDirs(WCHAR *strSearchPath, int cchSearch, LPCWSTR strLeaf, const WCHAR *strExePath, const WCHAR *strExeName)
{
    // Typical directories:
    //      .\
    //      ..\
    //      ..\..\
    //      %EXE_DIR%\
    //      %EXE_DIR%\..\
    //      %EXE_DIR%\..\..\
    //      %EXE_DIR%\..\%EXE_NAME%
    //      %EXE_DIR%\..\..\%EXE_NAME%
    //      DXSDK media path

    // Search in ".\"
    wcscpy_s(strSearchPath, cchSearch, strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "..\"
    swprintf_s(strSearchPath, cchSearch, L"..\\%ls", strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "..\..\"
    swprintf_s(strSearchPath, cchSearch, L"..\\..\\%ls", strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "..\..\"
    swprintf_s(strSearchPath, cchSearch, L"..\\..\\%ls", strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in the "%EXE_DIR%\"
    swprintf_s(strSearchPath, cchSearch, L"%ls\\%ls", strExePath, strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in the "%EXE_DIR%\..\"
    swprintf_s(strSearchPath, cchSearch, L"%ls\\..\\%ls", strExePath, strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in the "%EXE_DIR%\..\..\"
    swprintf_s(strSearchPath, cchSearch, L"%ls\\..\\..\\%ls", strExePath, strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "%EXE_DIR%\..\%EXE_NAME%\".  This matches the DirectX SDK layout
    swprintf_s(strSearchPath, cchSearch, L"%ls\\..\\%ls\\%ls", strExePath, strExeName, strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "%EXE_DIR%\..\..\%EXE_NAME%\".  This matches the DirectX SDK layout
    swprintf_s(strSearchPath, cchSearch, L"%ls\\..\\..\\%ls\\%ls", strExePath, strExeName, strLeaf);
    if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in media search dir
    WCHAR *s_strSearchPath = DXUTMediaSearchPath();
    if (s_strSearchPath[0] != 0)
    {
        swprintf_s(strSearchPath, cchSearch, L"%ls%ls", s_strSearchPath, strLeaf);
        if (GetFileAttributesW(strSearchPath) != 0xFFFFFFFF)
            return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------
// Search parent directories starting at strStartAt, and appending strLeafName
// at each parent directory.  It stops at the root directory.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_ bool DXUTFindMediaSearchParentDirs(WCHAR *strSearchPath, int cchSearch, const WCHAR *strStartAt, const WCHAR *strLeafName)
{
    WCHAR strFullPath[MAX_PATH] = {0};
    WCHAR strFullFileName[MAX_PATH] = {0};
    WCHAR strSearch[MAX_PATH] = {0};
    WCHAR *strFilePart = nullptr;

    if (!GetFullPathNameW(strStartAt, MAX_PATH, strFullPath, &strFilePart))
        return false;

    while (strFilePart && *strFilePart != '\0')
    {
        swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls", strFullPath, strLeafName);
        if (GetFileAttributesW(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strSearchPath, cchSearch, strFullFileName);
            return true;
        }

        swprintf_s(strSearch, MAX_PATH, L"%ls\\..", strFullPath);
        if (!GetFullPathNameW(strSearch, MAX_PATH, strFullPath, &strFilePart))
            return false;
    }

    return false;
}
