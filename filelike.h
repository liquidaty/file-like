#ifndef FILELIKE_H
#define FILELIKE_H

#include <stddef.h>

struct filelike_callbacks {
  /*
   * fopen is optional; if provided, it is called by XXXflopen to obtain
   * the input stream. if it is not provided, the input stream is assumed
   * to be the source pointer and callbacks->fclose() will be ignored
   * If you would like to use fopen here, maybe filelike_fopen() instead
   */
  void *(*fopen)(const void *src, const char *mode, void *ctx);

  /*
   * fclose is optional; if provided, it is called by XXXflclose
   * If you would like to use fclose here, maybe use filelike_fclose instead
   */
  int (*fclose)(void *stream);

  /*
   * fread, fwrite, fflush, feof and ferror are required
   * function signature should resemble the same from stdio
   */
  size_t (*fread)(void *ptr, size_t size, size_t n, void *stream);
  size_t (*fwrite)(void *ptr, size_t size, size_t n, void *stream);
  int (*fflush)(void *stream);
  int (*feof) ( void *stream);
  int (*ferror)(void *stream);
};


int filelike_fclose(void *stream);

void *filelike_fopen(const void *src, const char *mode, void *opts);

struct filelike_callbacks filelike_callbacks_FILENAME();

struct filelike_callbacks filelike_callbacks_FILEPTR();

#endif
