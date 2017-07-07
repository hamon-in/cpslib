/* This header file can be replaced with inttypes.h and stdint.h 
	If both are available in the compiler used to compile the program */
#define bool	int
#define false	0
#define true	1
// These macros must exactly match those in the Windows SDK's intsafe.h.
#define INT8_MIN         (-127i8 - 1)
#define INT16_MIN        (-32767i16 - 1)
#define INT32_MIN        (-2147483647i32 - 1)
#define INT64_MIN        (-9223372036854775807i64 - 1)
#define INT8_MAX         127i8
#define INT16_MAX        32767i16
#define INT32_MAX        2147483647i32
#define INT64_MAX        9223372036854775807i64
#define UINT8_MAX        0xffui8
#define UINT16_MAX       0xffffui16
#define UINT32_MAX       0xffffffffui32
#define UINT64_MAX       0xffffffffffffffffui64

#define INT_LEAST8_MIN   INT8_MIN
#define INT_LEAST16_MIN  INT16_MIN
#define INT_LEAST32_MIN  INT32_MIN
#define INT_LEAST64_MIN  INT64_MIN
#define INT_LEAST8_MAX   INT8_MAX
#define INT_LEAST16_MAX  INT16_MAX
#define INT_LEAST32_MAX  INT32_MAX
#define INT_LEAST64_MAX  INT64_MAX
#define UINT_LEAST8_MAX  UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INT_FAST8_MIN    INT8_MIN
#define INT_FAST16_MIN   INT32_MIN
#define INT_FAST32_MIN   INT32_MIN
#define INT_FAST64_MIN   INT64_MIN
#define INT_FAST8_MAX    INT8_MAX
#define INT_FAST16_MAX   INT32_MAX
#define INT_FAST32_MAX   INT32_MAX
#define INT_FAST64_MAX   INT64_MAX
#define UINT_FAST8_MAX   UINT8_MAX
#define UINT_FAST16_MAX  UINT32_MAX
#define UINT_FAST32_MAX  UINT32_MAX
#define UINT_FAST64_MAX  UINT64_MAX

#ifdef _WIN64
    #define INTPTR_MIN   INT64_MIN
    #define INTPTR_MAX   INT64_MAX
    #define UINTPTR_MAX  UINT64_MAX
#else
    #define INTPTR_MIN   INT32_MIN
    #define INTPTR_MAX   INT32_MAX
    #define UINTPTR_MAX  UINT32_MAX
#endif

#define INTMAX_MIN       INT64_MIN
#define INTMAX_MAX       INT64_MAX
#define UINTMAX_MAX      UINT64_MAX

#define PTRDIFF_MIN      INTPTR_MIN
#define PTRDIFF_MAX      INTPTR_MAX

#ifndef SIZE_MAX
    #define SIZE_MAX     UINTPTR_MAX
#endif

#define SIG_ATOMIC_MIN   INT32_MIN
#define SIG_ATOMIC_MAX   INT32_MAX


#define WINT_MIN         0x0000
#define WINT_MAX         0xffff



#define PRId8        "hhd"
#define PRId16       "hd"
#define PRId32       "d"
#define PRId64       "lld"
#define PRIdLEAST8   PRId8
#define PRIdLEAST16  PRId16
#define PRIdLEAST32  PRId32
#define PRIdLEAST64  PRId64
#define PRIdFAST8    PRId8
#define PRIdFAST16   PRId32
#define PRIdFAST32   PRId32
#define PRIdFAST64   PRId64
#define PRIdMAX      PRId64
#ifdef _WIN64
    #define PRIdPTR  PRId64
#else
    #define PRIdPTR  PRId32
#endif

#define PRIi8        "hhi"
#define PRIi16       "hi"
#define PRIi32       "i"
#define PRIi64       "lli"
#define PRIiLEAST8   PRIi8
#define PRIiLEAST16  PRIi16
#define PRIiLEAST32  PRIi32
#define PRIiLEAST64  PRIi64
#define PRIiFAST8    PRIi8
#define PRIiFAST16   PRIi32
#define PRIiFAST32   PRIi32
#define PRIiFAST64   PRIi64
#define PRIiMAX      PRIi64
#ifdef _WIN64
    #define PRIiPTR  PRIi64
#else
    #define PRIiPTR  PRIi32
#endif

#define PRIo8        "hho"
#define PRIo16       "ho"
#define PRIo32       "o"
#define PRIo64       "llo"
#define PRIoLEAST8   PRIo8
#define PRIoLEAST16  PRIo16
#define PRIoLEAST32  PRIo32
#define PRIoLEAST64  PRIo64
#define PRIoFAST8    PRIo8
#define PRIoFAST16   PRIo32
#define PRIoFAST32   PRIo32
#define PRIoFAST64   PRIo64
#define PRIoMAX      PRIo64
#ifdef _WIN64
    #define PRIoPTR  PRIo64
#else
    #define PRIoPTR  PRIo32
#endif

#define PRIu8        "hhu"
#define PRIu16       "hu"
#define PRIu32       "u"
#define PRIu64       "llu"
#define PRIuLEAST8   PRIu8
#define PRIuLEAST16  PRIu16
#define PRIuLEAST32  PRIu32
#define PRIuLEAST64  PRIu64
#define PRIuFAST8    PRIu8
#define PRIuFAST16   PRIu32
#define PRIuFAST32   PRIu32
#define PRIuFAST64   PRIu64
#define PRIuMAX      PRIu64
#ifdef _WIN64
    #define PRIuPTR  PRIu64
#else
    #define PRIuPTR  PRIu32
#endif

#define PRIx8        "hhx"
#define PRIx16       "hx"
#define PRIx32       "x"
#define PRIx64       "llx"
#define PRIxLEAST8   PRIx8
#define PRIxLEAST16  PRIx16
#define PRIxLEAST32  PRIx32
#define PRIxLEAST64  PRIx64
#define PRIxFAST8    PRIx8
#define PRIxFAST16   PRIx32
#define PRIxFAST32   PRIx32
#define PRIxFAST64   PRIx64
#define PRIxMAX      PRIx64
#ifdef _WIN64
    #define PRIxPTR  PRIx64
#else
    #define PRIxPTR  PRIx32
#endif

#define PRIX8        "hhX"
#define PRIX16       "hX"
#define PRIX32       "X"
#define PRIX64       "llX"
#define PRIXLEAST8   PRIX8
#define PRIXLEAST16  PRIX16
#define PRIXLEAST32  PRIX32
#define PRIXLEAST64  PRIX64
#define PRIXFAST8    PRIX8
#define PRIXFAST16   PRIX32
#define PRIXFAST32   PRIX32
#define PRIXFAST64   PRIX64
#define PRIXMAX      PRIX64
#ifdef _WIN64
    #define PRIXPTR  PRIX64
#else
    #define PRIXPTR  PRIX32
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Input Format Specifier Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define SCNd8        "hhd"
#define SCNd16       "hd"
#define SCNd32       "d"
#define SCNd64       "lld"
#define SCNdLEAST8   SCNd8
#define SCNdLEAST16  SCNd16
#define SCNdLEAST32  SCNd32
#define SCNdLEAST64  SCNd64
#define SCNdFAST8    SCNd8
#define SCNdFAST16   SCNd32
#define SCNdFAST32   SCNd32
#define SCNdFAST64   SCNd64
#define SCNdMAX      SCNd64
#ifdef _WIN64
    #define SCNdPTR  SCNd64
#else
    #define SCNdPTR  SCNd32
#endif

#define SCNi8        "hhi"
#define SCNi16       "hi"
#define SCNi32       "i"
#define SCNi64       "lli"
#define SCNiLEAST8   SCNi8
#define SCNiLEAST16  SCNi16
#define SCNiLEAST32  SCNi32
#define SCNiLEAST64  SCNi64
#define SCNiFAST8    SCNi8
#define SCNiFAST16   SCNi32
#define SCNiFAST32   SCNi32
#define SCNiFAST64   SCNi64
#define SCNiMAX      SCNi64
#ifdef _WIN64
    #define SCNiPTR  SCNi64
#else
    #define SCNiPTR  SCNi32
#endif

#define SCNo8        "hho"
#define SCNo16       "ho"
#define SCNo32       "o"
#define SCNo64       "llo"
#define SCNoLEAST8   SCNo8
#define SCNoLEAST16  SCNo16
#define SCNoLEAST32  SCNo32
#define SCNoLEAST64  SCNo64
#define SCNoFAST8    SCNo8
#define SCNoFAST16   SCNo32
#define SCNoFAST32   SCNo32
#define SCNoFAST64   SCNo64
#define SCNoMAX      SCNo64
#ifdef _WIN64
    #define SCNoPTR  SCNo64
#else
    #define SCNoPTR  SCNo32
#endif

#define SCNu8        "hhu"
#define SCNu16       "hu"
#define SCNu32       "u"
#define SCNu64       "llu"
#define SCNuLEAST8   SCNu8
#define SCNuLEAST16  SCNu16
#define SCNuLEAST32  SCNu32
#define SCNuLEAST64  SCNu64
#define SCNuFAST8    SCNu8
#define SCNuFAST16   SCNu32
#define SCNuFAST32   SCNu32
#define SCNuFAST64   SCNu64
#define SCNuMAX      SCNu64
#ifdef _WIN64
    #define SCNuPTR  SCNu64
#else
    #define SCNuPTR  SCNu32
#endif

#define SCNx8        "hhx"
#define SCNx16       "hx"
#define SCNx32       "x"
#define SCNx64       "llx"
#define SCNxLEAST8   SCNx8
#define SCNxLEAST16  SCNx16
#define SCNxLEAST32  SCNx32
#define SCNxLEAST64  SCNx64
#define SCNxFAST8    SCNx8
#define SCNxFAST16   SCNx32
#define SCNxFAST32   SCNx32
#define SCNxFAST64   SCNx64
#define SCNxMAX      SCNx64
#ifdef _WIN64
    #define SCNxPTR  SCNx64
#else
    #define SCNxPTR  SCNx32
#endif

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;