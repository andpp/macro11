#ifndef OBJECT_H
#define OBJECT_H

/* Object file constant definitions */

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

#define FBR_LEAD1 1                    /* The byte value that defines the
                                          beginning of a formatted binary
                                          record */
#define FBR_LEAD2 0                    /* Followed by a 0 */
/* Followed by two bytes length */
/* Followed by (length-4) bytes data */
/* Followed by a 1 byte checksum */
/* which is the negative sum of all
                       preceeding bytes */

#define OBJ_GSD 01                     /* GSD (Global symbol directory) */
#define OBJ_ENDGSD 02                  /* ENDGSD */
#define OBJ_TEXT 03                    /* TEXT */
#define OBJ_RLD 04                     /* RLD (Relocation directory) */
#define OBJ_ISD 05                     /* ISD (Internal symbol directory,
                                          currently unused) */
#define OBJ_ENDMOD 06                  /* ENDMOD (End of object module) */
#define OBJ_LIBHDR 07                  /* LIBHDR (Object Library header) */
#define OBJ_LIBEND 010                 /* LIBEND (Object Library header end) */

#define GSD_MODNAME 00                 /* Module name */
#define GSD_CSECT 01                   /* Control section name */
#define GSD_ISN 02                     /* Internal symbol name */
#define GSD_XFER 03                    /* Transfer address */
#define GSD_GLOBAL 04                  /* Global symbol definition/reference */
#define GSD_PSECT 05                   /* PSECT name */
#define GSD_IDENT 06                   /* IDENT */
#define GSD_VSECT 07                   /* VSECT (Virtual array declaration) */

#define GLOBAL_WEAK 01                 /* GLOBAL is weak, else strong */
#define GLOBAL_DEF 010                 /* GLOBAL is definition, else reference */
#define GLOBAL_REL 040                 /* GLOBAL is relative, else absolute */

#define PSECT_SAV 001                  /* PSECT is a root section, else overlay */
#define PSECT_COM 004                  /* PSECT is merged common area, else
                                          contatenated */
#define PSECT_RO 020                   /* PSECT is read-only, else R/W */
#define PSECT_REL 040                  /* PSECT is relative, else absolute
                                          (absolute implies PSECT_COM) */
#define PSECT_GBL 0100                 /* PSECT is overlay-global, else
                                          overlay-local */
#define PSECT_DATA 0200                /* PSECT contains data, else instructions */

#define RLD_INT 01                     /* "Internal relocation" */
#define RLD_GLOBAL 02                  /* "Global relocation" */
#define RLD_INT_DISP 03                /* "Internal displaced" */
#define RLD_GLOBAL_DISP 04             /* "Global displaced" */
#define RLD_GLOBAL_OFFSET 05           /* "Global additive" */
#define RLD_GLOBAL_OFFSET_DISP 06      /* "Global additive displaced" */
#define RLD_LOCDEF 07                  /* "Location counter definition" */
#define RLD_LOCMOD 010                 /* "Location counter modification" */
#define RLD_LIMITS 011                 /* ".LIMIT" */
#define RLD_PSECT 012                  /* "P-sect" */
#define RLD_PSECT_DISP 014             /* "P-sect displaced" */
#define RLD_PSECT_OFFSET 015           /* "P-sect additive" */
#define RLD_PSECT_OFFSET_DISP 016      /* "P-sect additive displaced" */
#define RLD_COMPLEX 017                /* "Complex" */

#define RLD_BYTE 0200                  /* RLD modifies a byte, else a word */

/* Note: complex relocation is not well documented (in particular, no effort
    is made to define a section's "sector number"), but I'll just guess
    it's a stack language. */

#define CPLX_NOP 00                    /* NOP - used for padding */
#define CPLX_ADD 01
#define CPLX_SUB 02
#define CPLX_MUL 03
#define CPLX_DIV 04
#define CPLX_AND 05
#define CPLX_OR 06
#define CPLX_XOR 07
#define CPLX_NEG 010
#define CPLX_COM 011
#define CPLX_STORE 012                 /* Store result, terminate complex string. */
#define CPLX_STORE_DISP 013            /* Store result PC-relative, terminate */
#define CPLX_GLOBAL 016                /* Followed by four bytes RAD50 global name */
#define CPLX_REL 017                   /* Followed by one byte "sector number" and two bytes offset */
#define CPLX_CONST 020                 /* Followed by two bytes constant value */

struct GSD {
    GSD(FILE *fp) : offset(0)  { gsd_init(fp); };
    GSD() : offset(0), fp(nullptr) {};
    ~GSD() {};
    FILE           *fp;         /* The file assigned for output */
    char            buf[1024*512];   /* space for 15 GSD entries */
    int             offset;     /* Current buffer for GSD entries */

    void  gsd_init(FILE *fp);
    int   gsd_flush();
    int   gsd_mod(char *modname);
    int   gsd_csect(char *sectname, int size);
    int   gsd_intname(char *name, int flags, unsigned value);
    int   gsd_xfer(char *name, unsigned value);
    int   gsd_global(char *name, int flags, unsigned value);
    int   gsd_psect(char *name, int flags, int size);
    int   gsd_ident(char *name);
    int   gsd_virt(char *name, int size);
    int   gsd_end();
private:
    int   gsd_write(char *name, int flags, int type, int value);
};

struct TEXT_RLD {
    TEXT_RLD(FILE *fp, unsigned addr): txt_offset(0), rld_offset(0)  {text_init(fp, addr); }
    TEXT_RLD() : fp(nullptr), txt_addr(0), txt_offset(0), rld_offset(0) {};
    ~TEXT_RLD() {};
    FILE           *fp;         /* The object file, or NULL */
    char            text[128*20];  /* text buffer */
    unsigned        txt_addr;   /* The base text address */
    int             txt_offset; /* Current text offset */
    char            rld[128*20];   /* RLD buffer */
    int             rld_offset; /* Current RLD offset */

    void  text_init(FILE *fp, unsigned addr);
    int   text_flush();
    int   text_word(unsigned *addr, int size, unsigned word);
    int   text_internal_word(unsigned *addr, int size, unsigned word);
    int   text_global_word(unsigned *addr, int size, unsigned word, char *global);
    int   text_displaced_word(unsigned *addr, int size, unsigned word);
    int   text_global_displaced_word(unsigned *addr, int size, unsigned word, char *global);
    int   text_global_offset_word(unsigned *addr, int size, unsigned word, char *global);
    int   text_global_displaced_offset_word(unsigned *addr, int size, unsigned word, char *global);
    int   text_define_location(char *name, unsigned *addr);
    int   text_modify_location(unsigned *addr);
    int   text_limits(unsigned *addr);
    int   text_psect_word(unsigned *addr, int size, unsigned word, char *name);
    int   text_psect_offset_word(unsigned *addr, int size, unsigned word, char *name);
    int   text_psect_displaced_word(unsigned *addr, int size, unsigned word, char *name);
    int   text_psect_displaced_offset_word(unsigned *addr, int size, unsigned word, char *name);
//private:
    int   text_fit(unsigned addr, int txtsize, int rldsize);
    void  rld_name(char *name);
    void  text_word_i(unsigned w, int size);
    void  rld_word(unsigned wd);
    void  rld_byte(unsigned byte);
    void  rld_code(unsigned code, unsigned addr, int size);
    void  rld_code_naddr(unsigned code, int size);
};

struct TEXT_COMPLEX {
    TEXT_COMPLEX(): len(0) {};
    char            accum[126*20];
    int             len;

    void  text_complex_begin();
    int   text_complex_add();
    int   text_complex_sub();
    int   text_complex_mul();
    int   text_complex_div();
    int   text_complex_and();
    int   text_complex_or();
    int   text_complex_xor();
    int   text_complex_com();
    int   text_complex_neg();
    int   text_complex_lit(unsigned word);
    int   text_complex_global(char *name);
    int   text_complex_psect(unsigned sect, unsigned offset);
//private:
    char *text_complex_fit(int size);
    int   text_complex_byte(unsigned byte);
};

int   text_complex_commit(TEXT_RLD *tr, unsigned *addr, int size, TEXT_COMPLEX *tx, unsigned word);
int   text_complex_commit_displaced(TEXT_RLD *tr, unsigned *addr, int size, TEXT_COMPLEX *tx, unsigned word);


int   write_endmod(FILE *fp);

#endif /* OBJECT_J */
