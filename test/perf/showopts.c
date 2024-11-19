#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "showopts.h"

static void displayOpts (char *opts)
{
    char *p;
    char *nxtOpt = opts;
    int ii;

    for (ii = 0;; ii++)
    {
        p = strchr (nxtOpt, ';');
        if (p)
        {
            /* there's more so terminate this one */
            *p = '\0';
        }
        printf ("%c\t%-40s", (ii % 2) ? ' ' : '\n', nxtOpt);
        if (p)
        {
            /* there's more so position after the old ';' location */
            nxtOpt = p + 1;
        }
        else
        {
            /* no more, head for home */
            break;
        }
    }
    printf ("\n");
}

RDM_RETCODE showOptsDB (RDM_DB hDB)
{
    RDM_RETCODE rc;
    size_t outSize;
    char *opts;

    rc = rdm_dbGetOptions (hDB, NULL, 0, &outSize);
    if (rc == sOKAY)
    {
        if (outSize)
        {
            opts = (char *) malloc (outSize);
            if (opts)
            {
                rc = rdm_dbGetOptions (hDB, opts, outSize, NULL);
                if (rc == sOKAY)
                {
                    displayOpts (opts);
                }
                free (opts);
            }
        }
    }
    return rc;
}

RDM_RETCODE showOptsTFS (RDM_TFS hTFS)
{
    RDM_RETCODE rc;
    size_t outSize;
    char *opts;

    rc = rdm_tfsGetOptions (hTFS, NULL, 0, &outSize);
    if (rc == sOKAY)
    {
        if (outSize)
        {
            opts = (char *) malloc (outSize);
            if (opts)
            {
                rc = rdm_tfsGetOptions (hTFS, opts, outSize, NULL);
                if (rc == sOKAY)
                {
                    displayOpts (opts);
                }
                free (opts);
            }
        }
    }
    return rc;
}
