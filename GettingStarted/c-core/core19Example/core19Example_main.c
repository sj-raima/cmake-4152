/*
**    IN-MEMORY
**    ---------
**    This document describes the process to create a simple in-memory database,
**    insert a record, and read data from the database. This application
*requires no
**    persistent storage to run and can act as a simple example for disk less
**    applications.
*/

#include "rdm.h" /* The RDM API.                   */
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core19Example_structs.h" /* The core19Example database definitions.*/
#include "core19Example_cat.h"     /* The core19Example database dictionary. */
#include "rdmstartupapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

RDM_EXPORT int32_t main_core19ExampleTutorial (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
    RDM_TFS tfs;
    RDM_DB db = NULL;
    RDM_TABLE_ID tables[] = {TABLE_INFO};

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS (&tfs);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (tfs);
        if (rc == sOKAY)
        {
            /* Allocate a database handle */
            rc = rdm_tfsAllocDatabase (tfs, &db);
            if (rc == sOKAY)
            {
                /* Associate the catalog with the database handle */
                rc = rdm_dbSetCatalog (db, core19Example_cat);

                if (rc == sOKAY)
                {
                    /* Set the in-memory volatile database option */
                    rc = rdm_dbSetOptions (db, "storage=inmemory_volatile");
                }

                if (rc == sOKAY)
                {
                    /* Open the database. */
                    rc = rdm_dbOpen (db, "core19", RDM_OPEN_SHARED);
                    if (rc == sOKAY)
                    {
                        /* Start an update transaction and lock the table */
                        rc = rdm_dbStartUpdate (
                            db, tables, RDM_LEN (tables), NULL, 0, NULL);
                        if (rc == sOKAY)
                        {
                            INFO infoInsert;

                            /* Create a row in the in-memory database */
                            strcpy (
                                infoInsert.mychar, "I'm running in memory!");
                            infoInsert._mychar_has_value = RDM_COL_HAS_VALUE;
                            rc = rdm_dbInsertRow (
                                db, TABLE_INFO, &infoInsert,
                                sizeof (infoInsert), NULL);

                            if (rc == sOKAY)
                            {
                                /* Commit a transaction */
                                rc = rdm_dbEnd (db);
                            }
                            else
                            {
                                /* Abort the transaction */
                                rdm_dbEndRollback (db);
                            }
                        }

                        rc = rdm_dbStartRead (
                            db, tables, RDM_LEN (tables), NULL);
                        if (rc == sOKAY)
                        {
                            RDM_CURSOR cursor = NULL;
                            rc = rdm_dbGetRows (db, TABLE_INFO, &cursor);
                            if (rc == sOKAY)
                            {
                                /* Move to the first row in the cursor */
                                rc = rdm_cursorMoveToFirst (cursor);
                                if (rc == sOKAY)
                                {
                                    INFO infoRead;

                                    /* read the row values */
                                    rc = rdm_cursorReadRow (
                                        cursor, &infoRead, sizeof (infoRead),
                                        NULL);
                                    if (rc == sOKAY)
                                    {
                                        printf ("%s\n\n", infoRead.mychar);
                                    }
                                    else
                                    {
                                        fprintf (
                                            stderr,
                                            "Sorry, I can't read the first "
                                            "info row.");
                                    }
                                }

                                /* Free the cursor allocated in the call to
                                 * rdm_dbGetRows */
                                rdm_cursorFree (cursor);
                            }

                            rdm_dbEnd (db);
                        }
                    }
                    else
                    {
                        fprintf (
                            stderr,
                            "Sorry, I can't open the core19Example database.");
                    }
                }
                rdm_dbFree (db);
            }
        }
        rdm_tfsFree (tfs);
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

RDM_STARTUP_EXAMPLE (core19ExampleTutorial)
