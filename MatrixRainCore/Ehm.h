#pragma once





#define ErrorLabel          Error

#define __EHM_ASSERT        TRUE
#define __EHM_NO_ASSERT     FALSE





#define WIDEN2(x)       L ## x
#define WIDEN(x)        WIDEN2(x)
#define __WFUNCTION__   WIDEN(__FUNCTION__)
#define __WFILE__       WIDEN(__FILE__)





//
// Forward declarations
//

void DEBUGMSG   (LPCWSTR pszFormat, ...);
void RELEASEMSG (LPCWSTR pszFormat, ...);





typedef void (*EHM_BREAKPOINT_FUNC)(void);

extern EHM_BREAKPOINT_FUNC g_pfnBreakpoint;

void SetBreakpointFunction (EHM_BREAKPOINT_FUNC func);
void EhmBreakpoint (void);





#if DBG || DEBUG || _DEBUG
    #define EHM_BREAKPOINT EhmBreakpoint()
#else
    #define EHM_BREAKPOINT
#endif





#define ASSERT(__condition)                                             \
    if (!(__condition))                                                 \
    {                                                                   \
        DEBUGMSG ((L"%s(%d) - %s - Assertion failed:  %s\n"),           \
                    __WFILE__, __LINE__, __WFUNCTION__, L#__condition); \
        EHM_BREAKPOINT;                                                 \
    }                                                                                                               





//
// Core helper macros
//

#define __CHRAExHelper(__arg_hrTest, __arg_fAssert, __arg_fReplaceHr, __arg_hrReplaceHr, __arg_pszLog)    \
{                                                                                           \
    HRESULT __hr = __arg_hrTest;                                                            \
    LPCWSTR pszLog = __arg_pszLog;                                                          \
                                                                                            \
    if (FAILED (__hr))                                                                      \
    {                                                                                       \
        if (pszLog != nullptr)                                                              \
        {                                                                                   \
            DEBUGMSG ((L"%s(%d) - %s - %s\n"),                                              \
                        __WFILE__, __LINE__, __WFUNCTION__, pszLog);                        \
        }                                                                                   \
                                                                                            \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            ASSERT (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        if (__arg_fReplaceHr)                                                               \
        {                                                                                   \
            hr = __arg_hrReplaceHr;                                                         \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            hr = __hr;                                                                      \
        }                                                                                   \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}                             

#define __CWRAExHelper(__arg_fSuccess, __arg_fAssert, __arg_fReplaceHr, __arg_hrReplaceHr, __arg_pszLog)  \
{                                                                                           \
    LPCWSTR pszLog = __arg_pszLog;                                                          \
                                                                                            \
    if (!(__arg_fSuccess))                                                                  \
    {                                                                                       \
        if (pszLog != nullptr)                                                              \
        {                                                                                   \
            DEBUGMSG ((L"%s(%d) - %s - %s\n"),                                              \
                        __WFILE__, __LINE__, __WFUNCTION__, pszLog);                        \
        }                                                                                   \
                                                                                            \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            ASSERT (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        if (__arg_fReplaceHr)                                                               \
        {                                                                                   \
            hr = __arg_hrReplaceHr;                                                         \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            hr = HRESULT_FROM_WIN32 (::GetLastError());                                     \
        }                                                                                   \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}  

#define __CBRAExHelper(__arg_brTest, __arg_fAssert, __arg_hrReplaceHr, __arg_pszLog)                      \
{                                                                                           \
    BOOL __br = __arg_brTest;                                                               \
    LPCWSTR pszLog = __arg_pszLog;                                                          \
                                                                                            \
    if (!(__br))                                                                            \
    {                                                                                       \
        if (pszLog != nullptr)                                                              \
        {                                                                                   \
            DEBUGMSG ((L"%s(%d) - %s - %s\n"),                                              \
                        __WFILE__, __LINE__, __WFUNCTION__, pszLog);                        \
        }                                                                                   \
                                                                                            \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            ASSERT (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        hr = __arg_hrReplaceHr;                                                             \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}                                                                                   

#define __CPRAExHelper(__arg_prTest, __arg_fAssert, __arg_hrReplaceHr, __arg_pszLog)                      \
{                                                                                           \
    /*void * __pr = __arg_prTest;*/                                                             \
    LPCWSTR pszLog = __arg_pszLog;                                                          \
                                                                                            \
    if (__arg_prTest == nullptr)                                                            \
    {                                                                                       \
        if (pszLog != nullptr)                                                              \
        {                                                                                   \
            DEBUGMSG ((L"%s(%d) - %s - %s\n"),                                              \
                        __WFILE__, __LINE__, __WFUNCTION__, pszLog);                        \
        }                                                                                   \
                                                                                            \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            ASSERT (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        hr = __arg_hrReplaceHr;                                                             \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}





//
// IGNORE_RETURN_VALUE
//

#define IGNORE_RETURN_VALUE(__result, __new_value)                                          \
{                                                                                           \
    __result = __new_value;                                                                 \
}
    




//
// CHR variants
//

#define CHRAEx(__arg_hrTest, __arg_hrReplaceHr)                                             \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_ASSERT, TRUE, __arg_hrReplaceHr, nullptr)           \
}

#define CHREx(__arg_hrTest, __arg_hrReplaceHr)                                              \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_NO_ASSERT, TRUE, __arg_hrReplaceHr, nullptr)        \
}                                                                                           \

#define CHRA(__arg_hrTest)                                                                  \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_ASSERT, FALSE, 0, nullptr)                          \
}                                                                                   

#define CHR(__arg_hrTest)                                                                   \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_NO_ASSERT, FALSE, 0, nullptr)                       \
}

#define CHRAExL(__arg_hrTest, __arg_hrReplaceHr, __arg_pszLog)                              \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_ASSERT, TRUE, __arg_hrReplaceHr, __arg_pszLog)      \
}

#define CHRExL(__arg_hrTest, __arg_hrReplaceHr, __arg_pszLog)                               \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_NO_ASSERT, TRUE, __arg_hrReplaceHr, __arg_pszLog)   \
}

#define CHRLA(__arg_hrTest, __arg_pszLog)                                                   \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_ASSERT, FALSE, 0, __arg_pszLog)                     \
}

#define CHRL(__arg_hrTest, __arg_pszLog)                                                    \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_NO_ASSERT, FALSE, 0, __arg_pszLog)                  \
}                                                                                           





//
// CWR variants
//

#define CWRAEx(__arg_fSuccess, __arg_hrReplaceHr)                                           \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_ASSERT, TRUE, __arg_hrReplaceHr, nullptr)         \
}

#define CWREx(__arg_fSuccess, __arg_hrReplaceHr)                                            \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_NO_ASSERT, TRUE, __arg_hrReplaceHr, nullptr)      \
}                                                                                           \

#define CWRA(__arg_fSuccess)                                                                \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_ASSERT, FALSE, 0, nullptr)                        \
}                                                                                   

#define CWR(__arg_fSuccess)                                                                 \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_NO_ASSERT, FALSE, 0, nullptr)                     \
}

#define CWRAExL(__arg_fSuccess, __arg_hrReplaceHr, __arg_pszLog)                            \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_ASSERT, TRUE, __arg_hrReplaceHr, __arg_pszLog)    \
}

#define CWRExL(__arg_fSuccess, __arg_hrReplaceHr, __arg_pszLog)                             \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_NO_ASSERT, TRUE, __arg_hrReplaceHr, __arg_pszLog) \
}

#define CWRLA(__arg_fSuccess, __arg_pszLog)                                                 \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_ASSERT, FALSE, 0, __arg_pszLog)                   \
}

#define CWRL(__arg_fSuccess, __arg_pszLog)                                                  \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_NO_ASSERT, FALSE, 0, __arg_pszLog)                \
}                                                                                           





//
// CBR variants
//

#define CBRAEx(__arg_brTest, __arg_hrReplaceHr)                                             \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_ASSERT, __arg_hrReplaceHr, nullptr)                 \
}                                                                                   


#define CBREx(__arg_brTest, __arg_hrReplaceHr)                                              \
{                                                                                           \
    __CBRAExHelper(__arg_brTest, __EHM_NO_ASSERT, __arg_hrReplaceHr, nullptr);              \
}                                                                                           

#define CBRA(__arg_brTest)                                                                  \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_ASSERT, E_FAIL, nullptr)                            \
}                                                                                           

#define CBR(__arg_brTest)                                                                   \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_NO_ASSERT, E_FAIL, nullptr)                         \
}                                                                                   

#define BAIL_OUT_IF(__arg_boolTest, __arg_hrReplaceHr)                                      \
{                                                                                           \
    __CBRAExHelper (!(__arg_boolTest), __EHM_NO_ASSERT, __arg_hrReplaceHr, nullptr)         \
}

#define CBRAExL(__arg_brTest, __arg_hrReplaceHr, __arg_pszLog)                              \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_ASSERT, __arg_hrReplaceHr, __arg_pszLog)            \
}

#define CBRExL(__arg_brTest, __arg_hrReplaceHr, __arg_pszLog)                               \
{                                                                                           \
    __CBRAExHelper(__arg_brTest, __EHM_NO_ASSERT, __arg_hrReplaceHr, __arg_pszLog);         \
}

#define CBRLA(__arg_brTest, __arg_pszLog)                                                   \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_ASSERT, E_FAIL, __arg_pszLog)                       \
}

#define CBRL(__arg_brTest, __arg_pszLog)                                                    \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_NO_ASSERT, E_FAIL, __arg_pszLog)                    \
}

#define BAIL_OUT_IF_L(__arg_boolTest, __arg_hrReplaceHr, __arg_pszLog)                      \
{                                                                                           \
    __CBRAExHelper (!(__arg_boolTest), __EHM_NO_ASSERT, __arg_hrReplaceHr, __arg_pszLog)    \
}                                                                                           





//
// CPR variants
// 

#define CPRAEx(__arg_prTest, __arg_hrReplaceHr)                                             \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, TRUE, __arg_hrReplaceHr, nullptr)                         \
}                                                                                   


#define CPREx(__arg_prTest, __arg_hrReplaceHr)                                              \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, FALSE, __arg_hrReplaceHr, nullptr)                        \
}                                                                                   


#define CPRA(__arg_prTest)                                                                  \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, TRUE, E_OUTOFMEMORY, nullptr)                             \
}                                                                                   


#define CPR(__arg_prTest)                                                                   \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, FALSE, E_OUTOFMEMORY, nullptr)                            \
}

#define CPRAExL(__arg_prTest, __arg_hrReplaceHr, __arg_pszLog)                              \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, TRUE, __arg_hrReplaceHr, __arg_pszLog)                    \
}

#define CPRExL(__arg_prTest, __arg_hrReplaceHr, __arg_pszLog)                               \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, FALSE, __arg_hrReplaceHr, __arg_pszLog)                   \
}

#define CPRLA(__arg_prTest, __arg_pszLog)                                                   \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, TRUE, E_OUTOFMEMORY, __arg_pszLog)                        \
}

#define CPRL(__arg_prTest, __arg_pszLog)                                                    \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, FALSE, E_OUTOFMEMORY, __arg_pszLog)                       \
}                                                                                   

