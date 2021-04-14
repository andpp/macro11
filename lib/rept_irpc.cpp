#define REPT_IRPC__C

/*
        .REPT
        .IRPC streams
*/

#include <stdlib.h>
#include <string.h>

#include "rept_irpc.h"                 /* my own definitions */

#include "util.h"
#include "assemble_aux.h"
#include "parse.h"
#include "listing.h"
#include "macros.h"
#include "assemble_globals.h"


/* *** implement REPT_STREAM */

struct REPT_STREAM : BUFFER_STREAM {
    REPT_STREAM(BUFFER *buf, char *name) : BUFFER_STREAM(buf, name), count(0), savecond(0) { str_type = TYPE_REPT_STREAM; };
    virtual ~REPT_STREAM() override;
    // BUFFER_STREAM   bstr;
    int     count;      /* The current repeat countdown */
    int     savecond;   /* conditional stack level at time of
                                   expansion */
    char    *gets() override;
    // void   _delete() override;
    // void    rewind() override;

};

/* rept_stream_gets gets a line from a repeat stream.  At the end of
   each count, the coutdown is decreated and the stream is reset to
   it's beginning. */

char *REPT_STREAM::gets()
{
    char  *cp;

    for (;;) {
        if ((cp = BUFFER_STREAM::gets()) != NULL)
            return cp;

        if (--count <= 0)
            return NULL;

        rewind();
    }
}

/* rept_stream_delete unwinds nested conditionals like .MEXIT does. */

// void rept_stream_delete(STREAM *str)
REPT_STREAM::~REPT_STREAM()
{
    // REPT_STREAM    *rstr = (REPT_STREAM *) str;

    pop_cond(savecond);          /* complete unterminated  conditionals */
}

/* The VTBL */

// STREAM_VTBL     rept_stream_vtbl = {
//     rept_stream_delete, rept_stream_gets, buffer_stream_rewind
// };

/* expand_rept is called when a .REPT is encountered in the input. */

REPT_STREAM   *expand_rept(
    STACK *stack,
    char *cp)
{
    EX_TREE        *value;
    BUFFER         *gb;
    REPT_STREAM    *rstr;
    int             levelmod;

    value = parse_expr(cp, 0);
    if (value->type != EX_LIT) {
        report(stack->top, ".REPT value must be constant\n");
        delete (value);
        return NULL;
    }

    gb = new BUFFER();

    levelmod = 0;
    if (!list_md) {
        list_level--;
        levelmod = 1;
    }

    read_body(stack, gb, NULL, FALSE);

    list_level += levelmod;

    char           *name = (char *)memcheck(malloc(strlen(stack->top->name) + 32));

    sprintf(name, "%s:%d->.REPT", stack->top->name, stack->top->line);
    rstr = new REPT_STREAM(gb, name);
    free(name);
    

    rstr->count = value->data.lit;
    // rstr->bstr.stream.vtbl = &rept_stream_vtbl;
    rstr->savecond = last_cond;

    buffer_free(gb);
    delete (value);

    return rstr;
}

/* *** implement IRP_STREAM */

struct IRP_STREAM : public BUFFER_STREAM{
    IRP_STREAM(BUFFER *buf, char *name) : BUFFER_STREAM(buf, name), offset(0), body(0), savecond(0) { str_type = TYPE_IRP_STREAM; };
    virtual ~IRP_STREAM() override;
    // BUFFER_STREAM   bstr;
    char           *label;      /* The substitution label */
    char           *items;      /* The substitution items (in source code
                                   format) */
    int             offset;     /* Current offset into "items" */
    BUFFER         *body;       /* Original body */
    int             savecond;   /* Saved conditional level */

    char           *gets() override;
    // void            _delete() override;
    // void            rewind() override;

};

/* irp_stream_gets expands the IRP as the stream is read. */
/* Each time an iteration is exhausted, the next iteration is
   generated. */

char  *IRP_STREAM::gets()
{
    char           *cp;
    BUFFER         *buf;
    ARG            *arg;

    for (;;) {
        if ((cp = BUFFER_STREAM::gets()) != NULL)
            return cp;

        cp = items + offset;

        if (!*cp)
            return NULL;               /* No more items.  EOF. */

        arg = new ARG();
        arg->next = NULL;
        arg->locsym = 0;
        arg->label = strdup(label);
        arg->value = getstring(cp, &cp);
        cp = skipdelim(cp);
        offset = (int) (cp - items);

        eval_arg(this, arg);
        buf = subst_args(body, arg);

        // free(arg->value);
        delete (arg);
        set_buffer(buf);
        buffer_free(buf);
    }
}

/* irp_stream_delete - also pops the conditional stack */

IRP_STREAM::~IRP_STREAM()
{
    // IRP_STREAM     *istr = (IRP_STREAM *) str;

    pop_cond(savecond);          /* complete unterminated conditionals */

    buffer_free(body);
    free(items);
    free(label);
}

// STREAM_VTBL     irp_stream_vtbl = {
//     irp_stream_delete, irp_stream_gets, buffer_stream_rewind
// };

/* expand_irp is called when a .IRP is encountered in the input. */

IRP_STREAM  *expand_irp(STACK *stack, char *cp)
{
    char           *label,
                   *items;
    BUFFER         *gb;
    int             levelmod = 0;
    IRP_STREAM     *str;

    label = get_symbol(cp, &cp, NULL);
    if (!label) {
        report(stack->top, "Illegal .IRP syntax\n");
        return NULL;
    }

    cp = skipdelim(cp);

    items = getstring(cp, &cp);
    if (!items) {
        report(stack->top, "Illegal .IRP syntax\n");
        free(label);
        return NULL;
    }

    gb = new BUFFER();

    levelmod = 0;
    if (!list_md) {
        list_level--;
        levelmod++;
    }

    read_body(stack, gb, NULL, FALSE);

    list_level += levelmod;

    char           *name = (char *)memcheck(malloc(strlen(stack->top->name) + 32));

    sprintf(name, "%s:%d->.IRP", stack->top->name, stack->top->line);
    str = new IRP_STREAM(NULL, name);
    free(name);

    // str->bstr.stream.vtbl = &irp_stream_vtbl;

    str->body = gb;
    str->items = items;
    str->offset = 0;
    str->label = label;
    str->savecond = last_cond;

    return str;
}


/* *** implement IRPC_STREAM */

struct IRPC_STREAM : public BUFFER_STREAM {
    IRPC_STREAM(BUFFER *buf, char *name) : BUFFER_STREAM(buf, name), offset(0), body(0), savecond(0) { str_type = TYPE_IRPC_STREAM; };
    virtual ~IRPC_STREAM() override;
// BUFFER_STREAM   bstr;
    char           *label;      /* The substitution label */
    char           *items;      /* The substitution items (in source code
                                   format) */
    int             offset;     /* Current offset in "items" */
    BUFFER         *body;       /* Original body */
    int             savecond;   /* conditional stack at invocation */

    char           *gets() override;
    // void            _delete() override;
    // void            rewind() override;
};

/* irpc_stream_gets - same comments apply as with irp_stream_gets, but
   the substitution is character-by-character */

char           *IRPC_STREAM::gets()
{
    // IRPC_STREAM    *istr = (IRPC_STREAM *) str;
    char           *cp;
    BUFFER         *buf;
    ARG            *arg;

    for (;;) {
        if ((cp = BUFFER_STREAM::gets()) != NULL)
            return cp;

        cp = items + offset;

        if (!*cp)
            return NULL;               /* No more items.  EOF. */

        arg = new ARG();
        arg->next = NULL;
        arg->locsym = 0;
        arg->label = strdup(label);
        arg->value = (char *)memcheck(malloc(2));
        arg->value[0] = *cp++;
        arg->value[1] = 0;
        offset = (int) (cp - items);

        buf = subst_args(body, arg);

        // free(arg->value);
        delete (arg);
        set_buffer(buf);
        buffer_free(buf);
    }
}

/* irpc_stream_delete - also pops contidionals */

IRPC_STREAM::~IRPC_STREAM()
{
    // IRPC_STREAM    *istr = (IRPC_STREAM *) str;

    pop_cond(savecond);          /* complete unterminated  conditionals */
    buffer_free(body);
    free(items);
    free(label);
}

// STREAM_VTBL     irpc_stream_vtbl = {
//     irpc_stream_delete, irpc_stream_gets, buffer_stream_rewind
// };

/* expand_irpc - called when .IRPC is encountered in the input */

IRPC_STREAM *expand_irpc(STACK *stack, char *cp)
{
    char           *label,
                   *items;
    BUFFER         *gb;
    int             levelmod = 0;
    IRPC_STREAM    *str;

    label = get_symbol(cp, &cp, NULL);
    if (!label) {
        report(stack->top, "Illegal .IRPC syntax\n");
        return NULL;
    }

    cp = skipdelim(cp);

    items = getstring(cp, &cp);
    if (!items) {
        report(stack->top, "Illegal .IRPC syntax\n");
        free(label);
        return NULL;
    }

    gb = new BUFFER();

    levelmod = 0;
    if (!list_md) {
        list_level--;
        levelmod++;
    }

    read_body(stack, gb, NULL, FALSE);

    list_level += levelmod;

    // str = (IRPC_STREAM *)memcheck(malloc(sizeof(IRPC_STREAM))); {
        char           *name = (char *)memcheck(malloc(strlen(stack->top->name) + 32));

        sprintf(name, "%s:%d->.IRPC", stack->top->name, stack->top->line);
        str = new IRPC_STREAM(NULL, name);
        free(name);
    // }

    // str->bstr.stream.vtbl = &irpc_stream_vtbl;
    str->body = gb;
    str->items = items;
    str->offset = 0;
    str->label = label;
    str->savecond = last_cond;

    return str;
}
