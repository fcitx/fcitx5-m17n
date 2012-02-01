#ifndef __GETLINE_H
#define __GETLINE_H
#ifdef __cplusplus
#define CFUNC extern "C"
#else
#define CFUNC
#endif

CFUNC char *mygetline(const char *prompt);

#endif
