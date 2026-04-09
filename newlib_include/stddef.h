#ifndef _STDDEF_H
#define _STDDEF_H

#undef NULL
#ifdef __cplusplus
#define NULL __null
#else
#define NULL ((void *)0)
#endif

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

typedef __PTRDIFF_TYPE__  ptrdiff_t;
typedef __SIZE_TYPE__     size_t;
typedef __WCHAR_TYPE__    wchar_t;

#endif /* _STDDEF_H */
