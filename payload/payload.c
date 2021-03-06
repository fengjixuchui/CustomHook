#include "win32.h"
#include <delayimp.h>
#include "mzcrt/mzstr.h"
#include "mzcrt/mzlib.h"

BOOL DoHook(BOOL bHook);

static DWORD s_dwCurrentProcessId = 0;
static HANDLE s_hCurrentProcess = NULL;
static WCHAR s_szLogFileName[MAX_PATH] = L"";

typedef PVOID (WINAPI *FN_ImageDirectoryEntryToData)(PVOID, BOOLEAN, USHORT, PULONG);
static FN_ImageDirectoryEntryToData ch_fn_ImageDirectoryEntryToData = &ImageDirectoryEntryToData;

typedef BOOL (WINAPI *FN_VirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
static FN_VirtualProtect ch_fn_VirtualProtect = &VirtualProtect;

typedef BOOL (WINAPI *FN_WriteProcessMemory)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T *);
static FN_WriteProcessMemory ch_fn_WriteProcessMemory = &WriteProcessMemory;

typedef int (WINAPIV *FN_wsprintfA)(LPSTR, LPCSTR, ...);
static FN_wsprintfA ch_fn_wsprintfA = &wsprintfA;

typedef int (WINAPIV *FN_wsprintfW)(LPWSTR, LPCWSTR, ...);
static FN_wsprintfW ch_fn_wsprintfW = &wsprintfW;

typedef int (WINAPI *FN_wvsprintfA)(LPSTR, LPCSTR, va_list);
static FN_wvsprintfA ch_fn_wvsprintfA = &wvsprintfA;

typedef HANDLE (WINAPI *FN_CreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static FN_CreateFileW ch_fn_CreateFileW = &CreateFileW;

typedef DWORD (WINAPI *FN_SetFilePointer)(HANDLE, LONG, PLONG, DWORD);
static FN_SetFilePointer ch_fn_SetFilePointer = &SetFilePointer;

typedef BOOL (WINAPI *FN_WriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
static FN_WriteFile ch_fn_WriteFile = &WriteFile;

typedef BOOL (WINAPI *FN_CloseHandle)(HANDLE);
static FN_CloseHandle ch_fn_CloseHandle = &CloseHandle;

typedef HMODULE (WINAPI *FN_GetModuleHandleA)(LPCSTR);
static FN_GetModuleHandleA ch_fn_GetModuleHandleA = &GetModuleHandleA;

typedef DWORD (WINAPI *FN_GetLastError)(VOID);
static FN_GetLastError ch_fn_GetLastError = &GetLastError;

typedef VOID (WINAPI *FN_SetLastError)(DWORD);
static FN_SetLastError ch_fn_SetLastError = &SetLastError;

#define NUM 6

LPCSTR do_LPCSTR(LPCSTR str)
{
    static CHAR s_szText[NUM][1024];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    if (str == NULL)
        return "(null)";
    if (HIWORD(str) == 0)
        ch_fn_wsprintfA(psz, "#%u", LOWORD(str));
    else
        ch_fn_wsprintfA(psz, "'%s'", str);
    return psz;
}

LPCWSTR do_LPCWSTR(LPCWSTR str)
{
    static WCHAR s_szText[NUM][1024];
    static INT s_index = 0;
    WCHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    if (str == NULL)
        return L"(null)";
    if (HIWORD(str) == 0)
        ch_fn_wsprintfW (psz, L"#%u", LOWORD(str));
    else
        ch_fn_wsprintfW (psz, L"'%ls'", str);
    return psz;
}

LPCSTR do_LPCRECT(LPCRECT prc)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    if (prc == NULL)
        return "(null)";
    ch_fn_wsprintfA(psz, "(%ld, %ld, %ld, %ld)", prc->left, prc->top, prc->right, prc->bottom);
    return psz;
}

LPCSTR do_LPCRECTL(LPCRECTL prc)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    if (prc == NULL)
        return "(null)";
    ch_fn_wsprintfA(psz, "(%ld, %ld, %ld, %ld)", prc->left, prc->top, prc->right, prc->bottom);
    return psz;
}

LPCSTR do_BLENDFUNCTION(BLENDFUNCTION bf)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "(%d, %d, %d, %d)",
                bf.BlendOp, bf.BlendFlags, bf.SourceConstantAlpha, bf.AlphaFormat);
    return psz;
}

LPCSTR do_COORD(COORD c)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "(%d, %d)", c.X, c.Y);
    return psz;
}

LPCSTR do_div_t(div_t d)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "(%d, %d)", d.quot, d.rem);
    return psz;
}

LPCSTR do_ldiv_t(ldiv_t d)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "(%ld, %ld)", d.quot, d.rem);
    return psz;
}

LPCSTR do_LARGE_INTEGER(LARGE_INTEGER li)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "%I64d (0x%I64X)", li.QuadPart, li.QuadPart);
    return psz;
}

LPCSTR do_ULARGE_INTEGER(ULARGE_INTEGER uli)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "%I64u (0x%I64X)", uli.QuadPart, uli.QuadPart);
    return psz;
}

LPCSTR do_CY(CY cy)
{
    static CHAR s_szText[NUM][128];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];
    *psz = 0;

    ch_fn_wsprintfA(psz, "%I64d (0x%I64X)", cy.int64, cy.int64);
    return psz;
}

void CH_TraceV(const char *fmt, va_list va)
{
    HANDLE hFile;
    DWORD cbWritten;
    static CHAR s_szText[NUM][1024];
    static INT s_index = 0;
    CHAR *psz = s_szText[(s_index++) % NUM];

    hFile = ch_fn_CreateFileW(s_szLogFileName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    ch_fn_SetFilePointer(hFile, 0, NULL, FILE_END);
    ch_fn_wvsprintfA(psz, fmt, va);
    ch_fn_WriteFile(hFile, psz, strlen(psz), &cbWritten, NULL);
    ch_fn_CloseHandle(hFile);
}

void CH_Trace(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    CH_TraceV(fmt, va);
    va_end(va);
}

#define TRACE CH_Trace

LPVOID
CH_DoHookImportEx(HMODULE hModule, const char *module_name, const char *func_name, LPVOID fn)
{
    ULONG size;
    PIMAGE_IMPORT_DESCRIPTOR pImports;
    LPSTR mod_name;
    PIMAGE_THUNK_DATA pIAT;
    PIMAGE_THUNK_DATA pINT;
    WORD ordinal;
    PIMAGE_IMPORT_BY_NAME pName;
    DWORD dwOldProtect;
    LPVOID fnOriginal;

    if (hModule == NULL)
        return NULL;

    pImports = (PIMAGE_IMPORT_DESCRIPTOR)
        ch_fn_ImageDirectoryEntryToData(hModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);

    for (; pImports->Characteristics != 0; ++pImports)
    {
        mod_name = (LPSTR)((LPBYTE)hModule + pImports->Name);
        if (_stricmp(module_name, mod_name) != 0)
            continue;

        pIAT = (PIMAGE_THUNK_DATA)((LPBYTE)hModule + pImports->FirstThunk);
        pINT = (PIMAGE_THUNK_DATA)((LPBYTE)hModule + pImports->OriginalFirstThunk);
        for (; pINT->u1.AddressOfData != 0 && pIAT->u1.Function != 0; pIAT++, pINT++)
        {
            if (IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal))
            {
                ordinal = (WORD)IMAGE_ORDINAL(pINT->u1.Ordinal);
                if (func_name[0] == '#')
                {
                    if (atoi(&func_name[1]) != ordinal)
                        continue;
                }
                else if (HIWORD(func_name) == 0)
                {
                    if (LOWORD(func_name) != ordinal)
                        continue;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                pName = (PIMAGE_IMPORT_BY_NAME)((LPBYTE)hModule + pINT->u1.AddressOfData);
                if (_stricmp((LPSTR)pName->Name, func_name) != 0)
                    continue;
            }

            if (!ch_fn_VirtualProtect(&pIAT->u1.Function, sizeof(pIAT->u1.Function), PAGE_READWRITE, &dwOldProtect))
                return NULL;

            fnOriginal = (LPVOID)pIAT->u1.Function;
            if (fn)
            {
                ch_fn_WriteProcessMemory(s_hCurrentProcess, &pIAT->u1.Function, &fn, sizeof(pIAT->u1.Function), NULL);
                pIAT->u1.Function = (DWORD_PTR)fn;
            }

            ch_fn_VirtualProtect(&pIAT->u1.Function, sizeof(pIAT->u1.Function), dwOldProtect, &dwOldProtect);

            return fnOriginal;
        }
    }

    return NULL;
}

LPVOID
CH_DoHookDelayEx(HMODULE hModule, const char *module_name, const char *func_name, LPVOID fn)
{
    ULONG size;
    ImgDelayDescr *pDelay;
    LPSTR mod_name;
    PIMAGE_THUNK_DATA pIAT;
    PIMAGE_THUNK_DATA pINT;
    WORD ordinal;
    PIMAGE_IMPORT_BY_NAME pName;
    DWORD dwOldProtect;
    LPVOID fnOriginal;

    if (hModule == NULL)
        return NULL;

    pDelay = (ImgDelayDescr *)
        ch_fn_ImageDirectoryEntryToData(hModule, TRUE, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, &size);

    for (; pDelay->rvaDLLName != 0; ++pDelay)
    {
        mod_name = (LPSTR)((LPBYTE)hModule + pDelay->rvaDLLName);
        if (_stricmp(module_name, mod_name) != 0)
            continue;

        pIAT = (PIMAGE_THUNK_DATA)((LPBYTE)hModule + pDelay->rvaIAT);
        pINT = (PIMAGE_THUNK_DATA)((LPBYTE)hModule + pDelay->rvaINT);
        for (; pINT->u1.AddressOfData != 0 && pIAT->u1.Function != 0; pIAT++, pINT++)
        {
            if (IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal))
            {
                ordinal = (WORD)IMAGE_ORDINAL(pINT->u1.Ordinal);
                if (func_name[0] == '#')
                {
                    if (atoi(&func_name[1]) != ordinal)
                        continue;
                }
                else if (HIWORD(func_name) == 0)
                {
                    if (LOWORD(func_name) != ordinal)
                        continue;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                pName = (PIMAGE_IMPORT_BY_NAME)((LPBYTE)hModule + pINT->u1.AddressOfData);
                if (_stricmp((LPSTR)pName->Name, func_name) != 0)
                    continue;
            }

            if (!ch_fn_VirtualProtect(&pIAT->u1.Function, sizeof(pIAT->u1.Function), PAGE_READWRITE, &dwOldProtect))
                return NULL;

            fnOriginal = (LPVOID)pIAT->u1.Function;
            if (fn)
            {
                ch_fn_WriteProcessMemory(s_hCurrentProcess, &pIAT->u1.Function, &fn, sizeof(pIAT->u1.Function), NULL);
                pIAT->u1.Function = (DWORD_PTR)fn;
            }

            ch_fn_VirtualProtect(&pIAT->u1.Function, sizeof(pIAT->u1.Function), dwOldProtect, &dwOldProtect);

            return fnOriginal;
        }
    }

    return NULL;
}

LPVOID
CH_DoHookImport(const char *module_name, const char *func_name, LPVOID fn)
{
    HMODULE hModule;
    LPVOID ret;

    hModule = ch_fn_GetModuleHandleA(module_name);
    ret = CH_DoHookImportEx(hModule, module_name, func_name, fn);
    if (ret)
        return ret;

    hModule = ch_fn_GetModuleHandleA(NULL);
    ret = CH_DoHookImportEx(hModule, module_name, func_name, fn);
    return ret;
}

LPVOID
CH_DoHookDelay(const char *module_name, const char *func_name, LPVOID fn)
{
    HMODULE hModule;
    LPVOID ret;

    hModule = ch_fn_GetModuleHandleA(module_name);
    ret = CH_DoHookDelayEx(hModule, module_name, func_name, fn);
    if (ret)
        return ret;

    hModule = ch_fn_GetModuleHandleA(NULL);
    ret = CH_DoHookDelayEx(hModule, module_name, func_name, fn);
    return ret;
}

LPVOID
CH_DoHook(const char *module_name, const char *func_name, LPVOID fn)
{
    LPVOID ret;

    ret = CH_DoHookImport(module_name, func_name, fn);
    if (ret)
        return ret;

    ret = CH_DoHookDelay(module_name, func_name, fn);
    return ret;
}

BOOL CH_Init(BOOL bInit)
{
    LPVOID pv;
    if (bInit)
    {
        s_dwCurrentProcessId = GetCurrentProcessId();
        s_hCurrentProcess = GetCurrentProcess();

        GetModuleFileNameW(NULL, s_szLogFileName, ARRAYSIZE(s_szLogFileName));
        *wcsrchr(s_szLogFileName, L'\\') = 0;
        wcscat(s_szLogFileName, L"\\CustomHook.log");

        pv = CH_DoHookImport("imagehlp.dll", "ImageDirectoryEntryToData", NULL);
        if (pv)
            ch_fn_ImageDirectoryEntryToData = pv;
        pv = CH_DoHookImport("kernel32.dll", "VirtualProtect", NULL);
        if (pv)
            ch_fn_VirtualProtect = pv;
        pv = CH_DoHookImport("kernel32.dll", "WriteProcessMemory", NULL);
        if (pv)
            ch_fn_WriteProcessMemory = pv;
        pv = CH_DoHookImport("user32.dll", "wsprintfA", NULL);
        if (pv)
            ch_fn_wsprintfA = pv;
        pv = CH_DoHookImport("user32.dll", "wsprintfW", NULL);
        if (pv)
            ch_fn_wsprintfW = pv;
        pv = CH_DoHookImport("user32.dll", "wvsprintfA", NULL);
        if (pv)
            ch_fn_wvsprintfA = pv;
        pv = CH_DoHookImport("kernel32.dll", "CreateFileW", NULL);
        if (pv)
            ch_fn_CreateFileW = pv;
        pv = CH_DoHookImport("kernel32.dll", "SetFilePointer", NULL);
        if (pv)
            ch_fn_SetFilePointer = pv;
        pv = CH_DoHookImport("kernel32.dll", "WriteFile", NULL);
        if (pv)
            ch_fn_WriteFile = pv;
        pv = CH_DoHookImport("kernel32.dll", "CloseHandle", NULL);
        if (pv)
            ch_fn_CloseHandle = pv;
        pv = CH_DoHookImport("kernel32.dll", "GetModuleHandleA", NULL);
        if (pv)
            ch_fn_GetModuleHandleA = pv;
        pv = CH_DoHookImport("kernel32.dll", "GetLastError", NULL);
        if (pv)
            ch_fn_GetLastError = pv;
        pv = CH_DoHookImport("kernel32.dll", "SetLastError", NULL);
        if (pv)
            ch_fn_SetLastError = pv;
    }
    return TRUE;
}

#include "hookbody.h"

EXTERN_C BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!CH_Init(TRUE))
            return FALSE;
        if (!DoHook(TRUE))
            return FALSE;
        break;

    case DLL_PROCESS_DETACH:
        DoHook(FALSE);
        CH_Init(FALSE);
        break;
    }
    return TRUE;
}
