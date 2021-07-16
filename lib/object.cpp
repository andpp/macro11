/*
  object.c - writes RT-11 compatible .OBJ files.

  Ref: RT-11 Software Support Manual, File Formats.
*/

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

#include "rad50.h"

#include "object.h"
#include "assemble_globals.h"

//#include "macro11.h"

/*
  writerec writes "formatted binary records."
  Each is preceeded by any number of 0 bytes, begins with a 1,0 pair,
  followed by 2 byte length, followed by data, followed by 1 byte
  negative checksum.
*/

static int writerec(FILE *fp, char *data, int len)
{
    int             chksum;     /* Checksum is negative sum of all
                                   bytes including header and length */
    int             i;
    unsigned        hdrlen = len + 4;

    if (fp == NULL)
        return 1;                      /* Silently ignore this attempt to write. */

    chksum = 0;
    if (fputc(FBR_LEAD1, fp) == EOF)   /* All recs begin with 1,0 */
        return 0;
    chksum -= FBR_LEAD1;
    if (fputc(FBR_LEAD2, fp) == EOF)
        return 0;
    chksum -= FBR_LEAD2;

    i = hdrlen & 0xff;                 /* length, lsb */
    chksum -= i;
    if (fputc(i, fp) == EOF)
        return 0;

    i = (hdrlen >> 8) & 0xff;          /* length, msb */
    chksum -= i;
    if (fputc(i, fp) == EOF)
        return 0;

    i = fwrite(data, 1, len, fp);
    if (i < len)
        return 0;

    while (len > 0) {                  /* All the data bytes */
        chksum -= *data++ & 0xff;
        len--;
    }

    chksum &= 0xff;

    fputc(chksum, fp);                 /* Followed by the checksum byte */

    return 1;                          /* Worked okay. */
}

/* gsd_init - prepare a GSD prior to writing GSD records */

void GSD::gsd_init(FILE *_fp)
{
    fp = _fp;
    buf[0] = OBJ_GSD;             /* GSD records start with 1,0 */
    buf[1] = 0;
    offset = 2;                   /* Offset for further additions */
}

/* gsd_flush - write buffered GSD records */

int GSD::gsd_flush()
{
    if (offset > 2) {
        if (!writerec(fp, buf, offset))
            return 0;
        gsd_init(fp);
    }
    return 1;
}

/* gsd_write - buffers a GSD record */

/* All GSD entries have the following 8 byte format: */
/* 4 bytes RAD50 name */
/* 1 byte flags */
/* 1 byte type */
/* 2 bytes value */

int GSD::gsd_write(char *name, int flags, int type, int value)
{
    char           *cp;
    unsigned        radtbl[2];

    int nlen = disable_rad50_symbols ? 1 + 2 + strlen(name) : 4;

    if (offset > sizeof(buf) - nlen - 4) {
        if (!gsd_flush())
            return 0;
    }

    cp = buf + offset;

    if(disable_rad50_symbols) {
        *cp++ = 0xff; *cp++ = 0xff;
        offset +=2;
        while(*name) {
            *cp++ = *name++;
            offset++;
        }
        *cp++=0;
        offset++;

        *cp++ = flags;
        *cp++ = type;

        *cp++ = value & 0xff;
        *cp = (value >> 8) & 0xff;

        offset += 4;
    } else {
        rad50x2(name, radtbl);

        *cp++ = radtbl[0] & 0xff;
        *cp++ = (radtbl[0] >> 8) & 0xff;
        *cp++ = radtbl[1] & 0xff;
        *cp++ = (radtbl[1] >> 8) & 0xff;

        *cp++ = flags;
        *cp++ = type;

        *cp++ = value & 0xff;
        *cp = (value >> 8) & 0xff;

        offset += 8;
    }

    return 1;
}

/* gsd_mod - Write module name to GSD */

int GSD::gsd_mod(char *modname)
{
    return gsd_write(modname, 0, GSD_MODNAME, 0);
}

/* gsd_csect - Write a control section name & size to the GSD */
int GSD::gsd_csect(char *sectname, int size)
{
    return gsd_write(sectname, 0, GSD_CSECT, size);
}

/* gsd_intname - Write an internal symbol (ignored by RT-11 linker) */
int GSD::gsd_intname(char *name, int flags,  unsigned value)
{
    return gsd_write(name, flags, GSD_ISN, value);
}

/* gsd_xfer - Write a program transfer address to GSD */
int GSD::gsd_xfer(char *name, unsigned value)
{
    return gsd_write(name, 010, GSD_XFER, value);
}

/* gsd_global - Write a global definition or reference to GSD */
/* Caller must be aware of the proper flags. */
int GSD::gsd_global(char *name, int flags, unsigned value)
{
    return gsd_write(name, flags, GSD_GLOBAL, value);
}

/* Write a program section to the GSD */
/* Caller must be aware of the proper flags. */
int GSD::gsd_psect(char *name, int flags, int size)
{
    return gsd_write(name, flags, GSD_PSECT, size);
}

/* Write program ident to GSD */
int GSD::gsd_ident(char *name)
{
    return gsd_write(name, 0, GSD_IDENT, 0);
}

/* Write virtual array declaration to GSD */
int GSD::gsd_virt(char *name, int size)
{
    return gsd_write(name, 0, GSD_VSECT, size);
}

/* Write ENDGSD record */

int GSD::gsd_end()
{
    buf[0] = OBJ_ENDGSD;
    buf[1] = 0;
    return writerec(fp, buf, 2);
}

/* TEXT and RLD record handling */

/* TEXT records contain the plain binary of the program.  An RLD
   record refers to the prior TEXT record, giving relocation
   information. */

/* text_init prepares a TEXT_RLD prior to writing */

void TEXT_RLD::text_init(FILE *_fp, unsigned addr)
{
    fp = _fp;

    text[0] = OBJ_TEXT;            /* text records begin with 3, 0 */
    text[1] = 0;
    text[2] = addr & 0xff;         /* and are followed by load address */
    text[3] = (addr >> 8) & 0xff;
    txt_offset = 4;                /* Here's where recording new text will begin */

    rld[0] = OBJ_RLD;              /* RLD records begin with 4, 0 */
    rld[1] = 0;

    txt_addr = addr;
    rld_offset = 2;                /* And are followed by RLD entries */
}

/* text_flush - flushes buffer TEXT and RLD records. */

int TEXT_RLD::text_flush()
{
    if (txt_offset > 4) {
        if (!writerec(fp, text, txt_offset))
            return 0;
    }

    if (rld_offset > 2) {
        if (!writerec(fp, rld, rld_offset))
            return 0;
    }

    return 1;
}

/* Used to ensure that TEXT and RLD information will be in adjacent
   records.  If not enough space exists in either buffer, both are
   flushed. */

int TEXT_RLD::text_fit(unsigned addr, int txtsize, int rldsize)
{
    if (txt_offset + txtsize <= sizeof(text) && rld_offset + rldsize <= sizeof(rld)
        && (txtsize == 0 || txt_addr + txt_offset - 4 == addr))
        return 1;                      /* All's well. */

    if (!text_flush())
        return 0;
    text_init(fp, addr);

    return 1;
}

/* rld_word - adds a word to the RLD information. */

void TEXT_RLD::rld_name(char *name)
{
    rld[rld_offset++] = 0xff; rld[rld_offset++] = 0xff;
    while(*name) {
        rld[rld_offset++] = *name++;
    }
    rld[rld_offset++] = 0;
}


/* text_word_i - internal text_word.  Used when buffer space is
   already assured. */

void TEXT_RLD::text_word_i(unsigned w, int size)
{
    text[txt_offset++] = w & 0xff;
    if (size > 1)
        text[txt_offset++] = (w >> 8) & 0xff;
}

/* text_word - write constant word to text */

int TEXT_RLD::text_word(unsigned *addr, int size, unsigned word)
{
    if (!text_fit(*addr, size, 0))
        return 0;

    text_word_i(word, size);

    *addr += size;                     /* Update the caller's DOT */
    return 1;                          /* say "ok". */
}

/* rld_word - adds a word to the RLD information. */

void TEXT_RLD::rld_word(unsigned wd)
{
    rld[rld_offset++] = wd & 0xff;
    rld[rld_offset++] = (wd >> 8) & 0xff;
}

/* rld_byte - adds a byte to rld information. */

void TEXT_RLD::rld_byte(unsigned byte)
{
    rld[rld_offset++] = byte & 0xff;
}

/* rld_code - write the typical RLD first-word code.  Encodes the
   given address as the offset into the prior TEXT record. */

void TEXT_RLD::rld_code(unsigned code, unsigned addr, int size)
{
    unsigned offset = addr - txt_addr + 4;

    rld_word(code | offset << 8 | (size == 1 ? 0200 : 0));
}

/* rld_code_naddr - typical RLD entries refer to a text address.  This
   one is used when the RLD code does not. */

void TEXT_RLD::rld_code_naddr(unsigned code, int size)
{
    rld_word(code | (size == 1 ? 0200 : 0));
}

/* write a word with a psect-relative value */

int TEXT_RLD::text_internal_word(unsigned *addr, int size, unsigned word)
{
    if (!text_fit(*addr, size, 4))
        return 0;

    text_word_i(word, size);
    rld_code(RLD_INT, *addr, size);
    rld_word(word);

    *addr += size;

    return 1;
}

/* write a word which is an absolute reference to a global symbol */

int TEXT_RLD::text_global_word(unsigned *addr, int size, unsigned word, char *global)
{
    unsigned        radtbl[2];

    if (!text_fit(*addr, size, 6))
        return 0;

    text_word_i(word, size);
    rld_code(RLD_GLOBAL, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(global);
    } else {
        rad50x2(global, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }

    *addr += size;

    return 1;
}

/* Write a word which is a PC-relative reference to an absolute address */

int TEXT_RLD::text_displaced_word(unsigned *addr, int size, unsigned word)
{
    if (!text_fit(*addr, size, 4))
        return 0;

    text_word_i(word, size);
    rld_code(RLD_INT_DISP, *addr, size);
    rld_word(word);

    *addr += size;

    return 1;
}

/* write a word which is a PC-relative reference to a global symbol */

int TEXT_RLD::text_global_displaced_word(unsigned *addr, int size, unsigned word, char *global)
{
    unsigned        radtbl[2];

    if (!text_fit(*addr, size, 6))
        return 0;

    text_word_i(word, size);
    rld_code(RLD_GLOBAL_DISP, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(global);
    } else {
        rad50x2(global, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }

    *addr += size;

    return 1;
}

/* write a word which is an absolute reference to a global symbol plus
   an offset */

/* Optimizes to text_global_word when the offset is zero. */

int TEXT_RLD::text_global_offset_word(unsigned *addr, int size, unsigned word, char *global)
{
    unsigned        radtbl[2];

    if (word == 0)
        return text_global_word(addr, size, word, global);

    if (!text_fit(*addr, size, 8))
        return 0;

    text_word_i(word, size);

    rld_code(RLD_GLOBAL_OFFSET, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(global);
    } else {
        rad50x2(global, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }
    rld_word(word);

    *addr += size;

    return 1;
}

/* write a word which is a PC-relative reference to a global symbol
   plus an offset */

/* Optimizes to text_global_displaced_word when the offset is zero. */

int TEXT_RLD::text_global_displaced_offset_word(unsigned *addr, int size, unsigned word, char *global)
{
    unsigned        radtbl[2];

    if (word == 0)
        return text_global_displaced_word(addr, size, word, global);

    if (!text_fit(*addr, size, 8))
        return 0;

    text_word_i(word, size);
    rld_code(RLD_GLOBAL_OFFSET_DISP, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(global);
    } else {
        rad50x2(global, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }
    rld_word(word);

    *addr += size;

    return 1;
}

/* Define current program counter, plus PSECT */
/* Different because it must be the last RLD entry in a block.  That's
   because TEXT records themselves contain the current text
   address. */

int TEXT_RLD::text_define_location(char *name, unsigned *addr)
{
    unsigned        radtbl[2];

    if (!text_fit(*addr, 0, 8))    /* No text space used */
        return 0;

    rld_code_naddr(RLD_LOCDEF, 2); /* RLD code for "location
                                          counter def" with no offset */

    /* Set current section name */
    if(disable_rad50_symbols) {
        rld_name(name);
    } else {
        rad50x2(name, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }

    rld_word(*addr);               /* Set current location addr */

    if (!text_flush())               /* Flush that block out. */
        return 0;

    text_init(fp, *addr);      /* Set new text address */

    return 1;
}

/* Modify current program counter, assuming current PSECT */
/* Location counter modification is similarly weird */
/* (I wonder - why is this RLD code even here?  TEXT records contain
   thair own start address.) */

int TEXT_RLD::text_modify_location(unsigned *addr)
{
    if (!text_fit(*addr, 0, 4))    /* No text space used */
        return 0;

    rld_code_naddr(RLD_LOCMOD, 2); /* RLD code for "location
                                          counter mod" with no offset */
    rld_word(*addr);               /* Set current location addr */

    if (!text_flush())               /* Flush that block out. */
        return 0;
    text_init(fp, *addr);      /* Set new text address */

    return 1;
}

/* write two words containing program limits (the .LIMIT directive) */

int TEXT_RLD::text_limits(unsigned *addr)
{
    if (!text_fit(*addr, 4, 2))
        return 0;

    text_word_i(0, 2);
    text_word_i(0, 2);
    rld_code(RLD_LIMITS, *addr, 2);

    *addr += 4;

    return 1;
}

/* write a word which is the start address of a different PSECT */

int TEXT_RLD::text_psect_word(unsigned *addr, int size, unsigned word, char *name)
{
    unsigned        radtbl[2];

    if (!text_fit(*addr, size, 6))
        return 0;

    text_word_i(word, size);

    rld_code(RLD_PSECT, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(name);
    } else {
        rad50x2(name, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }

    *addr += size;

    return 1;
}

/* write a word which is an offset from the start of a different PSECT */

/* Optimizes to text_psect_word when offset is zero */

int TEXT_RLD::text_psect_offset_word(unsigned *addr, int size, unsigned word, char *name)
{
    unsigned        radtbl[2];

    if (word == 0)
        return text_psect_word(addr, size, word, name);

    if (!text_fit(*addr, size, 8))
        return 0;

    text_word_i(word, size);

    rld_code(RLD_PSECT_OFFSET, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(name);
    } else {
        rad50x2(name, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }
    rld_word(word);

    *addr += size;

    return 1;
}

/* write a word which is the address of a different PSECT, PC-relative */

int TEXT_RLD::text_psect_displaced_word(unsigned *addr, int size, unsigned word, char *name)
{
    unsigned        radtbl[2];

    if (!text_fit(*addr, size, 6))
        return 0;

    text_word_i(word, size);

    rld_code(RLD_PSECT_DISP, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(name);
    } else {
        rad50x2(name, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }

    *addr += size;

    return 1;
}

/* write a word which is an offset from the address of a different
   PSECT, PC-relative */

/* Optimizes to text_psect_displaced_word when offset is zero */

int TEXT_RLD::text_psect_displaced_offset_word(unsigned *addr, int size, unsigned word, char *name)
{
    unsigned        radtbl[2];

    if (word == 0)
        return text_psect_displaced_word(addr, size, word, name);

    if (!text_fit(*addr, size, 8))
        return 0;

    text_word_i(word, size);

    rld_code(RLD_PSECT_OFFSET_DISP, *addr, size);

    if(disable_rad50_symbols) {
        rld_name(name);
    } else {
        rad50x2(name, radtbl);
        rld_word(radtbl[0]);
        rld_word(radtbl[1]);
    }
    rld_word(word);

    *addr += size;

    return 1;
}

/* complex relocation! */

/* A complex relocation expression is where a piece of code is fed to
   the linker asking it to do some math for you, and store the result
   in a program word. The code is a stack-based language. */

/* complex_begin initializes a TEXT_COMPLEX */

void TEXT_COMPLEX::text_complex_begin()
{
    len = 0;
}

/* text_complex_fit checks if a complex expression will fit and
   returns a pointer to it's location */

char *TEXT_COMPLEX::text_complex_fit(int size)
{
    unsigned   _len;

    if (len + size > sizeof(accum))
        return NULL;                   /* Expression has grown too complex. */

    _len = len;
    len += size;

    return accum + _len;
}

/* text_complex_byte stores a single byte. */

int TEXT_COMPLEX::text_complex_byte(unsigned byte)
{
    char           *cp = text_complex_fit(1);

    if (!cp)
        return 0;
    *cp = byte;
    return 1;
}

/* text_complex_add - add top two stack elements */

int TEXT_COMPLEX::text_complex_add()
{
    return text_complex_byte(CPLX_ADD);
}

/* text_complex_sub - subtract top two stack elements. */
/* You know, I think these function labels are self-explanatory... */

int TEXT_COMPLEX::text_complex_sub()
{
    return text_complex_byte(CPLX_SUB);
}

int TEXT_COMPLEX::text_complex_mul()
{
    return text_complex_byte(CPLX_MUL);
}

int TEXT_COMPLEX::text_complex_div()
{
    return text_complex_byte(CPLX_DIV);
}

int TEXT_COMPLEX::text_complex_and()
{
    return text_complex_byte(CPLX_AND);
}

int TEXT_COMPLEX::text_complex_or()
{
    return text_complex_byte(CPLX_OR);
}

int TEXT_COMPLEX::text_complex_xor()
{
    return text_complex_byte(CPLX_XOR);
}

int TEXT_COMPLEX::text_complex_com()
{
    return text_complex_byte(CPLX_COM);
}

int TEXT_COMPLEX::text_complex_neg()
{
    return text_complex_byte(CPLX_NEG);
}

/* text_complex_lit pushes a literal value to the stack. */

int TEXT_COMPLEX::text_complex_lit(unsigned word)
{
    char           *cp = text_complex_fit(3);

    if (!cp)
        return 0;
    *cp++ = CPLX_CONST;
    *cp++ = word & 0xff;
    *cp = (word >> 8) & 0xff;
    return 1;
}

/* text_complex_global pushes the value of a global variable to the
   stack */

int TEXT_COMPLEX::text_complex_global(char *name)
{
    unsigned        radtbl[2];
    int size = disable_rad50_symbols ? strlen(name) + 2 + 1 + 1 : 5;
    char           *cp = text_complex_fit(size);

    if (!cp)
        return 0;

    if(disable_rad50_symbols) {
        *cp++ = CPLX_GLOBAL;
        *cp++ = 0xff; *cp++ = 0xff;
        while(*name) {
            *cp++ = *name++;
        }
        *cp = 0;
    } else {
        rad50x2(name, radtbl);
        *cp++ = CPLX_GLOBAL;
        *cp++ = radtbl[0] & 0xff;
        *cp++ = (radtbl[0] >> 8) & 0xff;
        *cp++ = radtbl[1] & 0xff;
        *cp = (radtbl[1] >> 8) & 0xff;
    }
    return 1;
}

/* text_complex_psect pushes the value of an offset into a PSECT to
   the stack. */

/* What was not documented in the Software Support manual is that
   PSECT "sect" numbers are assigned in the order they appear in the
   source program, and the order they appear in the GSD.  i.e. the
   first PSECT GSD is assigned sector 0 (which is always the default
   absolute section so that's a bad example), the next sector 1,
   etc. */

int TEXT_COMPLEX::text_complex_psect(unsigned sect, unsigned offset)
{
    char           *cp = text_complex_fit(4);

    if (!cp)
        return 0;
    *cp++ = CPLX_REL;
    *cp++ = sect & 0xff;
    *cp++ = offset & 0xff;
    *cp = (offset >> 8) & 0xff;
    return 1;
}

/* text_complex_commit - store the result of the complex expression
   and end the RLD code. */

int text_complex_commit(TEXT_RLD *tr, unsigned *addr, int size, TEXT_COMPLEX *tx, unsigned word)
{
    int             i;

    tx->text_complex_byte(CPLX_STORE);

    if (!tr->text_fit(*addr, size, tx->len + 2))
        return 0;

    tr->rld_code(RLD_COMPLEX, *addr, size);

    for (i = 0; i < tx->len; i++)
        tr->rld_byte(tx->accum[i]);

    tr->text_word_i(word, size);

    *addr += size;

    return 1;
}

/* text_complex_commit_displaced - store the result of the complex
   expression, relative to the current PC, and end the RLD code */

int text_complex_commit_displaced(TEXT_RLD *tr, unsigned *addr, int size, TEXT_COMPLEX *tx, unsigned word)
{
    int             i;

    tx->text_complex_byte(CPLX_STORE_DISP);

    if (!tr->text_fit(*addr, size, tx->len + 2))
        return 0;

    tr->rld_code(RLD_COMPLEX, *addr, size);

    for (i = 0; i < tx->len; i++)
        tr->rld_byte(tx->accum[i]);

    tr->text_word_i(word, size);

    *addr += size;

    return 1;
}

/* Write end-of-object-module to file. */

int write_endmod(FILE *fp)
{
    char            endmod[2] = {
        OBJ_ENDMOD, 0
    };
    return writerec(fp, endmod, 2);
}
