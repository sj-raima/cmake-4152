/* ----------------------------------------------------------------------------
 * Raima Database Manager
 *
 * Copyright (c) 2010 Raima Inc.,  All rights reserved.
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the Raima LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 * ----------------------------------------------------------------------------
 */
#include "dtxtrn.h"

#include <string.h>
#include <ctype.h>
#define PSP_FILE_NAMELEN 2048

/*lint -e801 */

/*
 * ysetup.c  -- RDMYacc Setup & initialization module
 *
 */

static int32_t CloseInclude (void);
static void OpenInclude (const char *);
static int32_t chfind (int32_t, const char *);
static void cpyact (int32_t);
static void cpycode (void);
static void cpyunion (void);
static const char *cstash (const char *s);
static int32_t defin (int32_t, const char *); /*lint -esym(534,defin) */
static void defout (void);
static int32_t fdtype (intptr_t);
static int32_t getcid (void);
static int32_t gettok (void);
static int32_t skipcom (void);
static void usage (void);
static void yyparse (void);

typedef struct infile_stack
{
    struct infile_stack *next;
    int32_t lineno;
    char name[FNAMESIZE];
    FILE *fp;
} INFILE_STACK;

/*lint -save -e956 */
static INFILE_STACK *filesp = NULL; /* top of file stack */

#define TABS 4

static int32_t i;
static int32_t ty;
static int32_t lev;
static int32_t linsw;
static int32_t start; /* start symbol */
static int32_t action = 0;
static int32_t rulesw;
static int32_t ylines;
static int32_t ntypes = 0; /* number of types defined */
static int32_t extval = 0;
static int32_t allacts;
static int32_t numbval;     /* value of an input number */
static int32_t ndefout = 3; /* number of defined symbols output */
static int32_t in_rules = 0;
static int32_t peekline; /* from gettok() */
static int32_t lbrace_indent = 0;
static char ytab_h[NAMESIZE] = FILED;
static char cnames[CNAMSZ];    /* place where token and nonterminal
                                names are stored */
static char tokname[NAMESIZE]; /* input token name */
static char ruletext[NAMESIZE + 2];     /* text of current rule */
static char filename[FNAMESIZE];
static char *cnamp = cnames;        /* place where next name is to be put in */
static const char *typeset[NTYPES]; /* pointers to type tags */
static FILE *frules;                /* file for saving rules */

int32_t tabs = TABS;
int32_t lineno = 1; /* current input line number */
char *infile;       /* input file name */
char yaccpar[NAMESIZE];
char startnm[NAMESIZE];
char startnmuc[NAMESIZE];
char ctxtype[NAMESIZE] = "PARSE_STATE";
char ctxname[NAMESIZE] = "psp";
FILE *finput;  /* yacc input file */
FILE *faction; /* file for saving actions */
FILE *fdefine; /* file for # defines */
FILE *ftable;  /* y.tab.c file */
FILE *ftemp;   /* tempfile to pass 2 */
FILE *foutput; /* y.output file */
char yystype[NAMESIZE] = "YYSTYPE";
/*lint -restore */

static const char *fix_filename (const char *fInput)
{
    static char fname[FNAMESIZE];
    char *p = &fname[0];

    strcpy (p, fInput);
    while ((p = strchr (p, '\\')) != NULL)
        *p = '/';

    return fname;
}

static int32_t chfind (int32_t tt, const char *s)
{
    int ii;

    if (s[0] == ' ')
        tt = 0;

    TLOOP (ii)
    {
        if (!strcmp (s, tokset[ii].name))
            return ii;
    }

    NTLOOP (ii)
    {
        if (!strcmp (s, nontrst[ii].name))
            return ii + NTBASE;
    }

    /* cannot find name */
    if (tt > 1)
        error ("%s should have been defined earlier", s);

    return defin (tt, s);
}

/* =========================================================================
 Copy C action to the next ; or closing }
 */
static void cpyact (int32_t offset)
{
    int32_t brac, c, match, j, s, tok;
    int32_t newline = 0;

    if (linsw)
        yprintf (faction, "#line %d \"%s\"\n", lineno, fix_filename (filename));

    brac = 0;

loop:
    c = getc (finput);
swt:
    if (newline)
    {
        yprintf (faction, "\t\t");
        if (c == ' ' || c == '\t')
        {
            int32_t spaces = 0;
            while (c == ' ' || c == '\t')
            {
                if (c == ' ')
                    spaces++;
                else
                    spaces = ((spaces + 1) / tabs + 1) * tabs;

                c = getc (finput);
            }

            spaces = (spaces > lbrace_indent) ? spaces - lbrace_indent : 0;
            while (spaces-- > 0)
                putc (' ', faction);
        }

        newline = 0;
    }

    switch (c)
    {
        case ';':
            if (brac == 0)
            {
                putc (c, faction);
                return;
            }

            goto lcopy;

        case '{':
            if (brac++ == 0)
                yprintf (faction, "\t\t");

            goto lcopy;

        case '$':
            s = 1;
            tok = -1;
            c = getc (finput);
            if (c == '<')
            {
                /* type description */
                ungetc (c, finput); /*lint !e534 */
                if (gettok () != TYPENAME)
                    error ("bad syntax on $<ident> clause", NULL);
                tok = numbval;
                c = getc (finput);
            }

            if (c == '$')
            {
                yprintf (faction, "yyval");
                yprintf (faction, "->");

                if (ntypes)
                {
                    c = getc (finput);
                    if (c == '.')
                    {
                        /* check for position, line or column */
                        int32_t t = getcid ();
                        switch (t)
                        {
                            case POSITION:
                                yprintf (faction, "pos");
                                break;
                            case LINE:
                                yprintf (faction, "pos.line");
                                break;
                            case COLUMN:
                                yprintf (faction, "pos.column");
                                break;
                            case YYFILE:
                                yprintf (faction, "pos.file");
                                break;
                            default:
                                if (tok < 0)
                                    tok = fdtype (*prdptr[nprod]);
                                yprintf (faction, "%s%s.%s", ylines ? "tok." : "", typeset[tok], tokname);
                        }
                        goto loop;
                    }
                    else
                        (void) ungetc (c, finput);

                    /* put out the proper tag... */
                    if (tok < 0)
                        tok = fdtype (*prdptr[nprod]);

                    yprintf (faction, "%s%s", ylines ? "tok." : "", typeset[tok]);
                }
                goto loop;
            }
            if (c == '-')
            {
                s = -s;
                c = getc (finput);
            }
            if (isdigit (c))
            {
                j = 0;
                while (isdigit (c))
                {
                    j = j * 10 + c - '0';
                    c = getc (finput);
                }
                j = j * s - offset;
                if (j > 0)
                {
                    j += offset;
                    error ("Illegal use of $%d", &j);
                }
                yprintf (faction, "yypvt[%d]", j);
                if (ntypes)
                {
                    /* put out the proper tag */
                    if (j + offset <= 0 && tok < 0)
                    {
                        j += offset;
                        error ("must specify type of $%d", &j);
                    }
                    if (c == '.')
                    {
                        /* check for position, line or column */
                        int32_t t = getcid ();
                        switch (t)
                        {
                            case POSITION:
                                yprintf (faction, ".pos");
                                break;
                            case LINE:
                                yprintf (faction, ".pos.line");
                                break;
                            case COLUMN:
                                yprintf (faction, ".pos.column");
                                break;
                            case YYFILE:
                                yprintf (faction, ".pos.file");
                                break;
                            default:
                                if (tok < 0)
                                    tok = fdtype (prdptr[nprod][j + offset]);
                                yprintf (faction, "%s.%s.%s", ylines ? ".tok" : "", typeset[tok], tokname);
                        }
                        goto loop;
                    }
                    if (tok < 0)
                        tok = fdtype (prdptr[nprod][j + offset]);

                    yprintf (faction, "%s.%s", ylines ? ".tok" : "", typeset[tok]);
                }
                goto swt;
            }
            putc ('$', faction);
            if (s < 0)
                putc ('-', faction);
            goto swt;
        case '}':
            if (--brac)
                goto lcopy;
            putc (c, faction);
            return;
        case '/': /* look for comments */
            putc (c, faction);
            if ((c = getc (finput)) != '*')
                goto swt;

            /* it really is a comment */
            putc (c, faction);
            while ((c = getc (finput)) != EOF)
            {
                while (c == '*')
                {
                    putc (c, faction);
                    if ((c = getc (finput)) == '/')
                        goto lcopy;
                }
                putc (c, faction);
                if (c == '\n')
                    ++lineno;
            }
            error ("EOF inside comment", NULL);
            break;
        case '\'': /* character constant */
        case '"':  /* character string */
            match = c;
            putc (c, faction);
            while ((c = getc (finput)) != 0)
            {
                if (c == '\\')
                {
                    putc (c, faction);
                    if ((c = getc (finput)) == '\n')
                        ++lineno;
                }
                else if (c == match)
                    goto lcopy;
                else if (c == '\n')
                    error ("newline in string or char. const.", NULL);

                putc (c, faction);
            }
            error ("EOF in string or character constant", NULL);
            break;

        case -1: /* EOF */
            error ("action does not terminate", NULL);
            break;

        case '\n':
            ++lineno;
            goto lcopy;
        default:
            break;
    }
lcopy:
    newline = (c == '\n');
    putc (c, faction);
    goto loop;
} /*lint !e438 */

static void cpycode (void)
{
    /* copies code between \{ and \} */
    int32_t cc;

    cc = getc (finput);
    if (cc == '\n')
    {
        cc = getc (finput);
        lineno++;
    }

    if (linsw)
        yprintf (ftable, "\n#line %d \"%s\"\n", lineno, fix_filename (filename));

    while (cc >= 0)
    {
        if (cc == '\\')
        {
            if ((cc = getc (finput)) == '}')
                return;

            putc ('\\', ftable);
        }

        if (cc == '%')
        {
            if ((cc = getc (finput)) == '}')
                return;

            putc ('%', ftable);
        }

        putc (cc, ftable);
        if (cc == '\n')
            ++lineno;

        cc = getc (finput);
    }

    error ("eof before %%}", NULL);
}

/* ======================================================================
 Copy the union declaration to the output, and the define file
 */
static void cpyunion (void)
{
    int32_t level, c;

    if (ylines)
    {
        yprintf (fdefine,
                 "#ifndef YYTOKPOS_DEFINED\n"
                 "#define YYTOKPOS_DEFINED\n"
                 "struct YYTOKPOS_S\n{\n"
                 "\tchar        file[%d];\n"
                 "\tint32_t     line;\n"
                 "\tint32_t     column;\n"
                 "};\n"
                 "using YYTOKPOS = YYTOKPOS_S;\n\n"
                 "#endif\n"

                 "struct %s_S\n{\n"
                 "\tYYTOKPOS pos;\n\n"
                 "\tunion ",
                 PSP_FILE_NAMELEN, yystype);
    }
    else
    {
        yprintf (fdefine, "typedef union ");
    }

    level = 0;
    for (;;)
    {
        if ((c = getc (finput)) < 0)
            error ("EOF encountered while processing %%union", NULL);

        putc (c, fdefine);

        if (c == '\n')
        {
            lineno++;
            if (ylines)
                yprintf (fdefine, "    ");
        }
        else if (c == '{')
            level++;
        else if (c == '}' && --level == 0)
        {
            /* we are finished copying */
            if (ylines)
            {
                yprintf (fdefine, " tok;\n};\n");
                yprintf (fdefine, "using %s = %s_S;\n\n", yystype, yystype);
            }
            else
            {
                yprintf (fdefine, " %s;\n\n", yystype);
            }
            return;
        }
    }
}

static const char *cstash (const char *s)
{
    const char *temp = cnamp;

    do
    {
        if (cnamp > &cnames[CNAMSZ - 1])
            error ("too many characters in id's and literals", NULL);

        *cnamp++ = *s;
    } while (*s++);

    return temp;
}

/* ======================================================================
 Define s to be a terminal if t=0 or a nonterminal if t=1
 */
static int32_t defin (int32_t t, const char *s)
{
    int32_t val = 0;

    if (t)
    {
        if (++nnonter >= NNONTERM)
            error ("too many nonterminals, limit %d", &nnonter);

        nontrst[nnonter].name = cstash (s);
        return NTBASE + nnonter;
    }

    /* must be a token */
    if (++ntokens >= NTERMS)
        error ("too many terminals, limit %d", &ntokens);

    tokset[ntokens].name = cstash (s);

    /* establish value for token */
    if (s[0] == ' ' && s[2] == '\0') /* single character literal */
        val = s[1];
    else if (s[0] == ' ' && s[1] == '\\')
    {
        /* escape sequence */
        if (s[3] == '\0')
        {
            /* single character escape sequence */
            switch (s[2])
            {
                    /* character which is escaped */
                case 'n':
                    val = '\n';
                    break;
                case 'r':
                    val = '\r';
                    break;
                case 'b':
                    val = '\b';
                    break;
                case 't':
                    val = '\t';
                    break;
                case 'f':
                    val = '\f';
                    break;
                case '\'':
                    val = '\'';
                    break;
                case '"':
                    val = '"';
                    break;
                case '\\':
                    val = '\\';
                    break;
                default:
                    error ("invalid escape", NULL);
            }
        }
        else if (s[2] <= '7' && s[2] >= '0')
        {
            /* \nnn sequence */
            if (s[3] < '0' || s[3] > '7' || s[4] < '0' || s[4] > '7' || s[5] != '\0')
                error ("illegal \\nnn construction", NULL);

            val = 64 * s[2] + 8 * s[3] + s[4] - 73 * '0';
            if (val == 0)
                error ("'\\000' is illegal", NULL);
        }
        else
            error ("illegal escape sequence", NULL);
    }
    else
        val = extval++;

    tokset[ntokens].value = val;
    toklev[ntokens] = 0;
    return ntokens;
}

/* ======================================================================
 Write out the defines (at the end of the declaration section)
 */
static void defout (void)
{
    int32_t ii;
    int32_t cc;
    const char *cp;

    /* output terminal symbol defines */
    for (ii = ndefout; ii <= ntokens; ++ii)
    {
        cp = tokset[ii].name;
        if (*cp == ' ')
            ++cp; /* literals */

        for (; (cc = *cp) != '\0'; ++cp)
        {
            if (!islower (cc) && !isupper (cc) && !isdigit (cc) && cc != '_')
                break;
        }

        if (cc != '\0')
            continue;

        yprintf (fdefine, "#define %s %d\n", tokset[ii].name, tokset[ii].value);
    }

    ndefout = ntokens + 1;
}

static int32_t fdtype (intptr_t t)
{
    /* determine the type of a symbol */
    int32_t v = 0;
    const char *name = "unknown";

    if (t >= NTBASE)
    {
        t -= NTBASE;
        v = nontrst[t].tvalue;
        name = nontrst[t].name;
    }
    else if (t < NTERMS)
    {
        v = TYPE (toklev[t]);
        name = tokset[t].name;
    }
    else
        error ("invalid type for %s", name);

    if (v <= 0)
        error ("must specify type for %s", name);

    return v;
}

/* =========================================================================
 Get C identifier (possibly qualified)
 */
static int32_t getcid (void)
{
    int32_t ii = 0, reserved, c, t;

    c = getc (finput);
    if (c == '%')
    {
        reserved = 1;
        c = getc (finput);
    }
    else
        reserved = 0;

    if (islower (c) || isupper (c) || c == '_' || c == '.')
    {
        while (islower (c) || isupper (c) || isdigit (c) || c == '_' || c == '.')
        {
            tokname[ii] = (char) c;
            if (reserved && isupper (c))
                tokname[ii] += 'a' - 'A';

            if (++ii >= NAMESIZE)
                --ii;

            c = getc (finput);
        }
    }

    tokname[ii] = '\0';
    (void) ungetc (c, finput);

    if (reserved)
    {
        if (strcmp (tokname, "position") == 0)
            t = POSITION;
        else if (strcmp (tokname, "line") == 0)
            t = LINE;
        else if (strcmp (tokname, "column") == 0)
            t = COLUMN;
        else if (strcmp (tokname, "file") == 0)
            t = YYFILE;
        else
            t = IDENTIFIER;
    }
    else
        t = IDENTIFIER;

    if (!ylines && (t == POSITION || t == LINE || t == COLUMN || t == YYFILE))
        error ("use of %%%s requires -p option", tokname);

    return t;
}

/* =======================================================================
 Get next input token
 */
static int32_t gettok (void)
{
    int32_t c;
    int32_t ii;
    int32_t base;
    int32_t match;
    int32_t reserve;
    int32_t quoted;
    int32_t newline;
    int32_t in_action;

begin:
    newline = 0;
    lineno += peekline;
    peekline = 0;
    quoted = reserve = 0;
    in_action = action;
    action = 0;

    c = getc (finput);
    while (c == ' ' || c == '\n' || c == '\t' || c == '\f' || c == '\r')
    {
        if (newline && c == ' ')
            lbrace_indent++;
        else if (newline && c == '\t')
            lbrace_indent = ((lbrace_indent + 1) / tabs + 1) * tabs;
        else if (c == '\n')
        {
            newline = 1;
            ++lineno;
            lbrace_indent = 0;
        }
        c = getc (finput);
    }

    if (c == '/')
    {
        /* skip comment */
        lineno += skipcom ();
        goto begin;
    }
    switch (c)
    {
        case -1: /* EOF */
            return ENDFILE;

        case '{':
            action = 1;
            ungetc (c, finput); /*lint !e534 */
            return '=';         /* action ... */

        case '<': /* get, and look up, a type name (union
                   member name) */
            ii = 0;
            while ((c = getc (finput)) != '>' && c >= 0 && c != '\n')
            {
                if (ii < NAMESIZE - 1)
                    tokname[ii++] = (char) c;
            }
            tokname[ii] = '\0';
            if (c != '>')
                error ("unterminated < ... > clause", NULL);

            for (ii = 1; ii <= ntypes; ++ii)
            {
                if (!strcmp (typeset[ii], tokname))
                {
                    numbval = ii;
                    return TYPENAME;
                }
            }
            typeset[numbval = ++ntypes] = cstash (tokname);
            return TYPENAME;

        case '"':
        case '\'':
            quoted = 1;
            match = c;
            tokname[0] = ' ';
            ii = 1;
            for (;;)
            {
                c = getc (finput);
                if (c == '\n' || c == EOF)
                    error ("illegal or missing ' or \"", NULL);
                if (c == '\\')
                {
                    c = getc (finput);
                    if (ii < NAMESIZE - 1)
                        tokname[ii++] = '\\';
                }
                else if (c == match)
                    break;

                if (ii < NAMESIZE - 1)
                    tokname[ii++] = (char) c;
            }
            break;

        case '%':
        case '\\':
            switch (c = getc (finput))
            {
                case '0':
                    return TERM;
                case '<':
                    return LEFT;
                case '2':
                    return BINARY;
                case '>':
                    return RIGHT;
                case '=':
                    return PREC;
                case '{':
                    return LCURLY;
                case '%':
                case '\\':
                    return MARK;
                default:
                    reserve = 1;
            } /*lint -fallthrough */

        default:
            if (isdigit (c))
            {
                /* number */
                numbval = c - '0';
                base = (c == '0') ? 8 : 10;
                for (c = getc (finput); isdigit (c); c = getc (finput))
                    numbval = numbval * base + c - '0';
                ungetc (c, finput); /*lint !e534 */
                return NUMBER;
            }
            else if (islower (c) || isupper (c) || c == '_' || c == '.' || c == '$')
            {
                ii = 0;
                while (islower (c) || isupper (c) || isdigit (c) || c == '_' || c == '.' || c == '$')
                {
                    if (reserve && isupper (c))
                        c += 'a' - 'A';
                    if (ii < NAMESIZE - 1)
                        tokname[ii++] = (char) c;
                    c = getc (finput);
                }
            }
            else
            {
                if (in_rules)
                {
                    if (c == '|' || c == ';')
                    {
                        if (in_action || !allacts)
                        {
                            if (c == '|')
                            {
                                char *cp;
                                if ((cp = strchr (ruletext, ':')) != 0)
                                    *(++cp) = '\0';
                            }
                        }
                        else
                        {
                            action = 1;
                            ungetc (c, finput); /*lint !e534 */
                            return '\0';
                        }
                    }
                    else
                    {
                        char ch[2];

                        ch[0] = (char) c;
                        ch[1] = '\0';
                        strcat (ruletext, ch);
                    }
                }
                return c;
            }
            ungetc (c, finput); /*lint !e534 */
    }
    tokname[ii] = '\0';

    if (in_rules)
    {
        if (quoted)
        {
            strcat (ruletext, " '");
            strcat (ruletext, tokname + 1);
            strcat (ruletext, "'");
        }
        else
        {
            strcat (ruletext, " ");
            if (reserve)
                strcat (ruletext, "%");
            strcat (ruletext, tokname);
        }
    }
    if (reserve)
    {
        /* find a reserved word */
        int32_t t = 0;
        if (!strcmp (tokname, "term"))
            t = TERM;
        else if (!strcmp (tokname, "token"))
            t = TERM;
        else if (!strcmp (tokname, "left"))
            t = LEFT;
        else if (!strcmp (tokname, "nonassoc"))
            t = BINARY;
        else if (!strcmp (tokname, "binary"))
            t = BINARY;
        else if (!strcmp (tokname, "right"))
            t = RIGHT;
        else if (!strcmp (tokname, "prec"))
            t = PREC;
        else if (!strcmp (tokname, "start"))
            t = START;
        else if (!strcmp (tokname, "type"))
            t = TYPEDEF;
        else if (!strcmp (tokname, "union"))
            t = UNION;
        else if (!strcmp (tokname, "include"))
            t = INCLUDE;
        else if (!strcmp (tokname, "context"))
            t = CONTEXT;
        else if (!strcmp (tokname, "yystype"))
            t = YYSTYPE;
        else
            error ("invalid escape, or illegal reserved word: %s", tokname);
        return t;
    }

    /* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */
    newline = 0;
    c = getc (finput);
    while (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '/')
    {
        if (newline && c == ' ')
            lbrace_indent++;
        else if (newline && c == '\t')
            lbrace_indent = ((lbrace_indent + 1) / tabs + 1) * tabs;
        else if (c == '\n')
        {
            newline = 1;
            ++peekline;
            lbrace_indent = 0;
        }
        else if (c == '/')
        {
            c = getc (finput);
            ungetc (c, finput); /*lint !e534 */
            if (c == '*')
            {
                /* look for comments */
                peekline += skipcom ();
            }
            else
            {
                c = '/';
                break;
            }
        }
        c = getc (finput);
    }

    if (c == ':')
    {
        in_rules = 1;
        sprintf (ruletext, "%s :", tokname);
        return C_IDENTIFIER;
    }
    ungetc (c, finput); /*lint !e534 */
    return IDENTIFIER;
}

/* =======================================================================
 Set up RDMYacc processing
 */
void setup (int32_t argc, const char *const argv[])
{
    char finsave[FNAMESIZE];
    char root[FNAMESIZE];
    char *cp;
    const char *root_base;
    int32_t j;
    int32_t infsw = 0;
    int32_t ccsw = 0;
    int32_t parsw = 0;

    ylines = rulesw = 0;
    linsw = allacts = 0;
    foutput = NULL;
    fdefine = NULL;
    for (i = 1; i < argc && argv[i][0] == '-'; ++i)
    {
        if (argv[i][1] == 'f')
        {
            strcpy (yaccpar, argv[++i]);
            parsw++;
        }
        else
            for (j = 1; argv[i][j]; ++j)
            {
                switch (argv[i][j])
                {
                    case 'b':
                    case 'B':
                        allacts++;
                        continue;
                    case 'c':
                    case 'C':
                        ccsw++;
                        continue;
                    case 'i':
                    case 'I':
                        infsw++;
                        continue;
                    case 'l':
                    case 'L':
                        linsw++;
                        continue;
                    case 'p':
                    case 'P':
                        ylines++;
                        continue;
                    case 'r':
                    case 'R':
                        rulesw++;
                        continue;
                    case 't':
                    case 'T':
                        tabs = 0;
                        while (isdigit (argv[i][j + 1]))
                            tabs = tabs * 10 + (argv[i][++j] - '0');
                        continue;
                    case 'y':
                        strcpy (ytab_h, "");
                        continue;
                    case 'z':
                        continue; /* ignore deprecated reentrant option */
                    default:
                        fprintf (stderr, "Illegal option: %c\n", *argv[i]);
                        usage ();
                }
            }
    } /*lint !e850 */
    if (argc < 2)
        usage (); /* Catch no filename given */ /*lint !e850 */
    /*
     * Now open the input file with a default extension of ".y",
     * then replace the period in argv[1] with a null, so argv[1]
     * can be used to form the table, defs and info filenames.
     */
    strcpy (root, argv[i]);
    cp = strrchr (root, '.');
    if (cp)
    {
        strcpy (filename, root);
        *cp = '\0'; /* Null the period */
    }
    else
    {
        strcpy (filename, root);
        strcat (filename, ".y");
    }
    root_base = strrchr (root, '/');
    if (root_base)
    {
        root_base++;
    }
    else
    {
        root_base = strrchr (root, '\\');
        if (root_base)
        {
            root_base++;
        }
        else
        {
            root_base = root;
        }
    }
    if (!parsw)
        strcpy (yaccpar, root);

    strcpy (finsave, filename);
    if ((finput = fopen (filename, "r")) == NULL)
        error ("cannot open input file \"%s\"", filename);
    /*
     * Now open ytab.h
     */
    if (!*ytab_h)
    {
        strcpy (ytab_h, root_base);
        strcat (ytab_h, ".h");
    }
    fdefine = fopen (ytab_h, "w");
    if (fdefine == NULL)
        error ("cannot open %s\n", ytab_h);

    {
        char YTAB_H[NAMESIZE];
        char *pYTAB_H;
        int ii;
        strcpy (YTAB_H, "");
        strcat (YTAB_H, ytab_h);
        if ((pYTAB_H = strrchr (YTAB_H, '/')) != NULL || (pYTAB_H = strrchr (YTAB_H, '\\')) != NULL ||
            (pYTAB_H = strrchr (YTAB_H, ':')) != NULL)
        {
            pYTAB_H++;
        }
        else
            pYTAB_H = YTAB_H;

        for (ii = 0; pYTAB_H[ii]; ii++)
        {
            if (pYTAB_H[ii] == '.')
                pYTAB_H[ii] = '_';
            else if (islower (pYTAB_H[ii]))
                pYTAB_H[ii] = (char) toupper (pYTAB_H[ii]);
        }
        strcat (YTAB_H, "_INCLUDED_");

        fprintf (fdefine, "#ifndef %s\n#define %s\n", pYTAB_H, pYTAB_H);
    }
    /*
     * Now open the .i file if requested.
     */
    if (infsw)
    {
        strcpy (filename, root_base);
        strcat (filename, ".i");
        foutput = fopen (filename, "w");
        if (foutput == NULL)
            error ("cannot open info file\"%s\"", filename);
    }
    /*
     * Now the "table" output C file.
     */
    strcpy (filename, root_base);
    strcat (filename, ccsw ? ".cpp" : ".c");
    ftable = fopen (filename, "w");
    if (ftable == NULL)
        error ("cannot open table file\"%s\"", filename);
    /*
     * Finally, the temp files.
     */
    ftemp = yfopen (TEMPNAME, "w");
    if (ftemp == NULL)
    {
        error ("cannot open temp file", NULL);
    }

    faction = yfopen (ACTNAME, "w");
    if (faction == NULL)
    {
        error ("cannot open action temp file", NULL);
    }

    frules = yfopen (RULENAME, "w");
    if (frules == NULL)
    {
        error ("cannot open rules temp file", NULL);
    }
    /*
     * Now put the full filename of the input file into
     * the "filename" buffer for cpyact(), and point the
     * global cell "infile" at it.
     */
    strcpy (filename, finsave);
    infile = filename;
    /*
     * Complete  initialization.
     */
    cnamp = cnames;
    defin (0, "$end");
    extval = 0400;
    defin (0, "error");
    defin (1, "$accept");
    mem = mem0;
    lev = 0;
    ty = 0;
    i = 0;

    yyparse ();
}

static int32_t skipcom (void)
{
    /* skip over comments */
    int32_t c;
    int32_t ii = 0; /* ii is the number of lines skipped */

    /* skipcom is called after reading a / */
    if (getc (finput) != '*')
        error ("illegal comment", NULL);

    c = getc (finput);
    while (c != EOF)
    {
        while (c == '*')
        {
            if ((c = getc (finput)) == '/')
                return ii;
        }

        if (c == '\n')
            ++ii;

        c = getc (finput);
    }

    error ("EOF inside comment", NULL);

    /* We cannot reach here, but just to remove compiler warnings... */
    return 0; /*lint !e527 */
}

/* =======================================================================
 */
static char *yystrupr (char *str)
{
    char *p;

    if (!str)
    {
        return NULL;
    }

    for (p = str; *p; p++)
    {
        if (islower ((int) *p))
        {
            *p = (char) toupper ((int) *p);
        }
    }

    return str;
}

/* =======================================================================
 Parse RDMYacc specification
 */
static void yyparse (void)
{
    /* sorry -- no yacc parser here..... we must bootstrap somehow... */
    char actname[8];
    intptr_t *p;
    intptr_t tempty;
    int32_t t;
    int32_t c;
    int32_t j;

    for (t = gettok (); t != MARK;)
    {
        switch (t)
        {
            case ';':
                t = gettok ();
                break;
            case START:
                if (gettok () != IDENTIFIER)
                    error ("bad %% start construction", NULL);

                strcpy (startnm, tokname);
                strcpy (startnmuc, tokname);
                (void) yystrupr (startnmuc);
                start = chfind (1, tokname);
                t = gettok ();
                continue;
            case ENDFILE:
                if (CloseInclude ())
                {
                    t = gettok ();
                    continue;
                }
                error ("unexpected EOF before %%", NULL);
                break;
            case INCLUDE:
                if (gettok () != IDENTIFIER)
                    error ("bad %%include specification", NULL);

                OpenInclude (tokname);
                t = gettok ();
                continue;
            case TYPEDEF:
                if ((t = gettok ()) != TYPENAME)
                    error ("bad syntax in %%type", NULL);
                ty = numbval;
                for (;;)
                {
                    t = gettok ();
                    switch (t)
                    {
                        case IDENTIFIER:
                            t = chfind (1, tokname);
                            if (t < NTERMS)
                            {
                                j = TYPE (toklev[t]);
                                if (j != 0 && j != ty)
                                {
                                    error ("type redeclaration of token %s", tokset[t].name);
                                }
                                else
                                    SETTYPE (toklev[t], ty);
                            }
                            else if (t >= NTBASE && t < NTBASE + NNONTERM)
                            {
                                j = nontrst[t - NTBASE].tvalue;
                                if (j != 0 && j != ty)
                                {
                                    error ("nonterminal type redeclaration %s", nontrst[t - NTBASE].name);
                                }
                                else
                                    nontrst[t - NTBASE].tvalue = ty;
                            }
                            else
                                error ("invalid %%type", NULL);
                            /*lint -fallthrough */
                        case ',':
                            continue;
                        case ';':
                            t = gettok ();
                            break;
                        default:
                            break;
                    }
                    break;
                }
                continue;
            case UNION:
                /* copy the union declaration to the output */
                cpyunion ();
                t = gettok ();
                continue;
            case LEFT:
            case BINARY:
            case RIGHT:
                ++i;
                /*lint -fallthrough */
            case TERM:
                lev = t - TERM; /* nonzero means new prec. and assoc. */
                ty = 0;

                /* get identifiers so defined */
                t = gettok ();
                if (t == TYPENAME)
                {
                    /* there is a type defined */
                    ty = numbval;
                    t = gettok ();
                }
                for (;;)
                {
                    switch (t)
                    {
                        case ',':
                            t = gettok ();
                            continue;
                        case ';':
                            break;
                        case IDENTIFIER:
                            j = chfind (0, tokname);
                            if (lev)
                            {
                                if (ASSOC (toklev[j]))
                                    error ("redeclaration of precedence of %s", tokname);
                                SETASC (toklev[j], lev);
                                SETPLEV (toklev[j], i);
                            }
                            if (ty)
                            {
                                if (TYPE (toklev[j]))
                                    error ("redeclaration of type of %s", tokname);
                                SETTYPE (toklev[j], ty);
                            }
                            if ((t = gettok ()) == NUMBER)
                            {
                                tokset[j].value = numbval;
                                if (j < ndefout && j > 2)
                                {
                                    error ("define type number of %s earlier", tokset[j].name);
                                }
                                t = gettok ();
                            }
                            continue;
                        default:
                            break;
                    }
                    break;
                }
                continue;
            case CONTEXT:
                t = gettok ();
                if (t == IDENTIFIER)
                {
                    strcpy (ctxtype, tokname);
                    t = gettok ();
                }
                if (t != IDENTIFIER)
                {
                    error ("%context expects two identifiers", tokname);
                    break;
                }
                else
                    strcpy (ctxname, tokname);

                t = gettok ();
                continue;
            case YYSTYPE:
                t = gettok ();
                if (t == IDENTIFIER)
                    strcpy (yystype, tokname);
                else
                {
                    error ("%yystype expects an identifier", tokname);
                    break;
                }
                t = gettok ();
                continue;
            case LCURLY:
                defout ();
                cpycode ();
                t = gettok ();
                continue;
            default:
                printf ("Unrecognized character: %c (0%o)\n", (char) t, t);
                error ("syntax error", NULL);
        }
    }
    /* t is MARK */
    defout ();

    yprintf (ftable, "\n");
    if (linsw)
        yprintf (ftable, "#line %d \"%s\"\n\n", lineno, fix_filename (filename));

    prdptr[0] = mem;

    /* added production */
    *mem++ = NTBASE;
    *mem++ = start; /* if start is 0, we will overwrite with the
                     lhs of the firstrule */
    *mem++ = 1;
    *mem++ = 0;
    prdptr[1] = mem;
    while ((t = gettok ()) == LCURLY)
        cpycode ();
    if (t != C_IDENTIFIER)
        error ("bad syntax on first rule", NULL);
    if (!start)
        prdptr[0][1] = chfind (1, tokname);

    /* read rules */
    while (t != MARK)
    {
        /* process a rule or include directive */
        if (t == ENDFILE)
        {
            if (CloseInclude ())
            {
                t = gettok ();
                continue;
            }
            break;
        }
        else if (t == INCLUDE)
        {
            if (gettok () != IDENTIFIER)
            {
                error ("bad %%include specification", NULL);
            }
            OpenInclude (tokname);
            t = gettok ();
            continue;
        }
        else if (t == '|')
        {
            *mem++ = *prdptr[nprod - 1];
        }
        else if (t == C_IDENTIFIER)
        {
            *mem = chfind (1, tokname);
            if (*mem < NTBASE)
                error ("token illegal on LHS of grammar rule", NULL);
            ++mem;
        }
        else
            error ("illegal rule: missing semicolon or | ?", NULL);

        /* read rule body */
        t = gettok ();
    more_rule:
        while (t == IDENTIFIER)
        {
            *mem = chfind (1, tokname);
            if (*mem < NTBASE)
                levprd[nprod] = toklev[*mem];
            ++mem;
            t = gettok ();
        }
        if (t == PREC)
        {
            if (gettok () != IDENTIFIER)
                error ("illegal %%prec syntax", NULL);
            j = chfind (2, tokname);
            if (j < NTERMS)
                levprd[nprod] = toklev[j];
            else if (j >= NTBASE && j < NTBASE + NNONTERM)
            {
                error ("nonterminal %s illegal after %%prec", nontrst[j - NTBASE].name);
            }
            else
                error ("invalid %%prec token", NULL);
            t = gettok ();
        }
        if (t == '=')
        {
            levprd[nprod] |= ACTFLAG;
            yprintf (frules, "\t\"%s\",\n", ruletext);
            yprintf (faction, "\n\t\tcase %d: ", nprod);
            if (rulesw)
                yprintf (faction, "/* %s */", ruletext);
            yprintf (faction, "\n");
            cpyact ((int32_t) ((mem - prdptr[nprod]) - 1));
            yprintf (faction, " break;\n");
            if ((t = gettok ()) == IDENTIFIER)
            {
                /* action within rule... */
                sprintf (actname, "$$%d", nprod);
                j = chfind (1, actname); /* make it a nonterminal */
                /* the current rule will become rule number nprod+1 */
                /* move the contents down, and make room for the null */
                for (p = mem; p >= prdptr[nprod]; --p)
                    p[2] = *p;
                mem += 2;
                /* enter null production for action */
                p = prdptr[nprod];
                *p++ = j;
                *p++ = -nprod;

                /* update the production information */
                levprd[nprod + 1] = levprd[nprod] & ~ACTFLAG;
                levprd[nprod] = ACTFLAG;

                if (++nprod >= NPROD)
                    error ("more than %d rules", &nprod);

                prdptr[nprod] = p;

                /* make the action appear in the original rule */
                *mem++ = j;

                /* get some more of the rule */
                goto more_rule;
            }
        }
        else if (t == '\0')
        { /* null rule */
            levprd[nprod] |= ACTFLAG;
            yprintf (frules, "\t\"%s\",\n", ruletext);
#if defined(RDM_YACC_NOOP_CASES)
            if (rulesw)
            {
                yprintf (faction,
                         "\n\t\tcase %d: "
                         "/* %s */\n",
                         nprod, ruletext);
                if (linsw)
                    yprintf (faction, "#line %d \"%s\"\n", lineno, fix_filename (filename));
                yprintf (faction, "\t\t\t{ int __noop UNUSEDVAR = 0; } break; /*lint !e438 !e529 */\n");
            }
#else
            if (rulesw)
            {
                yprintf (faction, "\n\t\t/* case: %d: %s */\n", nprod, ruletext);
            }
#endif
            t = gettok ();
        }
        while (t == ';')
            t = gettok ();

        *mem++ = -nprod;

        /* check that default action is reasonable */
        if (ntypes && !(levprd[nprod] & ACTFLAG) && nontrst[*prdptr[nprod] - NTBASE].tvalue)
        {
            /* no explicit action, LHS has value */
            /* 01 */
            tempty = prdptr[nprod][1];
            if (tempty < 0)
                error ("must return a value, since LHS has a type", NULL);
            else if (tempty < NTERMS)
                tempty = TYPE (toklev[tempty]);
            else if (tempty >= NTBASE && tempty < NTBASE + NNONTERM)
                tempty = nontrst[tempty - NTBASE].tvalue;
            else
                error ("default action error", NULL);

            if (tempty != nontrst[*prdptr[nprod] - NTBASE].tvalue)
                error ("default action causes potential type clash", NULL);
        }
        if (++nprod >= NPROD)
            error ("more than %d rules", &nprod);

        prdptr[nprod] = mem;
        levprd[nprod] = 0;
    }
    /* end of all rules */
#if defined(RDM_YACC_NOOP_CASES)
    yprintf (faction, "\n\t\tdefault: break;");
#endif
    yprintf (faction, "\n\t\t/* End of actions */\n");

    /* finish action routine */
    fclose (faction);
    fclose (frules);
    yunlink (RULENAME);

    yprintf (ftable, "#define YYERRCODE_%s %d\n", startnmuc, tokset[2].value);
    if (t == MARK)
    {
        if (linsw)
            yprintf (ftable, "\n#line %d \"%s\"\n", lineno, fix_filename (filename));

        while ((c = getc (finput)) != EOF)
            putc (c, ftable);
    }
    fclose (finput);
}

/* =======================================================================
 Open include file
 */
static void OpenInclude (const char *filenm)
{
    INFILE_STACK *fsp;

    fsp = (INFILE_STACK *) malloc (sizeof (INFILE_STACK));
    if (!fsp)
        error ("include stack too deep: out of memory", NULL);
    fsp->next = filesp;
    fsp->fp = finput;
    fsp->lineno = lineno;
    lineno = 1;
    strcpy (fsp->name, filename);
    strncpy (filename, filenm, sizeof (filename));
    filesp = fsp;
    if ((finput = fopen (filenm, "r")) == NULL)
        error ("unable to open include file: %s", filenm);
}

/* =======================================================================
 Close include file
 */
static int32_t CloseInclude ()
{
    INFILE_STACK *tfsp;
    /* returns 1 if open file is an include file otherwise 0 */
    if (!filesp)
        return 0;

    tfsp = filesp;
    fclose (finput);
    finput = filesp->fp;
    strcpy (filename, filesp->name);
    lineno = filesp->lineno;
    filesp = filesp->next;
    free (tfsp);
    return 1;
}

/* =======================================================================
 Show RDMYacc command usage
 */
static void usage (void)
{
    fprintf (stderr,
             "rdmyacc [-bcilpry] [-t#] [-f actsfcn] infile\n\n"
             "   -b   Include cases for all productions in <infile>.c\n"
             "   -c   Output to <infile>.cc instead of <infile>.c\n"
             "   -f   Name of parser actions function\n"
             "   -i   Create parser description file <infile>.i\n"
             "   -l   Output #line directives\n"
             "   -p   Include line & column positions in token type definition\n"
             "   -r   Output comments containing production rules\n"
             "   -t   Set tab stops (default=%d)\n"
             "   -y   Put token #defines in <infile>.h instead of ytab.h\n"
             "\nDefault input file extension is \".y\"\n",
             TABS);
    exit (EX_ERR);
}
