#pragma once

#include <inttypes.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef int atomic_t;

#define dsb()   \
        do {    \
            __asm__ __volatile__ ("dsb" : : : "memory"); \
        } while (0)

#define mt65xx_reg_sync_writel(v, a) \
        do {    \
            *(volatile unsigned int *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt65xx_reg_sync_writew(v, a) \
        do {    \
            *(volatile unsigned short *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt65xx_reg_sync_writeb(v, a) \
        do {    \
            *(volatile unsigned char *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt_reg_sync_writel(v, a) \
        do {    \
            *(volatile unsigned int *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt_reg_sync_writew(v, a) \
        do {    \
            *(volatile unsigned short *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt_reg_sync_writeb(v, a) \
        do {    \
            *(volatile unsigned char *)(a) = (v);    \
            dsb(); \
        } while (0)

static inline u16 __raw_readw(const volatile void *addr)
{
    u16 val;
    asm volatile("ldrh %0, %1"
             : "=r" (val)
             : "Q" (*(volatile u16 *)addr));
    return val;
}

static inline u8 __raw_readb(const volatile void *addr)
{
    u8 val;
    asm volatile("ldrb %0, %1"
             : "=r" (val)
             : "Qo" (*(volatile u8 *)addr));
    return val;
}

static inline u32 __raw_readl(const volatile void *addr)
{
    u32 val;
    asm volatile("ldr %0, %1"
             : "=r" (val)
             : "Qo" (*(volatile u32 *)addr));
    return val;
}
