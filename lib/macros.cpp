
#define MACROS__C


/*
 Dealing with MACROs
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "macros.h"                    /* my own definitions */

#include "util.h"
#include "assemble_globals.h"
#include "assemble_aux.h"
#include "listing.h"
#include "parse.h"
#include "stream2.h"
#include "symbols.h"


/* Here's where I pretend I'm a C++ compiler.  :-/ */

/* *** derive a MACRO_STREAM from a BUFFER_STREAM with a few other args */


/* macro_stream_delete is called when a macro expansion is
   exhausted.  The unique behavior is to unwind any stacked
   conditionals.  This allows a nested .MEXIT to work.  */

MACRO_STREAM::~MACRO_STREAM()
{
    // MACRO_STREAM   *mstr = (MACRO_STREAM *) str;

    pop_cond(cond);
}

// STREAM_VTBL     macro_stream_vtbl = {
//     macro_stream_delete, buffer_stream_gets, buffer_stream_rewind
// };

MACRO_STREAM::MACRO_STREAM(STREAM *refstr, BUFFER *buf, MACRO *mac, ARG *args) : BUFFER_STREAM(buf, ""), nargs(0), cond(0)
{
    str_type = TYPE_MACRO_STREAM;

    free(name);
    name = (char *)memcheck(malloc(strlen(refstr->name) + 32));
    sprintf(name, "%s:%d->%s", refstr->name, refstr->line, mac->label);

    // mstr->bstr.stream.vtbl = &macro_stream_vtbl;
    /* Count the args and save their number */
    for (nargs = 0; args; args = args->next, nargs++) ;
    cond = last_cond;
}

/* read_body fetches the body of .MACRO, .REPT, .IRP, or .IRPC into a
   BUFFER. */

void read_body(
    STACK *stack,
    BUFFER *gb,
    char *name,
    int called)
{
    int             nest;

    /* Read the stream in until the end marker is hit */

    /* Note: "called" says that this body is being pulled from a macro
       library, and so under no circumstance should it be listed. */

    nest = 1;
    for (;;) {
        SYMBOL         *op;
        char           *nextline;
        char           *cp;

        nextline = stack->gets();  /* Now read the line */
        if (nextline == NULL) {        /* End of file. */
            report(stack->top, "Macro body not closed\n");
            break;
        }

        if (!called && (list_level - 1 + list_md) > 0) {
            list_flush();
            list_source(stack->top, nextline);
        }

        op = get_op(nextline, &cp);

        if (op == NULL) {              /* Not a pseudo-op */
            gb->buffer_append_line(nextline);
            continue;
        }
        if (op->section->type == SECTION_PSEUDO) {
            if (op->value == P_MACRO || op->value == P_REPT || op->value == P_IRP || op->value == P_IRPC)
                nest++;

            if (op->value == P_ENDM || op->value == P_ENDR) {
                nest--;
                /* If there's a name on the .ENDM, then */
                /* close the body early if it matches the definition */
                if (name && op->value == P_ENDM) {
                    cp = skipwhite(cp);
                    if (!EOL(*cp)) {
                        char           *label = get_symbol(cp, &cp, NULL);

                        if (label) {
                            if (strcmp(label, name) == 0)
                                nest = 0;       /* End of macro body. */
                            free(label);
                        }
                    }
                }
            }

            if (nest == 0)
                return;                /* All done. */
        }

        gb->buffer_append_line(nextline);
    }
}

/* Diagnostic: dumpmacro dumps a macro definition to stdout.
   I used this for debugging; it's not called at all right now, but
   I hate to delete good code. */

void dumpmacro(MACRO *mac, FILE *fp)
{
    ARG            *arg;

    fprintf(fp, ".MACRO %s ", mac->label);

    for (arg = mac->args; arg != NULL; arg = arg->next) {
        fputs(arg->label, fp);
        if (arg->value) {
            fputc('=', fp);
            fputs(arg->value, fp);
        }
        fputc(' ', fp);
    }
    fputc('\n', fp);

    fputs(mac->text->buffer, fp);

    fputs(".ENDM\n", fp);
}

/* defmacro - define a macro. */
/* Also used by .MCALL to pull macro definitions from macro libraries */

MACRO          *defmacro(
    char *cp,
    STACK *stack,
    int called)
{
    MACRO          *mac;
    ARG            *arg,
                  **argtail;
    char           *label;

    cp = skipwhite(cp);
    label = get_symbol(cp, &cp, NULL);
    if (label == NULL) {
        report(stack->top, "Invalid macro definition\n");
        return NULL;
    }

    /* Allow redefinition of a macro; new definition replaces the old. */
    mac = (MACRO *) Glb_macro_st.lookup_sym(label);
    if (mac) {
        /* Remove from the symbol table... */
        Glb_macro_st.remove_sym(mac);
       delete (mac);
    }

    mac = new MACRO(label);

    // macro_st.add_table(mac->sym);
    Glb_macro_st.add_table(mac);

    argtail = &mac->args;
    cp = skipdelim(cp);

    while (!EOL(*cp)) {
        arg = new ARG();
        arg->locsym = *cp == '?';
        if (arg->locsym) /* special argument flag? */
            cp++;
        arg->label = get_symbol(cp, &cp, NULL);
        if (arg->label == NULL) {
            /* It turns out that I have code which is badly formatted
               but which MACRO.SAV assembles.  Sigh.  */
            /* So, just quit defining arguments. */
            break;
#if 0
            report(str, "Illegal macro argument\n");
            remove_sym(&mac->sym, &macro_st);
            delete (mac);
            return NULL;
#endif
        }

        cp = skipwhite(cp);
        if (*cp == '=') {
            /* Default substitution given */
            arg->value = getstring(cp + 1, &cp);
            if (arg->value == NULL) {
                report(stack->top, "Illegal macro argument\n");
                Glb_macro_st.remove_sym(mac);
                delete (mac);
                return NULL;
            }
        }

        /* Append to list of arguments */
        arg->next = NULL;
        *argtail = arg;
        argtail = &arg->next;

        cp = skipdelim(cp);
    }

    /* Read the stream in until the end marker is hit */  {
        BUFFER         *gb;
        int             levelmod = 0;

        gb = new BUFFER();

        if (!called && !list_md) {
            list_level--;
            levelmod = 1;
        }

        read_body(stack, gb, mac->label, called);

        list_level += levelmod;

        if (mac->text != NULL)         /* Discard old macro body */
            buffer_free(mac->text);

        mac->text = gb;
    }

    return mac;
}



/* Allocate a new ARG */

ARG::ARG(void)
{
    // ARG            *arg = (ARG *)memcheck(malloc(sizeof(ARG)));

    locsym = 0;
    value = NULL;
    next = NULL;
    label = NULL;
    // return arg;
}

ARG::~ARG()
{
    if (label) {
        free(label);
    }
    if (value) {
        free(value);
    }

}


/* Free a list of args (as for a macro, or a macro expansion) */

static void free_args(ARG *arg)
{
    ARG            *next;

    while (arg) {
        next = arg->next;
        delete (arg);
        arg = next;
    }
}


/* find_arg - looks for an arg with the given name in the given
   argument list */

static ARG *find_arg(ARG *arg, char *name)
{
    for (; arg != NULL; arg = arg->next)
        if (strcmp(arg->label, name) == 0)
            return arg;

    return NULL;
}

/* subst_args - given a BUFFER and a list of args, generate a new
   BUFFER with argument replacement having taken place. */

BUFFER *subst_args(BUFFER *text, ARG *args)
{
    char           *in;
    char           *begin;
    BUFFER         *gb;
    char           *label;
    ARG            *arg;

    gb = new BUFFER();

    /* Blindly look for argument symbols in the input. */
    /* Don't worry about quotes or comments. */

    for (begin = in = text->buffer; in < text->buffer + text->length;) {
        char           *next;

        if (issym(*in)) {
            label = get_symbol(in, &next, NULL);
            if (label) {
                if ((arg = find_arg(args, label))) {
                    /* An apostrophe may appear before or after the symbol. */
                    /* In either case, remove it from the expansion. */

                    if (in > begin && in[-1] == '\'')
                        in--;          /* Don't copy it. */
                    if (*next == '\'')
                        next++;

                    /* Copy prior characters */
                    gb->buffer_appendn(begin, (int) (in - begin));
                    /* Copy replacement string */
                    gb->buffer_append_line(arg->value);
                    in = begin = next;
                    --in;              /* prepare for subsequent increment */
                }
                free(label);
                in = next;
            } else
                in++;
        } else
            in++;
    }

    /* Append the rest of the text */
    gb->buffer_appendn(begin, (int) (in - begin));

    return gb;                         /* Done. */
}

/* eval_arg - the language allows an argument expression to be given
   as "\expression" which means, evaluate the expression and
   substitute the numeric value in the current radix. */

void eval_arg(
    STREAM *refstr,
    ARG *arg)
{
    /* Check for value substitution */

    if (arg->value[0] == '\\') {
        EX_TREE        *value = parse_expr(arg->value + 1, 0);
        unsigned        word = 0;
        char            temp[10];

        if (value->type != EX_LIT) {
            report(refstr, "Constant value required\n");
        } else
            word = value->data.lit;

        delete (value);

        /* printf can't do base 2. */
        my_ultoa(word & 0177777, temp, radix);
        free(arg->value);
        arg->value = (char *)memcheck(strdup(temp));
    }
}

/* expandmacro - return a STREAM containing the expansion of a macro */

STREAM         *expandmacro(
    STREAM *refstr,
    MACRO *mac,
    char *cp)
{
    ARG            *arg,
                   *args,
                   *macarg;
    char           *label;
    MACRO_STREAM         *str;
    BUFFER         *buf;

    args = NULL;
    arg = NULL;

    /* Parse the arguments */

    while (!EOL(*cp)) {
        char           *nextcp;

        /* Check for named argument */
        label = get_symbol(cp, &nextcp, NULL);
        if (label && (nextcp = skipwhite(nextcp), *nextcp == '=') && (macarg = find_arg(mac->args, label))) {
            /* Check if I've already got a value for it */
            if (find_arg(args, label) != NULL) {
                report(refstr, "Duplicate submission of keyword " "argument %s\n", label);
                free(label);
                free_args(args);
                return NULL;
            }

            arg = new ARG();
            arg->label = label;
            nextcp = skipwhite(nextcp + 1);
            arg->value = getstring(nextcp, &nextcp);
        } else {
            if (label)
                free(label);

            /* Find correct positional argument */

            for (macarg = mac->args; macarg != NULL; macarg = macarg->next) {
                if (find_arg(args, macarg->label) == NULL)
                    break;             /* This is the next positional arg */
            }

            if (macarg == NULL)
                break;                 /* Don't pick up any more arguments. */

            arg = new ARG();
            arg->label = (char *)memcheck(strdup(macarg->label));       /* Copy the name */
            arg->value = getstring(cp, &nextcp);
        }

        arg->next = args;
        args = arg;

        eval_arg(refstr, arg);         /* Check for expression evaluation */

        cp = skipdelim(nextcp);
    }

    /* Now go back and fill in defaults */  {
        int             locsym;

        if (last_lsb != lsb)
            locsym = last_locsym = 32768;
        else
            locsym = last_locsym;
        last_lsb = lsb;

        for (macarg = mac->args; macarg != NULL; macarg = macarg->next) {
            arg = find_arg(args, macarg->label);
            if (arg == NULL) {
                arg = new ARG();
                arg->label = (char *)memcheck(strdup(macarg->label));
                if (macarg->locsym) {
                    char            temp[32];

                    /* Here's where we generate local labels */
                    sprintf(temp, "%d$", locsym++);
                    arg->value = (char *)memcheck(strdup(temp));
                } else if (macarg->value) {
                    arg->value = (char *)memcheck(strdup(macarg->value));
                } else
                    arg->value = (char *)memcheck(strdup(""));

                arg->next = args;
                args = arg;
            }
        }

        last_locsym = locsym;
    }

    buf = subst_args(mac->text, args);

    str = new MACRO_STREAM(refstr, buf, mac, args);

    free_args(args);
    buffer_free(buf);

    return str;
}


/* dump_all_macros is a diagnostic function that's currently not
   used.  I used it while debugging, and I haven't removed it. */
#ifdef DEBUG
static void dump_all_macros(
    void)
{
    MACRO          *mac;
    SYMBOL_ITER     iter;

    for (mac = (MACRO *) first_sym(&macro_st, &iter); mac != NULL; mac = (MACRO *) next_sym(&macro_st, &iter)) {
        dumpmacro(mac, lstfile);

        printf("\n\n");
    }
}
#endif

/* Allocate a new macro */

MACRO::MACRO(char *label) : SYMBOL(label)
{
    flags = 0;
    // sym.label = label;
    stmtno = stmtno;
    next = NULL;
    section = &macro_section;
    value = 0;
    args = NULL;
    text = NULL;
}

/* free a macro, it's args, it's text, etc. */
MACRO::~MACRO()
{
    if (text) {
        free(text);
    }
    free_args(args);
    // delete (sym);
}
