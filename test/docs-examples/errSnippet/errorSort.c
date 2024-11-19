#include "errorSort.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CTX *errInit(
    unsigned int itemCnt)
{
    CTX *ctx;

    ctx = (CTX *) malloc(sizeof(struct _ctx));
    if (ctx == NULL)
        return NULL;

    ctx->tabSize = 0;
    ctx->errIndex = 0;
    ctx->errTable = (ERRTABLE *) malloc(sizeof(ERRTABLE) * itemCnt);
    if (ctx->errTable == NULL)
    {
        free(ctx);
        return NULL;
    }

    return ctx;
}

void errTerm(
    CTX *ctx)
{
    free(ctx->errTable);
    free(ctx);
}

void errInsert(
    CTX        *ctx,
    const char *name,
    int         err,
    const char *state,
    const char *msg)
{
    if (state == NULL)
    {
        state = " ";
    }

    strcpy( (&ctx->errTable[ctx->errIndex])->name, name );
    (&ctx->errTable[ctx->errIndex])->code = err;
    strcpy( (&ctx->errTable[ctx->errIndex])->sqlstate, state );
    strcpy( (&ctx->errTable[ctx->errIndex])->msg, msg );
    ctx->tabSize = (++ctx->errIndex);
}

static int errSortValue(
    const void *l,
    const void *r)
{
    const ERRTABLE *LVal = (const ERRTABLE *) l;
    const ERRTABLE *RVal = (const ERRTABLE *) r;

    if ( LVal->code < RVal->code )
        return -1;

    if ( LVal->code > RVal->code )
        return 1;

    return 0;
}

void errSortByValue(
    CTX *ctx)
{
    qsort( ctx->errTable, ctx->tabSize, ELEMENT_SIZE, errSortValue);
    ctx->errIndex = (unsigned int) -1;
}

static int errSortName(
    const void *l,
    const void *r)
{
    const ERRTABLE *LVal = (const ERRTABLE *) l;
    const ERRTABLE *RVal = (const ERRTABLE *) r;

    return strcmp( LVal->name, RVal->name );
}

void errSortByName(
    CTX *ctx)
{
    qsort( ctx->errTable, ctx->tabSize, ELEMENT_SIZE, errSortName);
    ctx->errIndex = (unsigned int) -1;
}

void errPosLast(
    CTX *ctx)
{
    ctx->errIndex = ctx->tabSize;
}

int errGetNext(
    CTX         *ctx,
    const char **name,
    int         *code,
    const char **state,
    const char **msg)
{
    ctx->errIndex++;
    if (ctx->errIndex >= ctx->tabSize)
        return -1;

    *name =  (&ctx->errTable[ctx->errIndex])->name;
    *code =  (&ctx->errTable[ctx->errIndex])->code;
    *state = (&ctx->errTable[ctx->errIndex])->sqlstate;
    *msg =   (&ctx->errTable[ctx->errIndex])->msg;

    return 0;
}

int errGetPrev(
    CTX         *ctx,
    const char **name,
    int         *code,
    const char **state,
    const char **msg)
{
    ctx->errIndex--;
    if (ctx->errIndex >= ctx->tabSize)
        return -1;

    *name =  (&ctx->errTable[ctx->errIndex])->name;
    *code =  (&ctx->errTable[ctx->errIndex])->code;
    *state = (&ctx->errTable[ctx->errIndex])->sqlstate;
    *msg =   (&ctx->errTable[ctx->errIndex])->msg;

    return 0;
}

int boldOn = 0; /*lint !e956 */
int italicOn = 0; /*lint !e956 */

void fputConvert(
    FILE       *outfp,
    const char *inp)
{
    const char *p;

    while ((p = strpbrk(inp, "<>&\"/")) != NULL)
    {
        int inIncr = 1;
        if (p != inp)
        {
            fwrite(inp, 1, (uintptr_t) p - (uintptr_t) inp, outfp);
        }
        inp = p;

        switch (*p)
        {
            case '<': p = "&lt;"; break;
            case '>': p = "&gt;"; break;
            case '&': p = "&amp;"; break;
            case '"': p = "&quot;"; break;
            case '/':
                /* look for fontface */
                if ( strncmp(p, BOLDON, BOLDON_SIZE) == 0) {
                    p = "";
                    inIncr = BOLDON_SIZE;
                    if (!boldOn) {
                        p = "<b>";
                        boldOn = 1;
                    }
                }
                else if ( strncmp(p, ITALICON, ITALICON_SIZE) == 0) {
                    p = "";
                    inIncr = BOLDON_SIZE;
                    if (!italicOn) {
                        p = "<i>";
                        italicOn = 1;
                    }
                }
                else if ( strncmp(p, BOLDOFF, BOLDOFF_SIZE) == 0) {
                    p = "";
                    inIncr = BOLDOFF_SIZE;
                    if (boldOn) {
                        p = "</b>";
                        boldOn = 0;
                    }
                }
                else if ( strncmp(p, ITALICOFF, ITALICOFF_SIZE) == 0) {
                    p = "";
                    inIncr = ITALICOFF_SIZE;
                    if (italicOn) {
                        p = "</i>";
                        italicOn = 0;
                    }
                }
                else
                    p = "/";
                break;
        } /*lint !e744 */

        fprintf(outfp, "%s", p);
        inp += inIncr;

    }

    fprintf(outfp, "%s", inp);
}

