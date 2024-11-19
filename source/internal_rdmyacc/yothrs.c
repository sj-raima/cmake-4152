/* ----------------------------------------------------------------------------
 *
 * Raima Database Manager
 *
 * Copyright (c) 2010 Raima Inc.,  All rights reserved.
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the Raima LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 *
 * ----------------------------------------------------------------------------
 */

#include "dtxtrn.h"

#include <string.h>
#if !defined(RDM_WINDOWS) && !defined(_MSC_VER)
#include <unistd.h>
#endif

static void warray (const char *, const intptr_t *, int32_t);

/*lint -save -e956 */
extern char yaccpar[];
extern char startnm[];
extern char startnmuc[];
extern char yystype[];
extern char ctxtype[];
extern char ctxname[];

/*lint -restore */

static void warray (const char *s, const intptr_t *v, int32_t n)
{
    int32_t i;

    if (strcmp (s, "yyr1") == 0 || strcmp (s, "yyr2") == 0)
        yprintf (ftable, "static const uint16_t %s_%s[] =\n{", s, startnm);
    else
        yprintf (ftable, "static const int16_t %s_%s[] =\n{", s, startnm);

    for (i = 0; i < n;)
    {
        if (i % 10 == 0)
            yprintf (ftable, "\n\t");

        yprintf (ftable, "%d", (int32_t) v[i]);
        if (++i == n)
            yprintf (ftable, "\n};\n\n");
        else
            yprintf (ftable, ", ");
    }
}

/* ===================================================================
 Put out other arrays, copy the parsers
 */
void others ()
{
    int32_t c, i, j;

    warray ("yyr1", levprd, nprod);

    aryfil (temp1, nprod, 0);
    PLOOP (1, i) temp1[i] = (int32_t) ((prdptr[i + 1] - prdptr[i]) - 2);
    warray ("yyr2", temp1, nprod);

    aryfil (temp1, nstate, -1000);
    TLOOP (i)
    {
        for (j = tstates[i]; j != 0; j = mstates[j])
            temp1[j] = tokset[i].value;
    }

    NTLOOP (i)
    {
        for (j = ntstates[i]; j != 0; j = mstates[j])
            temp1[j] = -i;
    }

    warray ("yychk", temp1, nstate);
    warray ("yydef", defact, nstate);

    /* Output YYTABLES initialization */
    yprintf (ftable,
             "extern const RDM::BASE::YYTABLES %s; /* TBD: extern declarations should really be in a header file */\n", startnm);
    yprintf (ftable,
             "const RDM::BASE::YYTABLES %s =\n {\n"
             "\tYYERRCODE_%s, YYLAST_%s, yyexca_%s, yyact_%s, yypact_%s,\n"
             "\tyypgo_%s, yyr1_%s, yyr2_%s, yychk_%s, yydef_%s"
             "\n};\n\n",
             startnm, startnmuc, startnmuc, startnm, startnm, startnm, startnm, startnm, startnm, startnm, startnm);

    /*
     Output function declaration
     */
    yprintf (ftable, "/* =====================================================================\n"
                     "   RDMYacc semantic actions function\n"
                     "*/\n");
    yprintf (ftable,
             "RDM::PSP::RETURN_CODE %s (\n"
             "\tRDM::BASE::PYYCONTEXT yyctx,\n"
             "\tRDM::BASE::PYYSTYPE   yypvt_,\n"
             "\tint16_t               ruleno,\n"
             "\tRDM::BASE::PYYSTYPE   yyval_)\n"
             "{\n",
             yaccpar);

    yprintf (ftable, "\t%s *%s = (%s *) yyctx;\n", ctxtype, ctxname, ctxtype);
    yprintf (ftable, "\t%s *yyval = (%s *) yyval_;\n", yystype, yystype);
    yprintf (ftable, "\t%s *yypvt = (%s *) yypvt_; /*lint !e954 */\n\n", yystype, yystype);
    yprintf (ftable, "\tRDM::PSP::RETURN_CODE yyrc = sOKAY;\n");

    yprintf (ftable, "\tswitch (ruleno) {");

    /* Copy actions */
    faction = yfopen (ACTNAME, "r");
    if (faction == NULL)
    {
        error ("cannot reopen action tempfile", NULL);
    }

    while ((c = getc (faction)) != EOF)
    {
        putc (c, ftable);
    }

    fclose (faction);
    yunlink (ACTNAME);
    yprintf (ftable, "\t} /*lint !e744 */\n\n");

    yprintf (ftable, "\treturn yyrc;\n} /*lint !e954 */\n");

    fclose (ftable);
}
