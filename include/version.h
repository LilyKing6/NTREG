/* 
 * Copyright (C) 2024 Lily King
 * PROJECT:     kernel
 * FILE:        version.h
 * PURPOSE:     Defines the current version
 */

#ifndef __VERSION_H
#define __VERSION_H

#define SOFTNAME    "NTSoft Registry"
#define CODENAME    "LOONG"

#define COPYRIGHT_YEAR      2024

//
// NT Registry Version 1.0
//
#define VER_PRODUCTMAJORVERSION		1
#define VER_PRODUCTMINORVERSION		0

#ifndef SET_BUILD_NUM
#define VER_PRODUCTBUILD	0
#endif
#define VER_PRODUCTBUILD_QFE	0

// VERSION_PATCH

//内核版本
#define VER_MAJORMINOR2(x,y) #x "." #y
#define VER_MAJORMINOR1(x,y) VER_MAJORMINOR2(x, y)
#define VERSION_KERNEL  VER_MAJORMINOR1(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION)

//产品版本
#define VER_PRODUCT2(w,x,y,z) #w "." #x "." #y "." #z
#define VER_PRODUCT1(w,x,y,z) VER_PRODUCT2(w,x,y,z)
#define VERSION_PRODUCT VER_PRODUCT1(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION, VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE)

//
// Full Product Version
//
#define VER_PRODUCTVERSION                  \
    VER_PRODUCTMAJORVERSION,VER_PRODUCTMINORVERSION,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE

//唯一版本号标识
#define VERSION_SUM    (VER_PRODUCTMAJORVERSION * 1e11 + VER_PRODUCTMINORVERSION * 1e10 + VER_PRODUCTBUILD * 1e5 + VER_PRODUCTBUILD_QFE)

//
// u"CVS", u"RC1", u"RC2" or u"FINAu"
//
#define VER_PRODUCTBETA_STR         ""

#define VER_PRODUCTBUILD_TYPE	    u"loong_m5"

/* 是RTM版本 */
// #define IS_RTM
#ifdef IS_RTM
/* RTM发布时间 */
#define RELEASE_DATE		20240901L
#endif

/* 大写 */
// #define Uppercase
#ifdef Uppercase
    #ifdef _WIN32
        #ifdef _WIN64
            #define _PLATFORM "WIN64"
        #else
            #define _PLATFORM "WIN32"
        #endif
    #else
        #ifdef __linux__
            #ifdef __x86_64__
                #define _PLATFORM "LINUX64"
            #elif __i386__
                #define _PLATFORM "LINUX32"
            #endif
        #else
            #ifdef __x86_64__
                #define _PLATFORM "MAC64"
            #elif __i386__
                #define _PLATFORM "MAC32"
            #endif
        #endif
    #endif
#else
    #ifdef _WIN32
        #ifdef _WIN64
            #define _PLATFORM "win64"
        #else
            #define _PLATFORM "win32"
        #endif
    #else
        #ifdef __linux__
            #ifdef __x86_64__
                #define _PLATFORM "linux64"
            #elif __i386__
                #define _PLATFORM "linux32"
            #endif
        #else
            #ifdef __x86_64__
                #define _PLATFORM "mac64"
            #elif __i386__
                #define _PLATFORM "mac32"
            #endif
        #endif
    #endif
#endif

#endif
/* EOF */
