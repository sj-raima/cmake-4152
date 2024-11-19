/*
 * TRANSACTIONS
 * ------------
 *
 * This document describes the process to create a simple database
 * that stores data using transactions. We'll insert a hundred records
 * in the table, and verify by counting the number of records stored
 * in the database.
 *
 */

#include "rdm.h" /* The RDM API.                   */
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core32Example_structs.h" /* The transaction database definitions.   */
#include "core32Example_cat.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <stdlib.h>

RDM_EXPORT int32_t main_transactionsTutorial (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc; /*lint -esym(850,iStatus) */
    RDM_TFS tfs;

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS (&tfs);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (tfs);
        if (rc == sOKAY)
        {
            RDM_DB db;
            /* Allocate a database hande */
            rc = rdm_tfsAllocDatabase (tfs, &db);
            if (rc == sOKAY)
            {
                RDM_TABLE_ID tables[] = {TABLE_INFO};
                int32_t iTrans;
                int32_t iCnt =0;

                rc = rdm_dbSetCatalog (db, core32Example_cat);
                if (rc == sOKAY)
                {
                    /* Open the database. */
                    rc = rdm_dbOpen (db, "core32", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("Can't open the transaction database.");
                    }
                }
                if (rc == sOKAY)
                {
                    /* Start an update transaction and lock the table */
                    rc = rdm_dbStartUpdate (
                        db, tables, RDM_LEN (tables), NULL, 0, NULL);
                }
                if (rc == sOKAY)
                {
                    /* Remove all of the rows from the database */
                    rc = rdm_dbDeleteAllRowsFromDatabase (db);

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

                /* Loop to group 10 records into 10 different transactions. */
                for (iTrans = 0; iTrans < 10 && rc == sOKAY; iTrans++)
                {
                    /* Start a transaction. */
                    rc = rdm_dbStartUpdate (
                        db, tables, RDM_LEN (tables), NULL, 0, NULL);
                    if (rc == sOKAY)
                    {
                        for (iCnt = 0; iCnt < 10 && rc == sOKAY; iCnt++)
                        {
                            INFO sInfo;

                            /* Insert a new row into the INFO table */
                            sprintf (
                                sInfo.mystr, "Inserted %d Transaction %d", iCnt,
                                iTrans);
                            sInfo._mystr_has_value = RDM_COL_HAS_VALUE;
                            rc = rdm_dbInsertRow (
                                db, TABLE_INFO, &sInfo, sizeof (sInfo), NULL);
                        }

                        if (rc == sOKAY)
                        {
                            /* Commit the transaction */
                            rc = rdm_dbEnd (db);
                        }
                        else
                        {
                            /* Something is wrong, rollback the transaction */
                            rdm_dbEndRollback (db);
                        }
                    }
                }

                if (rc == sOKAY)
                {
                    /* Start a transaction on the */
                    rc = rdm_dbStartRead (db, tables, RDM_LEN (tables), NULL);
                }
                if (rc == sOKAY)
                {
                    RDM_CURSOR cursor = NULL;
                    iCnt = 0;

                    /* Get a cursor containing all the rows in the INFO table */
                    rc = rdm_dbGetRows (db, TABLE_INFO, &cursor);

                    if (rc == sOKAY)
                    {
                        for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
                             rc = rdm_cursorMoveToNext (cursor))
                        {
                            INFO sInfo;

                            /* Read and print the INFO record. */
                            rc = rdm_cursorReadRow (
                                cursor, &sInfo, sizeof (sInfo), NULL);
                            if (rc == sOKAY)
                            {
                                printf ("%s\n", sInfo.mystr);
                            }

                            iCnt++;
                        }

                        /* We expect to break out of the loop with a return code
                         * of sENDOFCURSOR */
                        if (rc == sENDOFCURSOR)
                        {
                            rc = sOKAY;
                        }

                        /* Free the cursor allocated by rdm_dbGetRows */
                        rdm_cursorFree (cursor);
                    }

                    /* Free the read lock */
                    rdm_dbEnd (db);
                }
                printf ("\n");
                if (rc == sOKAY)
                {
                    printf ("I found %d INFO records in the database\n", iCnt);
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

RDM_STARTUP_EXAMPLE (transactionsTutorial)
