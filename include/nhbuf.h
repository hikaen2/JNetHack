/*
  $Id:$
 */

#ifndef NHBUF_H
#define NHBUF_H

#include <sys/types.h>

typedef struct _nhbuf{
    size_t	max_size;
    size_t	size;
    char	*data;
} NHBUF;

#endif
