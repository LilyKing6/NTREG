/*
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/ntreg/debug.h
 * PURPOSE:         Useful debugging macros
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/*
 * NOTE: Define NDEBUG before including this header
 * to disable debugging macros.
 */

#pragma once

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

/* Define DPRINT/DPRINTEx/RtlAssert unless the NDK is used */
#if !defined(_RTLFUNCS_H) && !defined(_NTDDK_)

/* Make sure we have basic types (some people include us *before* SDK)... */
#if !defined(_NTDEF_) && !defined(_NTDEF_H) && !defined(_WINDEF_) && !defined(_WINDEF_H)
// #error Please include SDK first.
#endif

void
RtlAssert(
    IN void* FailedAssertion,
    IN void* FileName,
    IN ULONG LineNumber,
    IN OPTIONAL char* Message
);

#endif /* !defined(_RTLFUNCS_H) && !defined(_NTDDK_) */

#ifndef assert
#if DBG && !defined(NASSERT)
#define assert(x) if (!(x)) { RtlAssert((void*)#x, (void*)__RELFILE__, __LINE__, ""); }
#else
#define assert(x) ((void) 0)
#endif
#endif

#ifndef ASSERT
#if DBG && !defined(NASSERT)
#define ASSERT(x) if (!(x)) { RtlAssert((void*)#x, (void*)__RELFILE__, __LINE__, ""); }
#else
#define ASSERT(x) ((void) 0)
#endif
#endif

#ifndef ASSERTMSG
#if DBG && !defined(NASSERT)
#define ASSERTMSG(m, x) if (!(x)) { RtlAssert((void*)#x, __RELFILE__, __LINE__, m); }
#else
#define ASSERTMSG(m, x) ((void) 0)
#endif
#endif

/* For internal purposes only */
#define __NOTICE(level, fmt, ...)   DPRINT(#level ":  %s at %s:%d " fmt, __FUNCTION__, __RELFILE__, __LINE__, ##__VA_ARGS__)

/* Print stuff only on Debug Builds*/
#if DBG

    /* These are always printed */
    #define DPRINT1(fmt, ...) do { \
        if (DPRINT("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
            DPRINT("(%s:%d) DPRINT() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

    /* These are printed only if NDEBUG is NOT defined */
    #ifndef NDEBUG

        #define DPRINT(fmt, ...) do { \
            if (DPRINT("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DPRINT("(%s:%d) DPRINT() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

    #if defined(_MSC_VER)
            #define DPRINT   __noop
    #else
            #define DPRINT(...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
    #endif

    #endif

#else /* not DBG */

    /* On non-debug builds, we never show these */

    #if defined(_MSC_VER)
        #define DPRINT1   __noop
        #define DPRINT    __noop

        #define ERR_(ch, ...)      __noop
        #define WARN_(ch, ...)     __noop
        #define TRACE_(ch, ...)    __noop
        #define INFO_(ch, ...)     __noop

        #define ERR__(ch, ...)     __noop
        #define WARN__(ch, ...)    __noop
        #define TRACE__(ch, ...)   __noop
        #define INFO__(ch, ...)    __noop
    #else
        // #define DPRINT1(...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        // #define DPRINT(...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)

        #define ERR_(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        #define WARN_(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        #define TRACE_(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        #define INFO_(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)

        #define ERR__(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        #define WARN__(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        #define TRACE__(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
        #define INFO__(ch, ...) do { if(0) { DPRINT(__VA_ARGS__); } } while(0)
    #endif /* _MSC_VER */

#endif /* not DBG */

/******************************************************************************/
/*
 * Declare a target-dependent process termination procedure.
 */



/* For internal purposes only */
#define __ERROR_DBGBREAK(...)   \
do {                            \
    DPRINT("" __VA_ARGS__);   \
    DbgBreakPoint();            \
} while (0)

/* For internal purposes only */
#define __ERROR_FATAL(Status, ...)      \
do {                                    \
    DPRINT("" __VA_ARGS__);           \
    DbgBreakPoint();                    \
    TerminateCurrentProcess(Status);    \
} while (0)

/*
 * These macros are designed to display an optional printf-like
 * user-defined message and to break into the debugger.
 * After that they allow to continue the program execution.
 */
#define ERROR_DBGBREAK(...)         \
do {                                \
    __NOTICE(ERROR, "\n");          \
    __ERROR_DBGBREAK(__VA_ARGS__);  \
} while (0)

#define UNIMPLEMENTED_DBGBREAK(...)         \
do {                                        \
    __NOTICE(ERROR, "is UNIMPLEMENTED!\n"); \
    __ERROR_DBGBREAK(__VA_ARGS__);          \
} while (0)

/*
 * These macros are designed to display an optional printf-like
 * user-defined message and to break into the debugger.
 * After that they halt the execution of the current thread.
 */
#define ERROR_FATAL(...)                                    \
do {                                                        \
    __NOTICE(UNRECOVERABLE ERROR, "\n");                    \
    __ERROR_FATAL(STATUS_ASSERTION_FAILURE, __VA_ARGS__);   \
} while (0)

#define UNIMPLEMENTED_FATAL(...)                            \
do {                                                        \
    __NOTICE(UNRECOVERABLE ERROR, "is UNIMPLEMENTED!\n");   \
    __ERROR_FATAL(STATUS_NOT_IMPLEMENTED, __VA_ARGS__);     \
} while (0)
/******************************************************************************/

#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))

#define __STRING2__(x) #x
#define __STRING__(x) __STRING2__(x)
#define __STRLINE__ __STRING__(__LINE__)

#if !defined(_MSC_VER) && !defined(__pragma)
#define __pragma(x) _Pragma(#x)
#endif

#define _WARN(msg) __pragma(message("WARNING! Line " __STRLINE__ ": " msg))

/* EOF */
