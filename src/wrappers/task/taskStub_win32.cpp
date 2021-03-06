/// -------------------------------------------------
/// taskStub_win32.cpp : windows操作系统任务实现文件
/// -------------------------------------------------
/// Copyright (c) 2015, Wang Yujia <combo.xy@163.com>
/// All rights reserved.
/// -------------------------------------------------

#include "dcop.h"

#if DCOP_OS == DCOP_OS_WINDOWS

/// -------------------------------------------------
/// 实现任务接口桩
/// -------------------------------------------------
#include "taskApi.h"
#include <windows.h>

/// -------------------------------------------------
/// 任务优先级
/// -------------------------------------------------
static int aiTaskPrio[] = 
{
    THREAD_PRIORITY_LOWEST,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST
};

/*******************************************************
  函 数 名: STUB_TaskCreate
  描    述: 创建任务桩
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
DWORD STUB_TaskCreate(OSHANDLE *pHandle,
            const char *cszName,
            DWORD *pdwID,
            void (*pEntry)(void *pPara),
            DWORD dwStackSize,
            DWORD dwPriority,
            void *pPara)
{
    if (!pHandle)
    {
        return ERRCODE_TASK_WRONG_HANDLE;
    }

    if (dwPriority >= (sizeof(aiTaskPrio)/sizeof(int)))
    {
        dwPriority = ((sizeof(aiTaskPrio)/sizeof(int))/2);
    }

    DWORD dwThId;
    *pHandle = (OSHANDLE)CreateThread(
                    NULL,
                    dwStackSize,
                    (LPTHREAD_START_ROUTINE)pEntry,
                    LPVOID(pPara),
                    0,
                    &dwThId
                    );

    if (*pHandle)
    {
        ERROR_CHECK(SetThreadPriority((HANDLE)(*pHandle), 
                                aiTaskPrio[dwPriority]));

        if (pdwID)
            *pdwID = dwThId;

        return SUCCESS;
    }

ERROR_LABEL:

    DWORD dwRc = GetLastError();
    if (!dwRc)
    {
        dwRc = ERRCODE_TASK_CREATE_FAIL;
    }

    if (*pHandle)
    {
        DWORD dwExitCode;
        if (GetExitCodeThread((HANDLE)(*pHandle), &dwExitCode))
        {
            (void)TerminateThread((HANDLE)(*pHandle), dwExitCode);
        }

        *pHandle = 0;
    }

    return dwRc;
}

/*******************************************************
  函 数 名: STUB_TaskDestroy
  描    述: 删除任务桩
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
DWORD STUB_TaskDestroy(OSHANDLE Handle, 
        const char *cszName,
        DWORD dwID)
{
    if (GetCurrentThreadId() == dwID)
    {
        /// 当前任务终止自己(见下面的注解，自己终止时，让函数直接返回是最好的，所以把原来ExitThread(0)去掉)
        /// ExitThread(0);

        /// [注]终止线程的运行，可以使用以下方法： 
        /// 1、线程函数返回（最好使用该方法）。 
        /// 2、通过调用ExitThread函数，线程将自行撤消（最好不使用该方法）。 
        /// 3、同一个进程或另一个进程中的线程调用TerminateThread函数（应避免使用该方法）。 
        /// 4、ExitProcess和TerminateProcess函数也可以用来终止线程的运行（应避免使用该方法）。
    }
    else
    {
        DWORD dwExitCode;
        ERROR_CHECK(GetExitCodeThread((HANDLE)Handle, &dwExitCode));
        ERROR_CHECK(TerminateThread((HANDLE)Handle, dwExitCode));
    }

    return SUCCESS;

ERROR_LABEL:

    DWORD dwRc = GetLastError();
    if (!dwRc)
    {
        dwRc = ERRCODE_TASK_DESTROY_FAIL;
    }

    return dwRc;
}

/*******************************************************
  函 数 名: STUB_TaskDelay
  描    述: 任务延时桩
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void STUB_TaskDelay(DWORD dwMilliseconds)
{
    Sleep(dwMilliseconds);
}

/*******************************************************
  函 数 名: STUB_TaskCurrent
  描    述: 获取当前任务桩
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
DWORD STUB_TaskCurrent()
{
    return GetCurrentThreadId();
}

/*******************************************************
  函 数 名: vRegOsTaskStubFunc
  描    述: 注册任务桩
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void vRegOsTaskStubFunc()
{
    TaskFuncs funcs = 
    {
        STUB_TaskCreate,
        STUB_TaskDestroy,
        STUB_TaskDelay,
        STUB_TaskCurrent
    };

    vSetTaskFuncs(&funcs);
}

/*******************************************************
  函 数 名: CPPBUILDUNIT_AUTO
  描    述: 自动安装任务桩
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CPPBUILDUNIT_AUTO(vRegOsTaskStubFunc, 0);


/// -------------------------------------------------
/// 实现原子操作
/// -------------------------------------------------
#include "task.h"

/*******************************************************
  函 数 名: objAtomic::Inc
  描    述: 原子递增
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objAtomic::T objAtomic::Inc(T &val)
{
    return (T)InterlockedIncrement((LPLONG)&val);
}

/*******************************************************
  函 数 名: objAtomic::Dec
  描    述: 原子递减
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objAtomic::T objAtomic::Dec(T &val)
{
    return (T)InterlockedDecrement((LPLONG)&val);
}

/*******************************************************
  函 数 名: objAtomic::Add
  描    述: 原子加
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objAtomic::T objAtomic::Add(T &val, T add)
{
    return (T)InterlockedExchangeAdd((LPLONG)&val, (LONG)add);
}

/*******************************************************
  函 数 名: objAtomic::Sub
  描    述: 原子减
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objAtomic::T objAtomic::Sub(T &val, T sub)
{
    return (T)InterlockedExchangeAdd((LPLONG)&val, 0 - (LONG)sub);
}

/*******************************************************
  函 数 名: objAtomic::Set
  描    述: 原子设置
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objAtomic::T objAtomic::Set(T &val, T set)
{
    return (T)InterlockedExchange((LPLONG)&val, (LONG)set);
}

/*******************************************************
  函 数 名: objAtomic::CAS
  描    述: 原子比较交换锁
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
bool objAtomic::CAS(T &val, T cmp, T swap)
{
    return (InterlockedCompareExchange((LPLONG)&val, (LONG)swap, (LONG)cmp) == (LONG)cmp)? true : false;
}


/// -------------------------------------------------
/// 实现随机数操作
/// -------------------------------------------------
#include <wincrypt.h>

/// -------------------------------------------------
/// windows Crypt真随机数
/// -------------------------------------------------
class CCryptRandom : public objRandom
{
public:
    static const DWORD TRY_COUNT = 16;

public:
    CCryptRandom();
    ~CCryptRandom();

    void Gen(void *pBuf, DWORD dwLen);

private:
    void Rand(void *pBuf, DWORD dwLen);

private:
    HCRYPTPROV m_hCryptProv;
};

/*******************************************************
  函 数 名: CCryptRandom::CCryptRandom
  描    述: CCryptRandom构造
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CCryptRandom::CCryptRandom()
{
    m_hCryptProv = NULL;
    CryptAcquireContext(&m_hCryptProv, NULL, NULL, PROV_RSA_FULL, 0);
    srand((unsigned)time(NULL));
}

/*******************************************************
  函 数 名: CCryptRandom::~CCryptRandom
  描    述: CCryptRandom析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CCryptRandom::~CCryptRandom()
{
    if (m_hCryptProv) CryptReleaseContext(m_hCryptProv, 0);
}

/*******************************************************
  函 数 名: CCryptRandom::Gen
  描    述: 获取随机数
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void CCryptRandom::Gen(void *pBuf, DWORD dwLen)
{
    if (!pBuf || !dwLen)
    {
        return;
    }

    DWORD dwRc = FAILURE;

    if (m_hCryptProv)
    {
        DWORD dwTryTimes = 0;
        while ((dwTryTimes < TRY_COUNT) && (dwRc != SUCCESS))
        {
            BOOL bRet = CryptGenRandom(m_hCryptProv, dwLen, (BYTE *)pBuf);
            dwRc = (bRet)? SUCCESS : FAILURE;
        }
    }

    if (dwRc != SUCCESS)
    {
        Rand(pBuf, dwLen);
    }
}

/*******************************************************
  函 数 名: CCryptRandom::Rand
  描    述: 模拟随机数
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void CCryptRandom::Rand(void *pBuf, DWORD dwLen)
{
    DWORD dwReadLen = 0;

    while (dwReadLen < dwLen)
    {
        *((BYTE *)pBuf + dwReadLen) = (BYTE)(rand() % 256);
        dwReadLen++;
    }
}

/*******************************************************
  函 数 名: objRandom::CreateInstance
  描    述: 创建随机数实例
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objRandom *objRandom::CreateInstance(const char *file, int line)
{
    #undef new
    return new (file, line) CCryptRandom();
    #define new new(__FILE__, __LINE__)
}

/*******************************************************
  函 数 名: objRandom::~objRandom
  描    述: 基类析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objRandom::~objRandom()
{
}


/// -------------------------------------------------
/// 实现获取调用栈操作
/// -------------------------------------------------
#include <dbghelp.h>
#pragma comment( lib, "dbghelp.lib" )

/// -------------------------------------------------
/// Architecture-specific definitions for x86 and x64
/// -------------------------------------------------
#if defined(_M_IX86)
#define SIZEOFPTR 4
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_I386
#define AXREG eax
#define BPREG ebp
#elif defined(_M_X64)
#define SIZEOFPTR 8
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_AMD64
#define AXREG rax
#define BPREG rbp
#endif // #if defined(_M_IX86)

#define INVALID_FP_RET_ADDR_VALUE 0x00000000

BOOL g_fSymInit = FALSE;
HANDLE g_hProcess = NULL;

/*******************************************************
  函 数 名: DisplaySymbolDetails
  描    述: address of the founction stack-call to walk.
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void DisplaySymbolDetails(DWORD_PTR dwAddress, LOG_PRINT logPrint, LOG_PARA logPara)
{
    DWORD64 displacement = 0;

    ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME*sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    if (!SymFromAddr(g_hProcess, dwAddress, &displacement, pSymbol))
    {
        logPrint(STR_FORMAT("-<Unable to get symbol details_%d>", GetLastError()), logPara);
        return;
    }

    /// Try to get the Module details
    IMAGEHLP_MODULE64 moduleinfo;
    moduleinfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    if (SymGetModuleInfo64(g_hProcess, pSymbol->Address, &moduleinfo))
    {
        logPrint(STR_FORMAT("%s!", moduleinfo.ModuleName), logPara);
    }
    else
    {
        logPrint(STR_FORMAT("<ErrorModuleInfo_%d>!", GetLastError()), logPara);
    }

    /// now print the function name
    if (pSymbol->MaxNameLen > 0)
    {
        logPrint(STR_FORMAT("%s", pSymbol->Name), logPara);
    }
    else
    {
        logPrint("<Unknown_Function>", logPara);
    }
}

/*******************************************************
  函 数 名: WalkTheStack
  描    述: 采用内联汇编获取当前stack Frame地址和当前程序指令地址
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
bool WalkTheStack(LOG_PRINT logPrint, LOG_PARA logPara, DWORD dwDepth)
{
    DWORD_PTR framepointer = INVALID_FP_RET_ADDR_VALUE;
    DWORD_PTR dwIPOfCurrentFunction = (DWORD_PTR)&WalkTheStack;

    /// Get the current Frame pointer
#if defined(_M_IX86) || defined(_M_X64)
    __asm
    {
        mov [framepointer], BPREG
    }
#else
    #error "AllocHook is not supported on this architecture."
#endif // defined(_M_IX86) || defined(_M_X64)

    /// We cannot walk the stack (yet!) without a frame pointer
    if (framepointer == INVALID_FP_RET_ADDR_VALUE) return false;

    logPrint("CurFP\t\tRetAddr \r\n", logPara);

    /// current Frame Pointer
    DWORD_PTR *pCurFP = (DWORD_PTR *)framepointer;
    DWORD dwFPCount = 0;
    while (pCurFP != INVALID_FP_RET_ADDR_VALUE)
    {
        /// pointer arithmetic works in terms of type pointed to. Thus,
        /// "+1" below is equivalent of 4 bytes since we are doing DWORD
        /// math.
        /// Find Caller,next print.
        DWORD_PTR pRetAddrInCaller = (*((DWORD_PTR *)(pCurFP + 1)));

        if (dwFPCount > 1)
        {
            logPrint(STR_FORMAT("%p\t%p  ", pCurFP, (DWORD_PTR *)pRetAddrInCaller), logPara);
            if (g_fSymInit) DisplaySymbolDetails(dwIPOfCurrentFunction, logPrint, logPara);
            logPrint("\r\n", logPara);
        }

        dwFPCount++;
        if (dwDepth && (dwFPCount > (dwDepth + 1)))
        {
            break;
        }

        if (pRetAddrInCaller == INVALID_FP_RET_ADDR_VALUE)
        {
            /// StackWalk is over now...
            break;
        }

        /// To get the name of the next function up the stack,
        /// we use the return address of the current frame
        dwIPOfCurrentFunction = pRetAddrInCaller;

        /// move up the stack to our caller
        DWORD_PTR pCallerFP = *((DWORD_PTR *)pCurFP);
        pCurFP = (DWORD_PTR *)pCallerFP;
    }

    return true;
}

/*******************************************************
  函 数 名: ShowCallStack
  描    述: 显示调用栈
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void ShowCallStack(LOG_PRINT print, LOG_PARA para, int depth)
{
    if (!print)
    {
        print = PrintToConsole;
        para = 0;
    }

    if (!g_fSymInit)
    {
        /// Initialize the debugger services to retrieve detailed stack info
        g_hProcess = GetCurrentProcess();
        if (!SymInitialize(g_hProcess, NULL, TRUE))
        {
            print("Unable to initialize symbols! \r\n", para);
        }
        else
        {
            /// SYMOPT_UNDNAME:All symbols are presented in undecorated form.
            /// SYMOPT_INCLUDE_32BIT_MODULES:
            /// When debugging on 64-bit Windows, include any 32-bit modules.
            /// SYMOPT_ALLOW_ABSOLUTE_SYMBOLS:
            /// Enables the use of symbols that are stored with absolute addresses. instead of RAVS forms.
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);
            g_fSymInit = TRUE;
        }
    }

    if (WalkTheStack(print, para, (DWORD)depth) == false)
    {
        print("Stackwalk failed! \r\n", para);
    }
}


/// -------------------------------------------------
/// 动态库加载操作
/// -------------------------------------------------

/// -------------------------------------------------
/// windows 动态库
/// -------------------------------------------------
class CDynamicLibrary : public objDynamicLoader
{
public:
    CDynamicLibrary();
    ~CDynamicLibrary();

    DWORD Load(const char *dllFile);
    DWORD Unload();

    void *FindSymbol(const char *symName);

    void SetErrLog(LOG_PRINT logPrint, LOG_PARA logPara);

private:
    HMODULE m_handle;
    LOG_PRINT m_logPrint;
    LOG_PARA m_logPara;
};

/*******************************************************
  函 数 名: CDynamicLibrary::CDynamicLibrary
  描    述: CDynamicLibrary构造
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CDynamicLibrary::CDynamicLibrary()
{
    m_handle = NULL;
    m_logPrint = PrintToConsole;
    m_logPara = 0;
}

/*******************************************************
  函 数 名: CDynamicLibrary::~CDynamicLibrary
  描    述: CDynamicLibrary析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CDynamicLibrary::~CDynamicLibrary()
{
    (void)Unload();
}

/*******************************************************
  函 数 名: CDynamicLibrary::Load
  描    述: 加载
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
DWORD CDynamicLibrary::Load(const char *dllFile)
{
    if (!dllFile || !(*dllFile))
    {
        return FAILURE;
    }

    if (m_handle)
    {
        return FAILURE;
    }

    HMODULE hModule = LoadLibraryA(dllFile);
    if (!hModule)
    {
        PrintLog(STR_FORMAT("LoadLibrary Fail(%d)!", GetLastError()), m_logPrint, m_logPara);
        return FAILURE;
    }

    m_handle = hModule;
    return SUCCESS;
}

/*******************************************************
  函 数 名: CDynamicLibrary::Unload
  描    述: 卸载
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
DWORD CDynamicLibrary::Unload()
{
    if (!m_handle)
    {
        return FAILURE;
    }

    BOOL bRc = FreeLibrary(m_handle);
    if (bRc != TRUE)
    {
        PrintLog(STR_FORMAT("FreeLibrary Fail(%d)!", GetLastError()), m_logPrint, m_logPara);
        return FAILURE;
    }

    m_handle = NULL;
    return SUCCESS;
}

/*******************************************************
  函 数 名: CDynamicLibrary::~CDynamicLibrary
  描    述: CDynamicLibrary析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void *CDynamicLibrary::FindSymbol(const char *symName)
{
    if (!symName || !(*symName))
    {
        return NULL;
    }

    if (!m_handle)
    {
        return NULL;
    }

    void *pSymAddr = (void *)GetProcAddress(m_handle, symName);
    if (!pSymAddr)
    {
        PrintLog(STR_FORMAT("GetProcAddress(%s) Fail(%d)!", symName, GetLastError()), m_logPrint, m_logPara);
        return NULL;
    }

    return pSymAddr;
}

/*******************************************************
  函 数 名: CDynamicLibrary::~CDynamicLibrary
  描    述: CDynamicLibrary析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void CDynamicLibrary::SetErrLog(LOG_PRINT logPrint, LOG_PARA logPara)
{
    m_logPrint = (logPrint)? logPrint : PrintToConsole;
    m_logPara = (logPrint)? logPara : 0;
}

/*******************************************************
  函 数 名: objDynamicLoader::CreateInstance
  描    述: 创建动态加载实例
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objDynamicLoader *objDynamicLoader::CreateInstance(const char *file, int line)
{
    #undef new
    return new (file, line) CDynamicLibrary();
    #define new new(__FILE__, __LINE__)
}

/*******************************************************
  函 数 名: objDynamicLoader::~objDynamicLoader
  描    述: 基类析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objDynamicLoader::~objDynamicLoader()
{
}


#endif // #if DCOP_OS == DCOP_OS_WINDOWS

