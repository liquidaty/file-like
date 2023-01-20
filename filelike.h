#ifndef FILELIKE_H
#define FILELIKE_H

#include <stddef.h>

struct filelike_callbacks {
  void *(*fopen)(const void *src, const char *mode, void *ctx); // eg filelike_fopen()
  size_t (*fread)(void *ptr, size_t size, size_t n, void *stream);
  size_t (*fwrite)(void *ptr, size_t size, size_t n, void *stream);
  int (*fflush)(void *stream);  // e.g. fflush
  int (*fclose)(void *stream);  // e.g. filelike_fclose
  int (*feof) ( void *stream); // e.g. feof
  int (*ferror)(void *stream);  // e.g. ferror
};


int filelike_fclose(void *stream);

void *filelike_fopen(const void *src, const char *mode, void *opts);

struct filelike_callbacks filelike_callbacks_FILE();

#endif
