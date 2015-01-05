// "License": Public Domain
// I, Mathias Panzenböck, place this file hereby into the public domain. Use it at your own risk for whatever you like.

#ifndef PORTABLE_ENDIAN_H__
#define PORTABLE_ENDIAN_H__

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#	define __WINDOWS__

#endif

// GLIBC / Linux with endian(3) support, which was added in glibc 2.9.
// Intended to support CentOS 6 and newer.
#if defined(__linux__) && \
    ((__GLIBC__ > 3) || \
     (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 9))

#	include <endian.h>

#elif defined(__CYGWIN__)

#	include <endian.h>

#elif defined(__APPLE__)

#	include <libkern/OSByteOrder.h>

#	define htobe16(x) OSSwapHostToBigInt16(x)
#	define htole16(x) OSSwapHostToLittleInt16(x)
#	define be16toh(x) OSSwapBigToHostInt16(x)
#	define le16toh(x) OSSwapLittleToHostInt16(x)
 
#	define htobe32(x) OSSwapHostToBigInt32(x)
#	define htole32(x) OSSwapHostToLittleInt32(x)
#	define be32toh(x) OSSwapBigToHostInt32(x)
#	define le32toh(x) OSSwapLittleToHostInt32(x)
 
#	define htobe64(x) OSSwapHostToBigInt64(x)
#	define htole64(x) OSSwapHostToLittleInt64(x)
#	define be64toh(x) OSSwapBigToHostInt64(x)
#	define le64toh(x) OSSwapLittleToHostInt64(x)

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN

#elif defined(__OpenBSD__)

#	include <sys/endian.h>

#	define be16toh(x) betoh16(x)
#	define le16toh(x) letoh16(x)

#	define be32toh(x) betoh32(x)
#	define le32toh(x) letoh32(x)

#	define be64toh(x) betoh64(x)
#	define le64toh(x) letoh64(x)

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#	include <sys/endian.h>

#elif defined(__sun) && defined(__SVR4)

#	include <sys/types.h>
#	include <netinet/in.h>
#	include <inttypes.h>

#	define be64toh(x) ntohll(x)
#	define htobe64(x) htonll(x)

#elif defined(__WINDOWS__)

#	include <winsock2.h>
#	include <sys/param.h>

#	if BYTE_ORDER == LITTLE_ENDIAN

#		define htobe16(x) htons(x)
#		define htole16(x) (x)
#		define be16toh(x) ntohs(x)
#		define le16toh(x) (x)
 
#		define htobe32(x) htonl(x)
#		define htole32(x) (x)
#		define be32toh(x) ntohl(x)
#		define le32toh(x) (x)
 
#		define htobe64(x) htonll(x)
#		define htole64(x) (x)
#		define be64toh(x) ntohll(x)
#		define le64toh(x) (x)

#	elif BYTE_ORDER == BIG_ENDIAN

		/* that would be xbox 360 */
#		define htobe16(x) (x)
#		define htole16(x) __builtin_bswap16(x)
#		define be16toh(x) (x)
#		define le16toh(x) __builtin_bswap16(x)
 
#		define htobe32(x) (x)
#		define htole32(x) __builtin_bswap32(x)
#		define be32toh(x) (x)
#		define le32toh(x) __builtin_bswap32(x)
 
#		define htobe64(x) (x)
#		define htole64(x) __builtin_bswap64(x)
#		define be64toh(x) (x)
#		define le64toh(x) __builtin_bswap64(x)

#	else

#		error byte order not supported

#	endif

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN

#else

// Unsupported platforms.
// Intended to support CentOS 5 but hopefully not too far from
// the truth because we use the homebrew htonll, et al. implementations
// that were originally the sole implementation of this functionality
// in iperf 3.0.
#	warning platform not supported
#	include <endian.h>
#if BYTE_ORDER == BIG_ENDIAN
#define HTONLL(n) (n)
#define NTOHLL(n) (n)
#else
#define HTONLL(n) ((((unsigned long long)(n) & 0xFF) << 56) | \
                   (((unsigned long long)(n) & 0xFF00) << 40) | \
                   (((unsigned long long)(n) & 0xFF0000) << 24) | \
                   (((unsigned long long)(n) & 0xFF000000) << 8) | \
                   (((unsigned long long)(n) & 0xFF00000000) >> 8) | \
                   (((unsigned long long)(n) & 0xFF0000000000) >> 24) | \
                   (((unsigned long long)(n) & 0xFF000000000000) >> 40) | \
                   (((unsigned long long)(n) & 0xFF00000000000000) >> 56))

#define NTOHLL(n) ((((unsigned long long)(n) & 0xFF) << 56) | \
                   (((unsigned long long)(n) & 0xFF00) << 40) | \
                   (((unsigned long long)(n) & 0xFF0000) << 24) | \
                   (((unsigned long long)(n) & 0xFF000000) << 8) | \
                   (((unsigned long long)(n) & 0xFF00000000) >> 8) | \
                   (((unsigned long long)(n) & 0xFF0000000000) >> 24) | \
                   (((unsigned long long)(n) & 0xFF000000000000) >> 40) | \
                   (((unsigned long long)(n) & 0xFF00000000000000) >> 56))
#endif

#define htobe64(n) HTONLL(n)
#define be64toh(n) NTOHLL(n)

#endif

#endif

