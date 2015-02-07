
#ifndef __RTMP_TYPE_H__
#define __RTMP_TYPE_H__

#define PACKED  __attribute__ ((packed))

// Put platform dependent declaration here
// For example, linux type definition
typedef unsigned char		UINT8;
typedef unsigned short		UINT16;
typedef unsigned int		UINT32;
typedef unsigned long long	UINT64;
typedef int					INT32;
typedef long long 			INT64;

typedef unsigned char *			PUINT8;
typedef unsigned short *		PUINT16;
typedef unsigned int *			PUINT32;
typedef unsigned long long *	PUINT64;
typedef int	*					PINT32;
typedef long long * 			PINT64;

typedef signed char			CHAR;
typedef signed short		SHORT;
typedef signed int			INT;
typedef signed long			LONG;
typedef signed long long	LONGLONG;


typedef unsigned char		UCHAR;
typedef unsigned short		USHORT;
typedef unsigned int		UINT;
typedef unsigned long		ULONG;
typedef unsigned long long	ULONGLONG;

typedef unsigned char		BOOLEAN;
typedef void				VOID;

typedef VOID *				PVOID;
typedef CHAR *				PCHAR;
typedef UCHAR * 			PUCHAR;
typedef USHORT *			PUSHORT;
typedef LONG *				PLONG;
typedef ULONG *				PULONG;
typedef UINT *				PUINT;

typedef unsigned int	NDIS_MEDIA_STATE;

typedef union _LARGE_INTEGER {
    struct {
        UINT LowPart;
        INT32 HighPart;
    } u;
    INT64 QuadPart;
} LARGE_INTEGER;

#endif  // __RTMP_TYPE_H__

