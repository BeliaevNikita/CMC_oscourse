/*	$OpenBSD: endian.h,v 1.25 2014/12/21 04:49:00 guenther Exp $	*/

/*-
 * Copyright (c) 1997 Niklas Hallqvist.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Public definitions for little- and big-endian systems.
 * This file should be included as <endian.h> in userspace and as
 * <sys/endian.h> in the kernel.
 *
 * System headers that need endian information but that can't or don't
 * want to export the public names here should include <sys/_endian.h>
 * and use the internal names: _BYTE_ORDER, _*_ENDIAN, etc.
 */

/* endian.h - C-standard-only endian conversion helpers (inline functions)
 *
 * Based on the public API shape of OpenBSD endian.h, but implemented here
 * without compiler intrinsics. Only the non-commented names from your
 * snippet are provided, and macro wrappers are replaced with inline funcs.
 */

#ifndef _SYS_ENDIAN_H_
#define _SYS_ENDIAN_H_

#include <stdint.h>

#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN    4321

/* ---- Internal helpers (C-standard only) ---- */

static inline int __is_little_endian(void) {
    /* Standard C trick: examine lowest-addressed byte */
    const uint16_t x = 1;

    return *((const uint8_t *)(const void *)&x) == 1;
}

static inline uint16_t __swap16(uint16_t x) {
    return (uint16_t)((uint16_t)(x >> 8) | (uint16_t)(x << 8));
}

static inline uint32_t __swap32(uint32_t x) {
    return ((x >> 24) & 0x000000FFu) |
           ((x >>  8) & 0x0000FF00u) |
           ((x <<  8) & 0x00FF0000u) |
           ((x << 24) & 0xFF000000u);
}

static inline uint64_t __swap64(uint64_t x) {
    return ((x >> 56) & 0x00000000000000FFull) |
           ((x >> 40) & 0x000000000000FF00ull) |
           ((x >> 24) & 0x0000000000FF0000ull) |
           ((x >>  8) & 0x00000000FF000000ull) |
           ((x <<  8) & 0x000000FF00000000ull) |
           ((x << 24) & 0x0000FF0000000000ull) |
           ((x << 40) & 0x00FF000000000000ull) |
           ((x << 56) & 0xFF00000000000000ull);
}

static inline uint16_t __htobe16(uint16_t x) {
    return __is_little_endian() ? __swap16(x) : x;
}

static inline uint32_t __htobe32(uint32_t x) {
    return __is_little_endian() ? __swap32(x) : x;
}

static inline uint64_t __htobe64(uint64_t x) {
    return __is_little_endian() ? __swap64(x) : x;
}

static inline uint16_t __htole16(uint16_t x) {
    return __is_little_endian() ? x : __swap16(x);
}

static inline uint32_t __htole32(uint32_t x) {
    return __is_little_endian() ? x : __swap32(x);
}

static inline uint64_t __htole64(uint64_t x) {
    return __is_little_endian() ? x : __swap64(x);
}

/* These were function-like macros; now they are inline functions. */
static inline uint16_t htobe16(uint16_t x) { return __htobe16(x); }
static inline uint32_t htobe32(uint32_t x) { return __htobe32(x); }
static inline uint64_t htobe64(uint64_t x) { return __htobe64(x); }
static inline uint16_t htole16(uint16_t x) { return __htole16(x); }
static inline uint32_t htole32(uint32_t x) { return __htole32(x); }
static inline uint64_t htole64(uint64_t x) { return __htole64(x); }

/* POSIX names */
static inline uint16_t be16toh(uint16_t x) { return __htobe16(x); }
static inline uint32_t be32toh(uint32_t x) { return __htobe32(x); }
static inline uint64_t be64toh(uint64_t x) { return __htobe64(x); }
static inline uint16_t le16toh(uint16_t x) { return __htole16(x); }
static inline uint32_t le32toh(uint32_t x) { return __htole32(x); }
static inline uint64_t le64toh(uint64_t x) { return __htole64(x); }

/* swap* */
static inline uint16_t swap16(uint16_t x) { return __swap16(x); }
static inline uint32_t swap32(uint32_t x) { return __swap32(x); }
static inline uint64_t swap64(uint64_t x) { return __swap64(x); }

/* original BSD names */
static inline uint16_t betoh16(uint16_t x) { return __htobe16(x); }
static inline uint32_t betoh32(uint32_t x) { return __htobe32(x); }
static inline uint64_t betoh64(uint64_t x) { return __htobe64(x); }
static inline uint16_t letoh16(uint16_t x) { return __htole16(x); }
static inline uint32_t letoh32(uint32_t x) { return __htole32(x); }
static inline uint64_t letoh64(uint64_t x) { return __htole64(x); }

/* these were exposed here before */
static inline uint16_t htons(uint16_t x) { return __htobe16(x); }
static inline uint32_t htonl(uint32_t x) { return __htobe32(x); }
static inline uint16_t ntohs(uint16_t x) { return __htobe16(x); }
static inline uint32_t ntohl(uint32_t x) { return __htobe32(x); }

#endif /* _SYS_ENDIAN_H_ */
