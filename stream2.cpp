/* functions for managing a stack of file and buffer input streams. */

/*

Copyright (c) 2001, Richard Krehbiel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

o Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

o Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

o Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "util.h"

#include "stream2.h"
#include "listing.h"

/* BUFFER functions */

/* new_buffer allocates a new buffer */

BUFFER::BUFFER()
{
    // BUFFER         *buf = (BUFFER *)memcheck(malloc(sizeof(BUFFER)));

    length = 0;
    size = 0;
    use = 1;
    buffer = NULL;
    // return buf;
}

BUFFER::BUFFER(int _size)
{
    size = _size;
    length = _size;
    use = 1;

    if (size == 0) {
        buffer = NULL;
    } else {
        buffer = (char *)memcheck(malloc(size));
    }

}

BUFFER::~BUFFER()
{
    if(buffer)
       free(buffer);
}


/* buffer_resize makes the buffer at least the requested size. */
/* If the buffer is already larger, then it will attempt */
/* to shrink it. */

void BUFFER::buffer_resize(int _size)
{
    size = _size;
    length = _size;

    if (size == 0) {
        free(buffer);
        buffer = NULL;
    } else {
        if (buffer == NULL)
            buffer = (char *)memcheck(malloc(size));
        else
            buffer = (char *)memcheck(realloc(buffer, size));
    }
}

/* buffer_clone makes a copy of a buffer */
/* Basically it increases the use count */

BUFFER *buffer_clone(BUFFER *from)
{
    if (from)
        from->use++;
    return from;
}

/* buffer_free frees a buffer */
/* It decreases the use count, and if zero, */
/* frees the memory. */

void buffer_free(BUFFER *buf)
{
    if (buf) {
        if (--(buf->use) == 0) {
            // free(buf->buffer);
            delete (buf);
        }
    }
}

/* Append characters to the buffer. */

void BUFFER::buffer_appendn(char *str, int len)
{
    int needed = length + len + 1;

    if (needed >= size) {
        size = needed + GROWBUF_INCR;

        if (buffer == NULL)
            buffer = (char *)memcheck(malloc(size));
        else
            buffer = (char *)memcheck(realloc(buffer, size));
    }

    memcpy(buffer + length, str, len);
    length += len;
    buffer[length] = 0;
}

/* append a text line (zero or newline-delimited) */

void BUFFER::buffer_append_line(char *str)
{
    char  *nl;

    if ((nl = strchr(str, '\n')) != NULL)
        buffer_appendn(str, (int) (nl - str + 1));
    else
        buffer_appendn(str, strlen(str));
}

/* Base STREAM class methods */

/* stream_construct initializes a newly allocated STREAM */

STREAM::STREAM(char *_name): str_type(TYPE_BASE_STREAM)
{
    line = 0;
    name = (char *)memcheck(strdup(_name));
    next = NULL;
}

/* stream_delete destroys and deletes (frees) a STREAM */

STREAM::~STREAM()
{
    free(name);
}

/* *** class BUFFER_STREAM implementation */

/* STREAM::gets for a buffer stream */

char * BUFFER_STREAM::gets()
{
    char           *nl;
    char           *cp;
    BUFFER         *buf = buffer;

    if (buf == NULL)
        return NULL;                   /* No buffer */

    if (offset >= buf->length)
        return NULL;

    cp = buf->buffer + offset;

    /* Find the next line in preparation for the next call */

    nl = (char *)memchr(cp, '\n', buf->length - offset);

    if (nl)
        nl++;

    offset = (int) (nl - buf->buffer);
    line++;

    return cp;
}

/* STREAM::close for a buffer stream */

BUFFER_STREAM::~BUFFER_STREAM()
{
    buffer_free(buffer);
}

/* STREAM::rewind for a buffer stream */

void BUFFER_STREAM::rewind()
{
    offset = 0;
    line = 0;
}

/* BUFFER_STREAM vtbl */

// STREAM_VTBL     buffer_stream_vtbl = {
//     buffer_stream_delete, buffer_stream_gets, buffer_stream_rewind
// };

BUFFER_STREAM::BUFFER_STREAM(BUFFER *buf,char *name): STREAM(name)
{
    // name = (char *)memcheck(strdup(name));
    str_type = TYPE_BUFFER_STREAM;
    buffer = buffer_clone(buf);
    offset = 0;
    line = 0;
}

void BUFFER_STREAM::set_buffer(BUFFER *buf)
{
    if (buffer)
        buffer_free(buffer);
    buffer = buffer_clone(buf);
    offset = 0;
}

/* new_buffer_stream clones the given buffer, gives it the name, */
/* and creates a BUFFER_STREAM to reference it */

// BUFFER_STREAM::BUFFER_STREAM(BUFFER *buf, char *name)
// {
//     buffer_stream_construct(bstr, buf, name);
// }

/* *** FILE_STREAM implementation */

/* Implement STREAM::gets for a file stream */

char    *FILE_STREAM::gets()
{
    int             i,
                    c;
    if (fp == NULL)
        return NULL;

    if (feof(fp))
        return NULL;

    /* Read single characters, end of line when '\n' or '\f' hit */

    i = 0;
    while (c = fgetc(fp), c != '\n' && c != '\f' && c != EOF) {
        if (c == 0)
            continue;                  /* Don't buffer zeros */
        if (c == '\r')
            continue;                  /* Don't buffer carriage returns either */
        if (i < STREAM_BUFFER_SIZE - 2)
            buffer[i++] = c;
    }

    buffer[i++] = '\n';          /* Silently transform formfeeds
                                          into newlines */
    buffer[i] = 0;

    if (c == '\n')
        line++;           /* Count a line */

    return buffer;
}

/* Implement STREAM::destroy for a file stream */

FILE_STREAM::~FILE_STREAM()
{
    fclose(fp);
    delete (buffer);
}

/* Implement STREAM::rewind for a file stream */

void FILE_STREAM::rewind()
{
    ::rewind(fp);
    line = 0;
}

// static STREAM_VTBL file_stream_vtbl = {
//     file_destroy, file_gets, file_rewind
// };

/* Prepare and open a stream from a file. */

FILE_STREAM::FILE_STREAM() : STREAM("")
{

}

bool FILE_STREAM::init(const char *filename)
{
    str_type = TYPE_FILE_STREAM;
    fp = fopen(filename, "r");
    if(fp == NULL)
        return false;

    // str = (FILE_STREAM *)memcheck(malloc(sizeof(FILE_STREAM)));

    // str->stream.vtbl = &file_stream_vtbl;
    name = (char *)memcheck(strdup(filename));
    buffer = (char *)memcheck(malloc(STREAM_BUFFER_SIZE));
    line = 0;
    return true;

    // return &str->stream;
}

/* STACK functions */

/* stack_init prepares a stack */

void STACK::stack_init(STREAM *str)
{
    top = str;                 /* Too simple */
}

/* stack_pop removes and deletes the topmost STRAM on the stack */

void STACK::pop()
{
    STREAM         *_top = top;

    top = top->next;
    delete (_top);
}

/* stack_push pushes a STREAM onto the top of the stack */

void STACK::push(STREAM *str)
{
    str->next = top;
    top = str;
}

/* stack_gets calls vtbl->gets for the topmost stack entry.  When
   topmost streams indicate they're exhausted, they are popped and
   deleted, until the stack is exhausted. */

char *STACK::gets()
{
    char           *line;

    if (top == NULL)
        return NULL;

    while ((line = top->gets()) == NULL) {
        pop();
        if (top == NULL)
            return NULL;
    }

    return line;
}
