
/*-------------------------------------------------------------*/
/*--- Public header file for the library.                   ---*/
/*---                                               bzlib.h ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.8 of 13 July 2019
   Copyright (C) 1996-2019 Julian Seward <jseward@acm.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */

#include <bzlib.h>

#ifndef _BZFLIB_H
#define _BZFLIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bzFileLike BZFILELIKE; // will struct bzfl_stream

struct filelike_callbacks BZ2_filelike_callbacks_FILE();

BZ_EXTERN BZFILELIKE* BZ_API(BZ2_bzflReadOpen) (
      int*  bzerror,
      void *stream, // FILE* f,
      int   verbosity,
      int   small,
      void* unused,
      int   nUnused,
      struct filelike_callbacks *callbacks
   );

BZ_EXTERN void BZ_API(BZ2_bzflReadClose) (
      int*    bzerror,
      BZFILELIKE* b
   );

BZ_EXTERN void BZ_API(BZ2_bzflReadGetUnused) (
      int*    bzerror,
      BZFILELIKE* b,
      void**  unused,
      int*    nUnused
   );

BZ_EXTERN int BZ_API(BZ2_bzflRead) (
      int*    bzerror,
      BZFILELIKE* b,
      void*   buf,
      int     len
   );

BZ_EXTERN BZFILELIKE* BZ_API(BZ2_bzflWriteOpen) (
      int*  bzerror,
      void *stream, // FILE* f,
      int   blockSize100k,
      int   verbosity,
      int   workFactor,
      struct filelike_callbacks *callbacks
   );

BZ_EXTERN void BZ_API(BZ2_bzflWrite) (
      int*    bzerror,
      BZFILELIKE* bzfl,
      void*   buf,
      int     len
   );

BZ_EXTERN void BZ_API(BZ2_bzflWriteClose) (
      int*          bzerror,
      BZFILELIKE*   b,
      int           abandon,
      unsigned int* nbytes_in,
      unsigned int* nbytes_out
   );

BZ_EXTERN void BZ_API(BZ2_bzflWriteClose64) (
      int*          bzerror,
      BZFILELIKE*   b,
      int           abandon,
      unsigned int* nbytes_in_lo32,
      unsigned int* nbytes_in_hi32,
      unsigned int* nbytes_out_lo32,
      unsigned int* nbytes_out_hi32
   );

BZFILELIKE * BZ_API(BZ2_bzflopen)
     (const void *src,
      const char *mode,
      struct filelike_callbacks *callbacks,
      void *open_ctx // will be passed to callbacks->fopen
      );

BZ_EXTERN size_t BZ_API(BZ2_bzflread)
       (
        void* buf, size_t size, size_t n, BZFILELIKE* bzfl
        );

BZ_EXTERN size_t BZ_API(BZ2_bzflwrite)
       (
        void * buff,
        size_t size,
        size_t nitems,
        void * stream
        );

BZ_EXTERN int BZ_API(BZ2_bzflflush) (
      BZFILELIKE* b
   );

BZ_EXTERN void BZ_API(BZ2_bzflclose) (
      BZFILELIKE* b
   );

BZ_EXTERN const char * BZ_API(BZ2_bzflerror) (
      BZFILELIKE *b,
      int    *errnum
   );

#ifdef __cplusplus
}
#endif

#endif

/*-------------------------------------------------------------*/
/*--- end                                           bzlib.h ---*/
/*-------------------------------------------------------------*/
