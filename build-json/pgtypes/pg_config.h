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
/* #undef HAVE__BUILTIN_BSWAP16 */
/* #undef HAVE__BUILTIN_BSWAP32 */
/* #undef HAVE__BUILTIN_BSWAP64 */
/* #undef HAVE__BUILTIN_CLZ */
/* #undef HAVE__BUILTIN_CTZ */
/* #undef HAVE__BUILTIN_POPCOUNT */
/* #undef HAVE__BUILTIN_UNREACHABLE */
/* #undef HAVE__BUILTIN_OP_OVERFLOW */
/* #undef HAVE__BUILTIN_TYPES_COMPATIBLE_P */
/* #undef HAVE__STATIC_ASSERT */
/* #undef HAVE__CPUID */

/*
 * Standard headers
 */
/* #undef HAVE_STRING_H */
/* #undef HAVE_STRINGS_H */
/* #undef HAVE_WCTYPE_H */
/* #undef HAVE_CRTDEFS_H */

/*
 * Functions / declarations (a HAVE_DECL_* is defined only when the
 * corresponding HAVE_* is defined)
 */
/* #undef HAVE_GETTIMEOFDAY */
/* #undef HAVE_READLINK */
/* #undef HAVE_FDATASYNC */
/* #undef HAVE_DECL_FDATASYNC */
/* #undef HAVE_POSIX_FADVISE */
/* #undef HAVE_DECL_POSIX_FADVISE */
/* #undef HAVE_SYNC_FILE_RANGE */
/* #undef HAVE_STRTOLL */
/* #undef HAVE_DECL_STRTOLL */
/* #undef HAVE_STRTOULL */
/* #undef HAVE_DECL_STRTOULL */
#define HAVE_STRCHRNUL           1

/*
 * C99 `restrict` keyword (falls back to __restrict or nothing). Sources
 * use `pg_restrict`; `restrict` itself is mapped for code that hasn't
 * been adapted.
 */
#define pg_restrict 

/*
 * Struct / union feature tests
 */
/* #undef HAVE_STRUCT_TM_TM_ZONE */
/* #undef HAVE_STRUCT_SOCKADDR_UN */
/* #undef HAVE_UNION_SEMUN */
/* #undef HAVE_INT_TIMEZONE */

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

/* #undef PG_USE_STDBOOL */
/* #undef HAVE_FUNCNAME__FUNC */
/* #undef HAVE_FUNCNAME__FUNCTION */
/* #undef USE_ASSERT_CHECKING */

#define PG_MAJORVERSION "18"

/*
 * Features MobilityDB/MEOS does not use. Left as explicit #undef lines
 * to make the contrast with upstream PG configure obvious.
 */
/* #undef USE_ICU */
/* #undef USE_OPENSSL */
/* #undef ENABLE_NLS */

/* Define to 1 to build client libraries as thread-safe code.
   (--enable-thread-safety) */
#define ENABLE_THREAD_SAFETY 1

/* Define to 1 if gettimeofday() takes only 1 argument. */
/* #undef GETTIMEOFDAY_1ARG */

#ifdef GETTIMEOFDAY_1ARG
# define gettimeofday(a,b) gettimeofday(a)
#endif

/* Define to 1 if you have the `append_history' function. */
#define HAVE_APPEND_HISTORY 1

/* Define to 1 if you have the `ASN1_STRING_get0_data' function. */
/* #undef HAVE_ASN1_STRING_GET0_DATA */

/* Define to 1 if you want to use atomics if available. */
#define HAVE_ATOMICS 1

/* Define to 1 if you have the <atomic.h> header file. */
/* #undef HAVE_ATOMIC_H */

/* Define to 1 if you have the `backtrace_symbols' function. */
#define HAVE_BACKTRACE_SYMBOLS 1

/* Define to 1 if you have the `BIO_get_data' function. */
/* #undef HAVE_BIO_GET_DATA */

/* Define to 1 if you have the `BIO_meth_new' function. */
/* #undef HAVE_BIO_METH_NEW */

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define to 1 if your compiler handles computed gotos. */
#define HAVE_COMPUTED_GOTO 1

/* Define to 1 if you have the `copyfile' function. */
/* #undef HAVE_COPYFILE */

/* Define to 1 if you have the <copyfile.h> header file. */
/* #undef HAVE_COPYFILE_H */

/* Define to 1 if you have the <crtdefs.h> header file. */
/* #undef HAVE_CRTDEFS_H */

/* Define to 1 if you have the `CRYPTO_lock' function. */
/* #undef HAVE_CRYPTO_LOCK */

/* Define to 1 if you have the declaration of `fdatasync', and to 0 if you
   don't. */
#define HAVE_DECL_FDATASYNC 1

/* Define to 1 if you have the declaration of `F_FULLFSYNC', and to 0 if you
   don't. */
#define HAVE_DECL_F_FULLFSYNC 0

/* Define to 1 if you have the declaration of
   `LLVMCreateGDBRegistrationListener', and to 0 if you don't. */
/* #undef HAVE_DECL_LLVMCREATEGDBREGISTRATIONLISTENER */

/* Define to 1 if you have the declaration of
   `LLVMCreatePerfJITEventListener', and to 0 if you don't. */
/* #undef HAVE_DECL_LLVMCREATEPERFJITEVENTLISTENER */

/* Define to 1 if you have the declaration of `LLVMGetHostCPUFeatures', and to
   0 if you don't. */
/* #undef HAVE_DECL_LLVMGETHOSTCPUFEATURES */

/* Define to 1 if you have the declaration of `LLVMGetHostCPUName', and to 0
   if you don't. */
/* #undef HAVE_DECL_LLVMGETHOSTCPUNAME */

/* Define to 1 if you have the declaration of `LLVMOrcGetSymbolAddressIn', and
   to 0 if you don't. */
/* #undef HAVE_DECL_LLVMORCGETSYMBOLADDRESSIN */

/* Define to 1 if you have the declaration of `posix_fadvise', and to 0 if you
   don't. */
#define HAVE_DECL_POSIX_FADVISE 1

/* Define to 1 if you have the declaration of `preadv', and to 0 if you don't.
   */
#define HAVE_DECL_PREADV 1

/* Define to 1 if you have the declaration of `pwritev', and to 0 if you
   don't. */
#define HAVE_DECL_PWRITEV 1

/* Define to 1 if you have the declaration of `RTLD_GLOBAL', and to 0 if you
   don't. */
#define HAVE_DECL_RTLD_GLOBAL 1

/* Define to 1 if you have the declaration of `RTLD_NOW', and to 0 if you
   don't. */
#define HAVE_DECL_RTLD_NOW 1

/* Define to 1 if you have the declaration of `strlcat', and to 0 if you
   don't. */
#define HAVE_DECL_STRLCAT 0

/* Define to 1 if you have the declaration of `strlcpy', and to 0 if you
   don't. */
#define HAVE_DECL_STRLCPY 0

/* Define to 1 if you have the declaration of `strnlen', and to 0 if you
   don't. */
#define HAVE_DECL_STRNLEN 1

/* Define to 1 if you have the declaration of `strtoll', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOLL 1

/* Define to 1 if you have the declaration of `strtoull', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOULL 1

/* Define to 1 if you have the `dlopen' function. */
#define HAVE_DLOPEN 1

/* Define to 1 if you have the <editline/history.h> header file. */
/* #undef HAVE_EDITLINE_HISTORY_H */

/* Define to 1 if you have the <editline/readline.h> header file. */
/* #undef HAVE_EDITLINE_READLINE_H */

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the `explicit_bzero' function. */
#define HAVE_EXPLICIT_BZERO 1

/* Define to 1 if you have the `fdatasync' function. */
#define HAVE_FDATASYNC 1

/* Define to 1 if you have the `fls' function. */
/* #undef HAVE_FLS */

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Define to 1 if your compiler understands __func__. */
#define HAVE_FUNCNAME__FUNC 1

/* Define to 1 if your compiler understands __FUNCTION__. */
/* #undef HAVE_FUNCNAME__FUNCTION */

/* Define to 1 if you have __atomic_compare_exchange_n(int *, int *, int). */
#define HAVE_GCC__ATOMIC_INT32_CAS 1

/* Define to 1 if you have __atomic_compare_exchange_n(int64 *, int64 *,
   int64). */
#define HAVE_GCC__ATOMIC_INT64_CAS 1

/* Define to 1 if you have __sync_lock_test_and_set(char *) and friends. */
#define HAVE_GCC__SYNC_CHAR_TAS 1

/* Define to 1 if you have __sync_val_compare_and_swap(int *, int, int). */
#define HAVE_GCC__SYNC_INT32_CAS 1

/* Define to 1 if you have __sync_lock_test_and_set(int *) and friends. */
#define HAVE_GCC__SYNC_INT32_TAS 1

/* Define to 1 if you have __sync_val_compare_and_swap(int64 *, int64, int64).
   */
#define HAVE_GCC__SYNC_INT64_CAS 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `gethostbyname_r' function. */
#define HAVE_GETHOSTBYNAME_R 1

/* Define to 1 if you have the `getifaddrs' function. */
#define HAVE_GETIFADDRS 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define to 1 if you have the `getpeereid' function. */
/* #undef HAVE_GETPEEREID */

/* Define to 1 if you have the `getpeerucred' function. */
/* #undef HAVE_GETPEERUCRED */

/* Define to 1 if you have the `getpwuid_r' function. */
#define HAVE_GETPWUID_R 1

/* Define to 1 if you have the `getrlimit' function. */
#define HAVE_GETRLIMIT 1

/* Define to 1 if you have the `getrusage' function. */
#define HAVE_GETRUSAGE 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY /* MSYS2 UCRT64 provides this function */

/* Define to 1 if you have the <gssapi/gssapi.h> header file. */
/* #undef HAVE_GSSAPI_GSSAPI_H */

/* Define to 1 if you have the <gssapi.h> header file. */
/* #undef HAVE_GSSAPI_H */

/* Define to 1 if you have the <history.h> header file. */
/* #undef HAVE_HISTORY_H */

/* Define to 1 if you have the `history_truncate_file' function. */
#define HAVE_HISTORY_TRUNCATE_FILE 1

/* Define to 1 if you have the `HMAC_CTX_free' function. */
/* #undef HAVE_HMAC_CTX_FREE */

/* Define to 1 if you have the `HMAC_CTX_new' function. */
/* #undef HAVE_HMAC_CTX_NEW */

/* Define to 1 if you have the <ifaddrs.h> header file. */
#define HAVE_IFADDRS_H 1

/* Define to 1 if you have the `inet_aton' function. */
#define HAVE_INET_ATON 1

/* Define to 1 if the system has the type `int64'. */
/* #undef HAVE_INT64 */

/* Define to 1 if the system has the type `int8'. */
/* #undef HAVE_INT8 */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the global variable 'int opterr'. */
#define HAVE_INT_OPTERR 1

/* Define to 1 if you have the global variable 'int optreset'. */
/* #undef HAVE_INT_OPTRESET */

/* Define to 1 if you have the global variable 'int timezone'. */
#define HAVE_INT_TIMEZONE 1

/* Define to 1 if you have support for IPv6. */
#define HAVE_IPV6 1

/* Define to 1 if __builtin_constant_p(x) implies "i"(x) acceptance. */
/* #undef HAVE_I_CONSTRAINT__BUILTIN_CONSTANT_P */

/* Define to 1 if you have the `kqueue' function. */
/* #undef HAVE_KQUEUE */

/* Define to 1 if you have the <langinfo.h> header file. */
#define HAVE_LANGINFO_H 1

/* Define to 1 if you have the <ldap.h> header file. */
/* #undef HAVE_LDAP_H */

/* Define to 1 if you have the `ldap_initialize' function. */
/* #undef HAVE_LDAP_INITIALIZE */

/* Define to 1 if you have the `crypto' library (-lcrypto). */
/* #undef HAVE_LIBCRYPTO */

/* Define to 1 if you have the `ldap' library (-lldap). */
/* #undef HAVE_LIBLDAP */

/* Define to 1 if you have the `lz4' library (-llz4). */
/* #undef HAVE_LIBLZ4 */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `pam' library (-lpam). */
/* #undef HAVE_LIBPAM */

/* Define if you have a function readline library */
#define HAVE_LIBREADLINE 1

/* Define to 1 if you have the `selinux' library (-lselinux). */
/* #undef HAVE_LIBSELINUX */

/* Define to 1 if you have the `ssl' library (-lssl). */
/* #undef HAVE_LIBSSL */

/* Define to 1 if you have the `wldap32' library (-lwldap32). */
/* #undef HAVE_LIBWLDAP32 */

/* Define to 1 if you have the `xml2' library (-lxml2). */
#define HAVE_LIBXML2 1

/* Define to 1 if you have the `xslt' library (-lxslt). */
/* #undef HAVE_LIBXSLT */

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the `link' function. */
#define HAVE_LINK 1

/* Define to 1 if the system has the type `locale_t'. */
#define HAVE_LOCALE_T 1

/* HAVE_LONG_INT_64 / HAVE_LONG_LONG_INT_64 are set by the cmakedefine
 * lines higher up in this file (CMake substitutes one or the other based
 * on SIZEOF_LONG). The stale hard-coded duplicates that used to live
 * here were Linux-snapshot residue and clobbered the platform-aware
 * values on LLP64 (Windows MSYS2/MinGW) builds. */

/* Define to 1 if you have the <lz4.h> header file. */
/* #undef HAVE_LZ4_H */

/* Define to 1 if you have the <mbarrier.h> header file. */
/* #undef HAVE_MBARRIER_H */

/* Define to 1 if you have the `mbstowcs_l' function. */
/* #undef HAVE_MBSTOWCS_L */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset_s' function. */
/* #undef HAVE_MEMSET_S */

/* Define to 1 if the system has the type `MINIDUMP_TYPE'. */
/* #undef HAVE_MINIDUMP_TYPE */

/* Define to 1 if you have the `mkdtemp' function. */
#define HAVE_MKDTEMP 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the <net/if.h> header file. */
#define HAVE_NET_IF_H 1

/* Define to 1 if you have the `OPENSSL_init_ssl' function. */
/* #undef HAVE_OPENSSL_INIT_SSL */

/* Define to 1 if you have the <ossp/uuid.h> header file. */
/* #undef HAVE_OSSP_UUID_H */

/* Define to 1 if you have the <pam/pam_appl.h> header file. */
/* #undef HAVE_PAM_PAM_APPL_H */

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the `posix_fadvise' function. */
#define HAVE_POSIX_FADVISE 1

/* Define to 1 if you have the `posix_fallocate' function. */
#define HAVE_POSIX_FALLOCATE 1

/* Define to 1 if the assembler supports PPC's LWARX mutex hint bit. */
/* #undef HAVE_PPC_LWARX_MUTEX_HINT */

#endif /* PG_CONFIG_H */
