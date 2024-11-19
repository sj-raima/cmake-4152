/*
 * DUPLICATE KEY
 * -------------
 *
 * This document describes the process to create a simple database containing
 * a key, insert records, and read data through the key.
 *
 * For a full description of RDM keys, see Database Design in the Users
 * Guide.
 *
 * This example contains a single record type with a single field which is
 * designated as a key.  An insertion with a key field value that already
 * exists in the database will not be rejected as an error from the d_fillnew
 * function. Multiple records with the same key value can be stored in the
 * database when the key is not designated as unique.
 */

#include "rdm.h"
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core17Example_structs.h"
#include "core17Example_cat.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static INFO infoData[] = {
    {"John", RDM_COL_HAS_VALUE},   {"James", RDM_COL_HAS_VALUE},
    {"Duncan", RDM_COL_HAS_VALUE}, {"Bill", RDM_COL_HAS_VALUE},
    {"John", RDM_COL_HAS_VALUE},   {"Paul", RDM_COL_HAS_VALUE},
    {"David", RDM_COL_HAS_VALUE}};

RDM_EXPORT int32_t main_core17ExampleTutorial (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
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
                INFO sInfo;
                RDM_TABLE_ID tables[] = {TABLE_INFO};

                /* Set the catalog */
                rc = rdm_dbSetCatalog (db, core17Example_cat);

                /* Open the database. */
                if (rc == sOKAY)
                {
                    rc = rdm_dbOpen (db, "core17", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("I can't open the unique_key database.");
                    }
                }

                /* Start an update transaction and lock the table */
                if (rc == sOKAY)
                {
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

                if (rc == sOKAY)
                {
                    /* Start an update transaction and lock the table */
                    rc = rdm_dbStartUpdate (
                        db, tables, RDM_LEN (tables), NULL, 0, NULL);
                }
                if (rc == sOKAY)
                {
                    uint32_t ii;
                    /* Add rows to the info table */
                    for (ii = 0; ii < RDM_LEN (infoData) && rc == sOKAY; ii++)
                    {
                        rc = rdm_dbInsertRow (
                            db, TABLE_INFO, &infoData[ii], sizeof (INFO), NULL);
                        if (rc == sDUPLICATE)
                        {
                            /* We expect duplicates, re-set error code. */
                            printf (
                                "%s is a duplicate record.\n", sInfo.mychar);
                            rc = sOKAY;
                        }
                    }

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

                if (rc == sOKAY)
                {
                    /* Start a read transaction and lock the table */
                    rc = rdm_dbStartRead (db, tables, RDM_LEN (tables), NULL);
                }
                if (rc == sOKAY)
                {
                    RDM_CURSOR cursor = NULL;
                    INFO_MYCHAR_KEY keyValue;
                    /* Get a key cursor positioned to the first row with the
                     * request key value */
                    strcpy (keyValue.mychar, infoData[3].mychar);
                    keyValue._mychar_has_value = RDM_COL_HAS_VALUE;
                    rc = rdm_dbGetRowsByKeyAtKey (
                        db, KEY_INFO_MYCHAR, &keyValue, sizeof (keyValue),
                        &cursor);
                    if (rc == sOKAY)
                    {
                        rc = rdm_cursorReadRow (
                            cursor, &sInfo, sizeof (sInfo), NULL);

                        if (rc == sOKAY)
                        {
                            printf (
                                "Row found doing a key lookup: %s\n",
                                sInfo.mychar);
                        }

                        /* Do a closest key match lookup, and return the rest in
                         * key order. */
                        if (rc == sOKAY)
                        {
                            char keyVal[34] = "Jim";
                            rc = rdm_cursorMoveToKey (
                                cursor, KEY_INFO_MYCHAR, keyVal,
                                sizeof (keyVal));
                        }

                        /* If we didn't get en exact match move to the next
                         * close key value */
                        if (rc == sNOTFOUND)
                        {
                            rc = rdm_cursorMoveToNext (cursor);
                        }

                        while (rc == sOKAY)
                        {
                            rc = rdm_cursorReadRow (
                                cursor, &sInfo, sizeof (sInfo), NULL);
                            if (rc == sOKAY)
                            {
                                printf (
                                    "Row found doing a key scan lookup: %s\n",
                                    sInfo.mychar);
                                rc = rdm_cursorMoveToNext (cursor);
                            }
                        }

                        /* We expect to break out of the loop with a
                         * sENDOFCURSOR return code */
                        if (rc == sENDOFCURSOR)
                        {
                            rc = sOKAY;
                        }
                        /* Free the cursor allocated by rdm_dbGetRowsByKeyAt Key
                         */
                        rdm_cursorFree (cursor);
                    }
                    /* End The read */
                    rdm_dbEnd (db);
                }

                /* Free the Database Handle */
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

RDM_STARTUP_EXAMPLE (core17ExampleTutorial)
