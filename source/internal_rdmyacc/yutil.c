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

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern int32_t tabs; /*lint !e956 */

void aryfil (intptr_t *v, int32_t n, int32_t c)
{
    /* set elements 0 through n-1 to c */
    while (n--)
    {
        v[n] = c;
    }
}

/* 12-Apr-83 (RBD) Add symbolic exit status */
void warn (const char *s, const void *a1)
{
    /* write out error comment */
    ++nerrors;

    if (infile)
    {
        fprintf (stderr, "%s:%d: error: ", infile, lineno);
    }

    if (strstr (s, "%s"))
    {
        fprintf (stderr, s, (const char *) a1); /*lint !e905 */
    }
    else if (strstr (s, "%d"))
    {
        fprintf (stderr, s, *((const int32_t *) a1)); /*lint !e905 */
    }
    else
    {
        fprintf (stderr, "%s", s);
    }

    fprintf (stderr, "\n");
}

void error (const char *s, const void *a1)
{
    warn (s, a1);
    summary ();
    exit (EX_ERR);
}

void summary ()
{
    /* output the summary on the tty */
    if (foutput != NULL)
    {
        yprintf (foutput,
                 "\n%d/%d terminals, %d/%d nonterminals\n"
                 "%d/%d grammar rules, %d/%d states\n"
                 "%d shift/reduce, %d reduce/reduce conflicts\n"
                 "%d/%d working sets used\n"
                 "memory: states,etc. %d/%d, parser %d/%d\n"
                 "%d/%d distinct lookahead sets\n"
                 "%d extra closures\n"
                 "%d shift entries, %d exceptions\n"
                 "%d goto entries\n"
                 "%d entries saved by goto default\n",
                 ntokens, NTERMS, nnonter, NNONTERM, nprod, NPROD, nstate, NSTATES, zzsrconf, zzrrconf, zzcwp - wsets,
                 WSETSIZE, (intptr_t *) zzmemsz - mem0, MEMSIZE, memp - amem, ACTSIZE, nlset, LSETSIZE,
                 zzclose - 2 * nstate, zzacent, zzexcp, zzgoent, zzgobest);
    }

    if (zzsrconf != 0 || zzrrconf != 0)
    {
        if (infile)
        {
            fprintf (stderr, "%s:%d: ", infile, lineno);
        }
        fprintf (stdout, "info: conflicts: ");
        if (zzsrconf)
            fprintf (stdout, "%d shift/reduce", zzsrconf);

        if (zzsrconf && zzrrconf)
            fprintf (stdout, ", ");

        if (zzrrconf)
            fprintf (stdout, "%d reduce/reduce", zzrrconf);

        fprintf (stdout, "\n");
    }

    if (ftemp)
        fclose (ftemp);

    if (fdefine)
    {
        fprintf (fdefine, "#endif\n");
        fclose (fdefine);
    }
}

/* =======================================================================
 */
void yprintf (FILE *f, const char *fmt, ...)
{
    static char line[512]; /*lint !e956 */

    va_list args;
    int32_t ii;
    const char *q;

    va_start (args, fmt);
    (void) vsprintf (line, fmt, args);
    va_end (args);

    for (q = line; *q; q++)
    {
        if (*q == '\t')
        {
            ii = tabs;
            while (ii-- > 0)
            {
                putc (' ', f);
            }
        }
        else
        {
            putc (*q, f);
        }
    }
} /*lint !e438 */

static uint64_t y_get_pid (void)
{
#if defined(_WIN32)
    return (uint32_t) _getpid ();
#else
    return (uint64_t) getpid ();
#endif
}

static char *y_get_unique_filename (const char *path /**< [in]  file name */
)
{
    size_t len;
    char *u_name;
    uint64_t pid;

    len = strlen (path) + DISPLAYSIZE;
    pid = y_get_pid ();

    u_name = (char *) malloc (len);
    if (u_name != NULL)
    {
#if defined(_WIN32)
        sprintf (u_name, "%I64u_%s", pid, path);
#elif defined(__APPLE__) & defined(__MACH__)
        sprintf (u_name, "%llu_%s", pid, path);
#elif defined(__x86_64) || defined(__x86_64__)
        sprintf (u_name, "%lu_%s", pid, path);
#else
        sprintf (u_name, "%llu_%s", pid, path);
#endif
    }

    return u_name;
}

/** \brief Open a file using process-specific file name
 */
FILE *yfopen (const char *path, /**< [in]  file name */
              const char *mode  /**< [in]  open mode */
)
{
    FILE *fp = NULL;
    char *u_name;

    u_name = y_get_unique_filename (path);
    if (u_name != NULL)
    {
        fp = fopen (u_name, mode);
        free (u_name);
    }

    return fp;
}

/** \brief Clobber tempfiles after use
 */
void yunlink (const char *pathname /**< [in]  file name */
)
{
    char *u_name;

    u_name = y_get_unique_filename (pathname);
    if (u_name != NULL)
    {
#if defined(_WIN32)
        (void) _unlink (u_name);
#else
        (void) unlink (u_name);
#endif
        free (u_name);
    }
}
