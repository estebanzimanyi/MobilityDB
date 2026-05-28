/*-------------------------------------------------------------------------
 *
 * pg_config.h.in  -  CMake-rendered PostgreSQL host configuration
 *
 * Generated at configure time by cmake/ConfigurePgConfig.cmake.
 * DO NOT edit the output (pg_config.h); edit this template or the
 * configure module instead.
 *
 * Narrowed to the set of macros actually referenced by MobilityDB,
 * MEOS, and the embedded PostgreSQL port sources. See the comment at
 * the top of ConfigurePgConfig.cmake for the rationale.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_CONFIG_H
#define PG_CONFIG_H

/*
 * Sizes (from CMake check_type_size)
 */
#define SIZEOF_VOID_P         8
#define SIZEOF_LONG           8
#define SIZEOF_LONG_LONG      8
#define SIZEOF_SIZE_T         8

/*
 * Alignment of native types (measured via offsetof at configure time)
 */
#define ALIGNOF_SHORT         2
#define ALIGNOF_INT           4
#define ALIGNOF_LONG          8
#define ALIGNOF_DOUBLE        8
#define MAXIMUM_ALIGNOF       8

/*
 * Byte order
 */
/* #undef WORDS_BIGENDIAN */

/*
 * 64-bit integer type. At most one of HAVE_LONG_INT_64 /
 * HAVE_LONG_LONG_INT_64 is defined - which one depends on the target's
 * data model (LP64 vs LLP64). INT64_MODIFIER is the corresponding
 * printf length modifier string ("l" or "ll").
 */
#define HAVE_LONG_INT_64       1
/* #undef HAVE_LONG_LONG_INT_64 */
#define PG_INT64_TYPE               long int
#define INT64_MODIFIER              "l"

/*
 * 128-bit integer type (optional; absent on MSVC and some 32-bit targets).
 * c.h derives HAVE_INT128 itself from PG_INT128_TYPE + ALIGNOF_PG_INT128_TYPE.
 */
#define PG_INT128_TYPE         __int128
#define ALIGNOF_PG_INT128_TYPE 16

/*
 * Compiler builtins / intrinsics
 */
#define HAVE__BUILTIN_BSWAP16           1
#define HAVE__BUILTIN_BSWAP32           1
#define HAVE__BUILTIN_BSWAP64           1
#define HAVE__BUILTIN_CLZ               1
#define HAVE__BUILTIN_CTZ               1
#define HAVE__BUILTIN_POPCOUNT          1
#define HAVE__BUILTIN_UNREACHABLE       1
#define HAVE__BUILTIN_OP_OVERFLOW       1
#define HAVE__BUILTIN_TYPES_COMPATIBLE_P 1
#define HAVE__STATIC_ASSERT             1
#define HAVE__CPUID                     1

/*
 * Standard headers
 */
#define HAVE_STRING_H    1
#define HAVE_STRINGS_H   1
#define HAVE_WCTYPE_H    1
/* #undef HAVE_CRTDEFS_H */

/*
 * Functions / declarations (a HAVE_DECL_* is defined only when the
 * corresponding HAVE_* is defined)
 */
#define HAVE_GETTIMEOFDAY        1
#define HAVE_READLINK            1
#define HAVE_FDATASYNC           1
#define HAVE_DECL_FDATASYNC      1
#define HAVE_POSIX_FADVISE       1
#define HAVE_DECL_POSIX_FADVISE  1
#define HAVE_SYNC_FILE_RANGE     1
#define HAVE_STRTOLL             1
#define HAVE_DECL_STRTOLL        1
#define HAVE_STRTOULL            1
#define HAVE_DECL_STRTOULL       1
#define HAVE_STRCHRNUL           1

/*
 * C99 `restrict` keyword (falls back to __restrict or nothing). Sources
 * use `pg_restrict`; `restrict` itself is mapped for code that hasn't
 * been adapted.
 */
#define pg_restrict restrict

/*
 * Struct / union feature tests
 */
#define HAVE_STRUCT_TM_TM_ZONE   1
#define HAVE_STRUCT_SOCKADDR_UN  1
/* #undef HAVE_UNION_SEMUN */
#define HAVE_INT_TIMEZONE        1

/*
 * PostgreSQL constants (fixed; never autodetected by PG's configure either)
 */
#define BLCKSZ             8192
#define XLOG_BLCKSZ        8192
#define NAMEDATALEN        64
#define MEMSET_LOOP_LIMIT  1024

/*
 * Preferred printf format attribute. GCC takes gnu_printf so that %m is
 * recognised; other compilers take plain printf.
 */
#define PG_PRINTF_ATTRIBUTE gnu_printf

#define PG_USE_STDBOOL            1
#define HAVE_FUNCNAME__FUNC       1
#define HAVE_FUNCNAME__FUNCTION   1
/* #undef USE_ASSERT_CHECKING */

#define PG_MAJORVERSION "14"

/*
 * Features MobilityDB/MEOS does not use. Left as explicit #undef lines
 * to make the contrast with upstream PG configure obvious.
 */
/* #undef USE_ICU */
/* #undef USE_OPENSSL */
/* #undef ENABLE_NLS */
/* #undef HAVE_PPC_LWARX_MUTEX_HINT */

#endif /* PG_CONFIG_H */
