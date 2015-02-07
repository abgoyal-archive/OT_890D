

#ifndef WTF_DisallowCType_h
#define WTF_DisallowCType_h

// The behavior of many of the functions in the <ctype.h> header is dependent
// on the current locale. But almost all uses of these functions are for
// locale-independent, ASCII-specific purposes. In WebKit code we use our own
// ASCII-specific functions instead. This header makes sure we get a compile-time
// error if we use one of the <ctype.h> functions by accident.

#include <ctype.h>

#undef isalnum
#undef isalpha
#undef isascii
#undef isblank
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
#undef toascii
#undef tolower
#undef toupper

#define isalnum WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isalpha WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isascii WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isblank WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define iscntrl WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isdigit WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isgraph WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define islower WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isprint WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define ispunct WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isspace WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isupper WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define isxdigit WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define toascii WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define tolower WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h
#define toupper WTF_Please_use_ASCIICType_instead_of_ctype_see_comment_in_ASCIICType_h

#endif
