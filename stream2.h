#ifndef STREAM2_H
#define STREAM2_H

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

enum : int {
  TYPE_BASE_STREAM = 0,
  TYPE_FILE_STREAM,
  TYPE_BUFFER_STREAM,
  TYPE_REPT_STREAM,
  TYPE_IRP_STREAM,
  TYPE_IRPC_STREAM,
  TYPE_MACRO_STREAM
};

struct STREAM {
    STREAM(char *name);
    virtual ~STREAM();
    // virtual void            _delete ();    // Destructor
    virtual char           *gets    () { return NULL; };    // "gets" function
    virtual void            rewind  () {};                  // "rewind" function
    // STREAM_VTBL    *vtbl;       // Pointer to dispatch table
    char           *name;       // Stream name
    int             line;       // Current line number in stream
    int str_type;
    STREAM  *next;       // Next stream in stack
};

struct FILE_STREAM : public STREAM {
    // STREAM          stream;     // Base class
    FILE_STREAM();
    bool init(const char *filename);
    virtual ~FILE_STREAM() override;
    // virtual void            _delete () override;    // Destructor
    virtual char           *gets() override;    // "gets" function
    virtual void            rewind() override;    // "rewind" function
    FILE           *fp;         // File pointer
    char           *buffer;     // Line buffer
} ;

struct BUFFER {
// BUFFER         *new_buffer(void);
// BUFFER         *buffer_clone(BUFFER *from);
    BUFFER();
    BUFFER(int size);
    ~BUFFER();

    char           *buffer;     // Pointer to text
    int             size;       // Size of buffer
    int             length;     // Occupied size of buffer
    int             use;        // Number of users of buffer
    void            buffer_resize(int size);
    // void            buffer_free(BUFFER *buf);   
    void            buffer_appendn(char *str, int len);
    void            buffer_append_line(char *str);

};

#define GROWBUF_INCR 1024              // Buffers grow by leaps and bounds

struct BUFFER_STREAM : public STREAM{
    BUFFER_STREAM(BUFFER *buf,char *name);
    virtual ~BUFFER_STREAM() override;
    // STREAM          stream;     // Base class
    BUFFER         *buffer;     // text buffer
    int             offset;     // Current read offset
    // STREAM         *new_buffer_stream(BUFFER *buf, char *name);
    void            set_buffer(BUFFER *buf);

    char           *gets() override;
    // void            _delete() override;
    void            rewind() override;

};

struct STACK {
    STREAM         *top;        // Top of stacked stream pieces
    STACK () {};
    void            stack_init(STREAM *str = NULL);
    void            push(STREAM *str);
    void            pop();
    char           *gets();
};

#define STREAM_BUFFER_SIZE 1024        // This limits the max size of an input line.

void buffer_free(BUFFER *buf);

/* Provide these so that macro11 can derive from a BUFFER_STREAM */
// extern STREAM_VTBL buffer_stream_vtbl;
// void            buffer_stream_construct(BUFFER_STREAM * bstr, BUFFER *buf, char *name);
// char           *buffer_stream_gets(STREAM *str);
// void            buffer_stream_delete(STREAM *str);
// void            buffer_stream_rewind(STREAM *str);

// STREAM         *new_file_stream(char *filename);

// void            stack_init(STACK *stack);
// void            stack_push(STACK *stack, STREAM *str);
// void            stack_pop(STACK *stack);
// char           *stack_gets(STACK *stack);

#endif /* STREAM2_H */
