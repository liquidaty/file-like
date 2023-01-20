// Adapted from https://raw.githubusercontent.com/libarchive/bzip2/master/bzlib.c

#include <bzlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../filelike.h"
#include "bzflib.h"
#include "bzlib_private.h"

// bzFileLike is the same as bzFileLike except that the handle
// is a void * and related functions are defined in the callbacks
typedef struct bzFileLike {
  void *handle; //  FILE*     handle;
  char      buf[BZ_MAX_UNUSED];
  int32_t     bufN;
  unsigned char      writing;
  bz_stream strm;
  int32_t     lastErr;
  unsigned char      initialisedOk;
  struct filelike_callbacks cb;
} bzFileLike;

#define BZFL_SETERR(eee)                  \
  {                                       \
    if(bzerror != NULL) *bzerror = eee;   \
    if(bzfl != NULL) bzfl->lastErr = eee; \
  }


/*---------------------------------------------------*/
int BZ_API(BZ2_bzflRead)
     ( int*    bzerror,
       BZFILELIKE * bzfl,
       void*   buf,
       int     len )
{
   int32_t   n, ret;

   BZFL_SETERR(BZ_OK);

   if (bzfl == NULL || buf == NULL || len < 0)
      { BZFL_SETERR(BZ_PARAM_ERROR); return 0; };

   if (bzfl->writing)
      { BZFL_SETERR(BZ_SEQUENCE_ERROR); return 0; };

   if (len == 0)
      { BZFL_SETERR(BZ_OK); return 0; };

   bzfl->strm.avail_out = len;
   bzfl->strm.next_out = buf;

   while (1) {

      if (bzfl->cb.ferror(bzfl->handle))
         { BZFL_SETERR(BZ_IO_ERROR); return 0; };

      if (bzfl->strm.avail_in == 0 && !bzfl->cb.feof(bzfl->handle)) {
        n = bzfl->cb.fread ( bzfl->buf, sizeof(unsigned char),
                             BZ_MAX_UNUSED, bzfl->handle );
        if (bzfl->cb.ferror(bzfl->handle))
          { BZFL_SETERR(BZ_IO_ERROR); return 0; };
        bzfl->bufN = n;
        bzfl->strm.avail_in = bzfl->bufN;
        bzfl->strm.next_in = bzfl->buf;
      }

      ret = BZ2_bzDecompress ( &(bzfl->strm) );

      if (ret != BZ_OK && ret != BZ_STREAM_END)
         { BZFL_SETERR(ret); return 0; };

      if (ret == BZ_OK && bzfl->cb.feof(bzfl->handle) &&
          bzfl->strm.avail_in == 0 && bzfl->strm.avail_out > 0)
        { BZFL_SETERR(BZ_UNEXPECTED_EOF); return 0; };

      if (ret == BZ_STREAM_END)
         { BZFL_SETERR(BZ_STREAM_END);
           return len - bzfl->strm.avail_out; };
      if (bzfl->strm.avail_out == 0)
         { BZFL_SETERR(BZ_OK); return len; };

   }

   return 0; /*not reached*/
}

/*-------------------------------------------------------------*/
/*--- Library top-level functions.                          ---*/
/*---                                               bzlib.c ---*/
/*-------------------------------------------------------------*/

// #include "bzlib_private.h"

BZFILELIKE* BZ_API(BZ2_bzflWriteOpen)
                    ( int*  bzerror,
                      void *stream,
                      // FILE* f,
                      int   blockSize100k,
                      int   verbosity,
                      int   workFactor,
                      struct filelike_callbacks *cb
 )
{
   int32_t   ret;
   BZFILELIKE *bzfl = NULL;
   BZFL_SETERR(BZ_OK);

   if (stream == NULL ||
       (blockSize100k < 1 || blockSize100k > 9) ||
       (workFactor < 0 || workFactor > 250) ||
       (verbosity < 0 || verbosity > 4))
      { BZFL_SETERR(BZ_PARAM_ERROR); return NULL; };

   if (cb->ferror && cb->ferror(stream))
      { BZFL_SETERR(BZ_IO_ERROR); return NULL; };

   bzfl = calloc(1, sizeof(*bzfl));
   if (bzfl == NULL)
      { BZFL_SETERR(BZ_MEM_ERROR); return NULL; };

   bzfl->cb = *cb;

   BZFL_SETERR(BZ_OK);
   bzfl->initialisedOk = 0;
   bzfl->bufN          = 0;
   bzfl->handle        = stream;
   bzfl->writing       = 1;
   bzfl->strm.bzalloc  = NULL;
   bzfl->strm.bzfree   = NULL;
   bzfl->strm.opaque   = NULL;

   if (workFactor == 0) workFactor = 30;
   ret = BZ2_bzCompressInit ( &(bzfl->strm), blockSize100k,
                              verbosity, workFactor );
   if (ret != BZ_OK)
      { BZFL_SETERR(ret); free(bzfl); return NULL; };

   bzfl->strm.avail_in = 0;
   bzfl->initialisedOk = 1;
   return bzfl;
}



/*---------------------------------------------------*/
void BZ_API(BZ2_bzflWrite)
             ( int*    bzerror,
               BZFILELIKE* bzfl,
               void*   buf,
               int     len )
{
   int32_t n, n2, ret;

   BZFL_SETERR(BZ_OK);
   if (bzfl == NULL || buf == NULL || len < 0)
      { BZFL_SETERR(BZ_PARAM_ERROR); return; };
   if (!(bzfl->writing))
      { BZFL_SETERR(BZ_SEQUENCE_ERROR); return; };
   if (bzfl->cb.ferror(bzfl->handle))
      { BZFL_SETERR(BZ_IO_ERROR); return; };

   if (len == 0)
      { BZFL_SETERR(BZ_OK); return; };

   bzfl->strm.avail_in = len;
   bzfl->strm.next_in  = buf;

   while (1) {
      bzfl->strm.avail_out = BZ_MAX_UNUSED;
      bzfl->strm.next_out = bzfl->buf;
      ret = BZ2_bzCompress ( &(bzfl->strm), BZ_RUN );
      if (ret != BZ_RUN_OK)
         { BZFL_SETERR(ret); return; };

      if (bzfl->strm.avail_out < BZ_MAX_UNUSED) {
         n = BZ_MAX_UNUSED - bzfl->strm.avail_out;
         n2 = bzfl->cb.fwrite ( (void*)(bzfl->buf), sizeof(unsigned char),
                       n, bzfl->handle );
         if (n != n2 || bzfl->cb.ferror(bzfl->handle))
            { BZFL_SETERR(BZ_IO_ERROR); return; };
      }

      if (bzfl->strm.avail_in == 0)
         { BZFL_SETERR(BZ_OK); return; };
   }
}


/*---------------------------------------------------*/
void BZ_API(BZ2_bzflWriteClose)
                  ( int*          bzerror,
                    BZFILELIKE*   bzfl,
                    int           abandon,
                    unsigned int* nbytes_in,
                    unsigned int* nbytes_out ) {
   BZ2_bzflWriteClose64(bzerror, bzfl, abandon,
                        nbytes_in, NULL, nbytes_out, NULL);
}


void BZ_API(BZ2_bzflWriteClose64)
     ( int*          bzerror,
       BZFILELIKE*   bzfl,
       int           abandon,
       unsigned int* nbytes_in_lo32,
       unsigned int* nbytes_in_hi32,
       unsigned int* nbytes_out_lo32,
       unsigned int* nbytes_out_hi32 )
{
   int32_t   n, n2, ret;

   if (bzfl == NULL)
      { BZFL_SETERR(BZ_OK); return; };

   if (!(bzfl->writing))
      { BZFL_SETERR(BZ_SEQUENCE_ERROR); return; };
   if (bzfl->cb.ferror(bzfl->handle))
      { BZFL_SETERR(BZ_IO_ERROR); return; };

   if (nbytes_in_lo32 != NULL) *nbytes_in_lo32 = 0;
   if (nbytes_in_hi32 != NULL) *nbytes_in_hi32 = 0;
   if (nbytes_out_lo32 != NULL) *nbytes_out_lo32 = 0;
   if (nbytes_out_hi32 != NULL) *nbytes_out_hi32 = 0;

   if ((!abandon) && bzfl->lastErr == BZ_OK) {
      while (1) {
         bzfl->strm.avail_out = BZ_MAX_UNUSED;
         bzfl->strm.next_out = bzfl->buf;
         ret = BZ2_bzCompress(&(bzfl->strm), BZ_FINISH);
         if (ret != BZ_FINISH_OK && ret != BZ_STREAM_END)
            { BZFL_SETERR(ret); return; };

         if (bzfl->strm.avail_out < BZ_MAX_UNUSED) {
            n = BZ_MAX_UNUSED - bzfl->strm.avail_out;
            n2 = bzfl->cb.fwrite((void*)(bzfl->buf), sizeof(unsigned char),
                              n, bzfl->handle );
            if (n != n2 || bzfl->cb.ferror(bzfl->handle))
               { BZFL_SETERR(BZ_IO_ERROR); return; };
         }

         if (ret == BZ_STREAM_END) break;
      }
   }

   if ( !abandon && !bzfl->cb.ferror ( bzfl->handle ) ) {
     bzfl->cb.fflush(bzfl->handle );
     if (bzfl->cb.ferror(bzfl->handle))
       { BZFL_SETERR(BZ_IO_ERROR); return; };
   }

   if (nbytes_in_lo32 != NULL)
      *nbytes_in_lo32 = bzfl->strm.total_in_lo32;
   if (nbytes_in_hi32 != NULL)
      *nbytes_in_hi32 = bzfl->strm.total_in_hi32;
   if (nbytes_out_lo32 != NULL)
      *nbytes_out_lo32 = bzfl->strm.total_out_lo32;
   if (nbytes_out_hi32 != NULL)
      *nbytes_out_hi32 = bzfl->strm.total_out_hi32;

   BZFL_SETERR(BZ_OK);
   BZ2_bzCompressEnd ( &(bzfl->strm) );
   free ( bzfl );
}


/*---------------------------------------------------*/
BZFILELIKE* BZ_API(BZ2_bzflReadOpen)
                   ( int*  bzerror,
                     void *stream, // e.g. FILE *
                     int   verbosity,
                     int   small,
                     void* unused,
                     int   nUnused,
                     struct filelike_callbacks *callbacks
                     ) {
  bzFileLike* bzfl = NULL;
  int     ret;

  BZFL_SETERR(BZ_OK);

  if (stream == NULL ||
      (small != 0 && small != 1) ||
      (verbosity < 0 || verbosity > 4) ||
      (unused == NULL && nUnused != 0) ||
      (unused != NULL && (nUnused < 0 || nUnused > BZ_MAX_UNUSED)))
    { BZFL_SETERR(BZ_PARAM_ERROR); return NULL; };

  if (callbacks->ferror(stream))
    { BZFL_SETERR(BZ_IO_ERROR); return NULL; };

   bzfl = calloc(1, sizeof(*bzfl));
   if (bzfl == NULL)
      { BZFL_SETERR(BZ_MEM_ERROR); return NULL; };

   bzfl->cb = *callbacks;

   BZFL_SETERR(BZ_OK);

   bzfl->initialisedOk = 0;
   bzfl->handle        = stream;
   bzfl->bufN          = 0;
   bzfl->writing       = 0;
   bzfl->strm.bzalloc  = NULL;
   bzfl->strm.bzfree   = NULL;
   bzfl->strm.opaque   = NULL;

   while (nUnused > 0) {
      bzfl->buf[bzfl->bufN] = *((unsigned char*)(unused)); bzfl->bufN++;
      unused = ((void*)( 1 + ((unsigned char*)(unused))  ));
      nUnused--;
   }

   ret = BZ2_bzDecompressInit ( &(bzfl->strm), verbosity, small );
   if (ret != BZ_OK)
      { BZFL_SETERR(ret); free(bzfl); return NULL; };

   bzfl->strm.avail_in = bzfl->bufN;
   bzfl->strm.next_in  = bzfl->buf;

   bzfl->initialisedOk = 1;
   return bzfl;
}


/*---------------------------------------------------*/
void BZ_API(BZ2_bzflReadClose) ( int *bzerror, BZFILELIKE *bzfl )
{
   BZFL_SETERR(BZ_OK);
   if (bzfl == NULL)
      { BZFL_SETERR(BZ_OK); return; };

   if (bzfl->writing)
      { BZFL_SETERR(BZ_SEQUENCE_ERROR); return; };

   if (bzfl->initialisedOk)
      (void)BZ2_bzDecompressEnd ( &(bzfl->strm) );
   free ( bzfl );
}

void BZ_API(BZ2_bzflReadGetUnused)
     ( int*    bzerror,
       BZFILELIKE* bzfl,
       void**  unused,
       int*    nUnused )
{
   if (bzfl == NULL)
      { BZFL_SETERR(BZ_PARAM_ERROR); return; };
   if (bzfl->lastErr != BZ_STREAM_END)
      { BZFL_SETERR(BZ_SEQUENCE_ERROR); return; };
   if (unused == NULL || nUnused == NULL)
      { BZFL_SETERR(BZ_PARAM_ERROR); return; };

   BZFL_SETERR(BZ_OK);
   *nUnused = bzfl->strm.avail_in;
   *unused = bzfl->strm.next_in;
}


/*---------------------------------------------*/
int BZ_API(bzflib_feof)( void *stream) {
  FILE* f = stream;
  int32_t c = fgetc ( f );
  if (c == EOF) return 1;
  ungetc ( c, f );
  return 0;
}

struct filelike_callbacks BZ2_filelike_callbacks_FILE() {
  struct filelike_callbacks c = filelike_callbacks_FILE();
  c.feof = bzflib_feof;
  return c;
}


/**
 * open file for read or write.
 * ex) bzopen("file","w9", filelike_fopen, filelike_fclose, bzflib_feof, ferror)
 * case path="" or NULL => use stdin or stdout.
 */
BZFILELIKE * BZ_API(BZ2_bzflopen)
     (const void *src,
      const char *mode,
      struct filelike_callbacks *callbacks,
      void *open_ctx // will be passed to callbacks->fopen
      ) {
  int    bzerr;
  char   unused[BZ_MAX_UNUSED];
  int    blockSize100k = 9;
  int    writing       = 0;
  char   mode2[10]     = "";
  void *stream = NULL;
  // FILE   *fp           = NULL;
  BZFILELIKE *bzfl         = NULL;
  int    verbosity     = 0;
  int    workFactor    = 30;
  int    smallMode     = 0;
  int    nUnused       = 0;

   if (mode == NULL) return NULL;
   while (*mode) {
     switch (*mode) {
     case 'r':
       writing = 0; break;
     case 'w':
       writing = 1; break;
     case 's':
       smallMode = 1; break;
     default:
       if (isdigit((int)(*mode))) {
         blockSize100k = *mode-BZ_HDR_0;
       }
     }
     mode++;
   }

   strcat(mode2, writing ? "wb" : "rb" );
   strcat (mode2, writing ? "e" : "e" );

   stream = callbacks->fopen(src, mode2, open_ctx);

   if (writing) {
      /* Guard against total chaos and anarchy -- JRS */
      if (blockSize100k < 1) blockSize100k = 1;
      if (blockSize100k > 9) blockSize100k = 9;
      bzfl = BZ2_bzflWriteOpen(&bzerr,stream,blockSize100k,
                               verbosity,workFactor, callbacks);
   } else {
     bzfl = BZ2_bzflReadOpen(&bzerr,stream,verbosity,smallMode,
                             unused,nUnused,
                             callbacks);
   }
   if (bzfl == NULL) {
     if(stream)
       callbacks->fclose(stream);
   }
   return bzfl;
}

/*---------------------------------------------------*/
size_t BZ_API(BZ2_bzflread)
     (void* buf, size_t size, size_t n, BZFILELIKE* bzfl) {
  int len = (int) size * n;
  if(size != 0 && len / size != n) { // overflow
    fprintf(stderr, "Overflow!\n");
    return 0;
  }

  int bzerr, nread;
  if(bzfl->lastErr == BZ_STREAM_END) return 0;
  nread = BZ2_bzflRead(&bzerr,bzfl,buf,len);
  if (bzerr == BZ_OK || bzerr == BZ_STREAM_END)
    return (size_t)nread;

  return 0;
}

/*---------------------------------------------------*/
size_t BZ_API(BZ2_bzflwrite) (void* buf, size_t size, size_t n, void *stream) {
  BZFILELIKE* bzfl = stream;
  int bzerr;
  int len = (int) size * n;
  if (size != 0 && len / size != n) { // overflow
    fprintf(stderr, "Overflow!\n");
    return 0;
  }
  BZ2_bzflWrite(&bzerr,bzfl,buf,len);
  if(bzerr == BZ_OK)
    return (size_t) len;

  return 0;
}


/*---------------------------------------------------*/
int BZ_API(BZ2_bzflflush) (BZFILELIKE *b) {
  return 0;
}

/*---------------------------------------------------*/
void BZ_API(BZ2_bzflclose) (BZFILELIKE* bzfl) {
   int bzerr;
   if(bzfl == NULL)
     return;

   void *stream = bzfl->handle;
   if(bzfl->writing) {
      BZ2_bzflWriteClose(&bzerr,bzfl,0,NULL,NULL);
      if(bzerr != BZ_OK){
         BZ2_bzflWriteClose(NULL,bzfl,1,NULL,NULL);
      }
   } else {
      BZ2_bzflReadClose(&bzerr,bzfl);
   }
   if(stream)
     bzfl->cb.fclose(stream);
}


/*---------------------------------------------------*/
/*--
   return last error code
--*/
static const char *bzerrorstrings[] = {
       "OK"
      ,"SEQUENCE_ERROR"
      ,"PARAM_ERROR"
      ,"MEM_ERROR"
      ,"DATA_ERROR"
      ,"DATA_ERROR_MAGIC"
      ,"IO_ERROR"
      ,"UNEXPECTED_EOF"
      ,"OUTBUFF_FULL"
      ,"CONFIG_ERROR"
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
      ,"???"   /* for future */
};


const char * BZ_API(BZ2_bzflerror) (BZFILELIKE *bzfl, int *errnum)
{
   int err = bzfl->lastErr;
   if(err>0) err = 0;
   *errnum = err;
   return bzerrorstrings[err*-1];
}

#ifdef BZFLIB_TEST
int main(int argc, const char *argv[]) {
  char mode = 0;
  if(argc > 1) {
    if(!strcmp(argv[1], "compress"))
      mode = 'c';
    else if(!strcmp(argv[1], "decompress"))
      mode = 'd';
  }
  if(!mode) {
    fprintf(stderr, "Usage: bzflib <compress|decompress>\n");
    return 0;
  }

  struct filelike_callbacks cb = filelike_callbacks_FILE();
  unsigned char buff[4096];
  size_t bytes;
  BZFILELIKE *bzfl;
  int err = 0;
  if(mode == 'c') {
    bzfl = BZ2_bzflopen(NULL, "w", &cb, NULL); // write to stdout
    if(bzfl) {
      while(!err && (bytes = fread(buff, 1, sizeof(buff), stdin)))
        if(!BZ2_bzflwrite(buff, 1, bytes, bzfl))
          // to do: print error
          break;
    }
  } else {
    bzfl = BZ2_bzflopen(NULL, "r", &cb, NULL); // read from stdin
    if(bzfl) {
      while(!err && (bytes = BZ2_bzflread(buff, 1, sizeof(buff), bzfl)))
        fwrite(buff, 1, bytes, stdout);
    }
  }

  if(bzfl)
    BZ2_bzflclose(bzfl);
  return err;
}

#endif
