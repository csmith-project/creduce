/*
 * Copyright (c) 2016 The University of Utah
 * Copyright (c) 2012 - 2013 Tony Finch <dot@dotat.at>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/stat.h>

#include <ctype.h>
#ifdef _WIN32
#define errx(code, ...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(code); } while(0)
#define err errx
#define warnx(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define warn warnx
#define vwarnx(fmt, args) do { vfprintf(stderr, fmt, args); fprintf(stderr, "\n"); } while(0)
#else
#include <err.h>
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* portability stubs */

#define fbinmode(fp) (fp)

#define replace(old,new) rename(old,new)

static FILE *
mktempmode(char *tmp, int mode)
{
	int fd = mkstemp(tmp);
	if (fd < 0) return (NULL);
#ifndef _WIN32
	fchmod(fd, mode & (S_IRWXU|S_IRWXG|S_IRWXO));
#endif
	return (fdopen(fd, "wb"));
}
