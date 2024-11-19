/*
 * HELLO WORLD
 * -----------
 *
 * This document describes the process to create a simple database,
 * insert a record containing a text field, read the text field from
 * database and print it out.
 */

#include "rdm.h"                   /* The RDM API. */
#include "core01Example_structs.h" /* The core01Example database definitions. */
#include "core01Example_cat.h"     /* The core01Example database catalog */
#include "rdmstartupapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Read "helloworld" row to database */
static RDM_RETCODE readARow (RDM_DB hDB)
{
    RDM_RETCODE rc;

    /* Start an update transaction and lock the table */
    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    if (rc == sOKAY)
    {
        RDM_CURSOR hCursor;

        rc = rdm_dbAllocCursor (hDB, &hCursor);
        if (rc == sOKAY)
        {
            /* Fill a cursor with all the rows in a table */
            rc = rdm_dbGetRows (hDB, TABLE_INFO, &hCursor);
            if (rc == sOKAY)
            {
                /* Move to the first row in the info table */
                rc = rdm_cursorMoveToFirst (hCursor);
            }
            if (rc == sOKAY)
            {
                INFO infoRead; /* Row buffer */

                /* Read the full content of the current record */
                rc = rdm_cursorReadRow (
                    hCursor, &infoRead, sizeof (infoRead), NULL);
                if (rc == sOKAY)
                {
                    printf ("%s\n", infoRead.mychar);
                }
            }
            /* Free the cursor */
            rdm_cursorFree (hCursor);
        }
        /* Free the read lock on the table */
        rdm_dbEnd (hDB);
    }
    return rc;
}

/* Insert "helloworld" row to database */
static RDM_RETCODE writeARow (RDM_DB hDB)
{
    RDM_RETCODE rc;

    /* Start an update transaction and lock the table */
    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    if (rc == sOKAY)
    {
        INFO infoInserted; /* Row buffer */

        /* Populate the mychar column in the Row buffer */
        strcpy (infoInserted.mychar, "Hello World! - using the embedded TFS");

        /* Insert a row into the table */
        rc = rdm_dbInsertRow (
            hDB, TABLE_INFO, &infoInserted, sizeof (infoInserted), NULL);
        if (rc == sOKAY)
        {
            /* Commit a transaction */
            rc = rdm_dbEnd (hDB);
        }
        else
        {
            /* Abort the transaction */
            rdm_dbEndRollback (hDB);
        }
    }
    return rc;
}

RDM_EXPORT int32_t main_core01Example (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc; /* Status/Error Return Code */
    RDM_TFS hTFS;    /* TFS Handle */

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS (&hTFS);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (hTFS);
        if (rc == sOKAY)
        {
            RDM_DB hDB = NULL; /* Database Handle */
            rc = rdm_tfsDropDatabase (hTFS, "helloworld");
            if (rc == sNODB)
            {
                /* It is okay if there wasn't a database to drop */
                rc = sOKAY;
            }
            if (rc == sOKAY)
            {
                /* Allocate a database hande */
                rc = rdm_tfsAllocDatabase (hTFS, &hDB);
            }
            if (rc == sOKAY)
            {
                /* Associate the catalog with the database handle */
                rc = rdm_dbSetCatalog (hDB, core01Example_cat);

                if (rc == sOKAY)
                {
                    /* Open the database */
                    rc = rdm_dbOpen (hDB, "helloworld", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("I can't open the helloworld database.\n");
                    }
                }
                if (rc == sOKAY)
                {
                    /* Insert "helloworld" row to database */
                    rc = writeARow (hDB);
                    if (rc == sOKAY)
                    {
                        /* Read "helloworld" row to database */
                        rc = readARow (hDB);
                    }
                    rdm_dbClose (hDB);
                }
                rdm_dbFree (hDB);
            }
        }
        rdm_tfsFree (hTFS);
    }

    if (rc != sOKAY)
    {
        printf (
            "There was an error in this Tutorial (%s - %s)\n",
            rdm_retcodeGetName (rc), rdm_retcodeGetDescription (rc));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

RDM_STARTUP_EXAMPLE (core01Example)
