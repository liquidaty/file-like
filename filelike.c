#include <stdio.h>
#include <string.h>
#include "filelike.h"

// filelike_fclose: replacement for fclose()
int filelike_fclose(void *stream) {
  if (stream && stream != stdin && stream != stdout) return fclose(stream);
  return 0;
}

void *filelike_fopen(const void *src, const char *mode, void *opts) {
  (void)(opts);
  const char *path = src;
  FILE *fp;
  if (path==NULL || strcmp(path,"")==0) {
    char writing = strchr(mode, 'w') ? 1 : 0;
    fp = (writing ? stdout : stdin);
#if defined(_WIN32) || defined(OS2) || defined(MSDOS)
#   include <fcntl.h>
#   include <io.h>
    setmode(fileno(fp),O_BINARY);
#endif
  } else {
    fp = fopen(path, mode);
  }
  return fp;
}

struct filelike_callbacks filelike_callbacks_FILENAME() {
  struct filelike_callbacks c = { 0 };
  c.fopen = filelike_fopen;
  c.fread = (size_t(*)(void *, size_t, size_t, void *))fread;
  c.fwrite = (size_t(*)(void *, size_t, size_t, void *))fwrite;
  c.fflush = (int (*)(void *))fflush;
  c.fclose = filelike_fclose;
  c.feof = (int (*)(void *))feof;
  c.ferror = (int (*)(void *))ferror;
  return c;
}

struct filelike_callbacks filelike_callbacks_FILEPTR() {
  struct filelike_callbacks c = filelike_callbacks_FILENAME();
  c.fopen = NULL;
  c.fclose = NULL;
  return c;
}
