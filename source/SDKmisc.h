//--------------------------------------------------------------------------------------
// File: SDKMisc.h
//
// Various helper functionality that is shared between SDK samples
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#ifndef _DXUT_OPTIONAL_SDK_MISC_H_
#define _DXUT_OPTIONAL_SDK_MISC_H_ 1

//--------------------------------------------------------------------------------------
// Tries to finds a media file by searching in common locations
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTFindDXSDKMediaFileCch(_Out_writes_(cchDest) WCHAR *strDestPath, _In_ int cchDest, _In_z_ LPCWSTR strFilename);
HRESULT WINAPI DXUTSetMediaSearchPath(_In_z_ LPCWSTR strPath);
LPCWSTR WINAPI DXUTGetMediaSearchPath();

#endif
