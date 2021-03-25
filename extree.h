
#ifndef EXTREE__H
#define EXTREE__H

#include "symbols.h"

    enum ex_type { EX_LIT = 1,
        /* Expression is a literal value */
        EX_SYM = 2,
        /* Expression has a symbol reference */
        EX_UNDEFINED_SYM = 3,
        /* Expression is undefined sym reference */
        EX_TEMP_SYM = 4,
        /* Expression is temp sym reference */

        EX_COM = 5,
        /* One's complement */
        EX_NEG = 6,
        /* Negate */
        EX_ERR = 7,
        /* Expression with an error */

        EX_ADD = 8,
        /* Add */
        EX_SUB = 9,
        /* Subtract */
        EX_MUL = 10,
        /* Multiply */
        EX_DIV = 11,
        /* Divide */
        EX_AND = 12,
        /* bitwise and */
        EX_OR = 13                     /* bitwise or */
    };

class EX_TREE;

// EX_TREE        *new_ex_tree()(void);
// EX_TREE        *new_ex_lit(unsigned value);
EX_TREE        *ex_err(EX_TREE *tp, char *cp);
// EX_TREE        *evaluate(EX_TREE *tp, int undef);


class EX_TREE {
  public:
    EX_TREE() {};
    EX_TREE(unsigned value) {
        type = EX_LIT;
        data.lit = value;
    }
    EX_TREE(const char *label, SECTION *section, unsigned value);

    ~EX_TREE(); //{ free_tree(); };
    enum ex_type type;

    char           *cp;         /* points to end of parsed expression */

    union {
        struct {
            EX_TREE *left,
                    *right;      /* Left, right children */
        } child;
        unsigned        lit;    /* Literal value */
        SYMBOL         *symbol; /* Symbol reference */
    } data;

    // void            free_tree();
    // EX_TREE        *new_ex_lit(unsigned value);
    EX_TREE        *ex_err(char *cp) { return ::ex_err(this, cp); };
    EX_TREE        *evaluate(int undef);
} ;

#endif
