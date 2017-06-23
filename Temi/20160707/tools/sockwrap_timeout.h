/*
 * Sviluppato da Enrico Masala <masala@polito.it> , Mar 2016
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "errlib.h"
#include "sockwrap.h"

#ifndef _SOCKWRAP_TIMEOUT_H

#define _SOCKWRAP_TIMEOUT_H

ssize_t readn_timeout (int fd, void *vptr, size_t n,  struct timeval *p_tv);
ssize_t readline_unbuffered_timeout (int fd, void *vptr, size_t maxlen, struct timeval *p_tv);

#endif

