
#ifndef MACROS__H
#define MACROS__H

/* Arguments given to macros or .IRP/.IRPC blocks */

#include "symbols.h"
#include "stream2.h"


struct ARG {
    ARG();
    ~ARG();
    ARG     *next;       /* Pointer in arg list */
    int      locsym;     /* Whether arg represents an optional local symbol */
    char    *label;      /* Argument name */
    char    *value;      /* Default or active substitution */
};

/* A MACRO is a superstructure surrounding a SYMBOL. */

struct MACRO : public SYMBOL {
    MACRO(char *label);
    ~MACRO();
    // SYMBOL   *sym;        /* Surrounds a symbol, contains the macro name */
    ARG      *args;       /* The argument list */
    BUFFER   *text;       /* The macro text */
};

struct MACRO_STREAM : public BUFFER_STREAM{
    // BUFFER_STREAM   bstr;       /* Base class: buffer stream */
    int       nargs;      /* Add number-of-macro-arguments */
    int       cond;       /* Add saved conditional stack */

    MACRO_STREAM(STREAM *refstr, BUFFER *buf, MACRO *mac, ARG *args);
    virtual ~MACRO_STREAM() override;

    char      *gets() override { return BUFFER_STREAM::gets(); };
    // void     _delete() override;
    void      rewind() override { BUFFER_STREAM::rewind(); };

} ;


#ifndef MACROS__C
// extern STREAM_VTBL macro_stream_vtbl;
#endif

// MACRO   *new_macro(char *label);
// void     free_macro(MACRO *mac);

MACRO   *defmacro(char *cp, STACK *stack, int called);

STREAM  *expandmacro(STREAM *refstr, MACRO *mac, char *cp);

// ARG  *new_arg(void);

void  read_body(STACK *stack, BUFFER *gb, char *name, int called);
void  eval_arg(STREAM *refstr, ARG *arg);
BUFFER   *subst_args(BUFFER *text, ARG *args);



#endif
