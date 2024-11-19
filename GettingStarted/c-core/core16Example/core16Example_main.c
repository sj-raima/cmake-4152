/*
 * COMPOUND KEY
 * ------------
 *
 * This document describes the process to create a simple database
 * containing a key, insert records, and read data through the key.
 *
 * For a full description of RDM keys, see Database Design in the
 * Users Guide.
 *
 * This example contains a single record type with two fields which
 * are referenced in a non-unique compound key.
 */

#include "rdm.h"
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core16Example_structs.h"
#include "core16Example_cat.h"
#include "rdmstartupapi.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const INFO infos[] = {
    {"John", 30, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"James", 33, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"Duncan", 37, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"Bill", 44, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"John", 27, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"Paul", 21, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"David", 27, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"David", 27, RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE}};

RDM_EXPORT int32_t main_core16ExampleTutorial (int32_t argc, const char *const *argv)
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

        /* Allocate a database hande */
        if (rc == sOKAY)
        {
            RDM_DB db;
            rc = rdm_tfsAllocDatabase (tfs, &db);
            if (rc == sOKAY)
            {
                RDM_TABLE_ID tables[] = {TABLE_INFO};
                RDM_CURSOR cursor = NULL;
                RDM_SEARCH_KEY key = {0};
                INFO_MYKEY_KEY mykey = {"", 0, RDM_COL_HAS_VALUE,
                                        RDM_COL_HAS_VALUE};

                rc = rdm_dbSetCatalog (db, core16Example_cat);
                if (rc == sOKAY)
                {
                    /* Open the database. */
                    rc = rdm_dbOpen (db, "core16", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("I can't open the core16 database.");
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

                if (rc == sOKAY)
                {
                    rc = rdm_dbStartUpdate (
                        db, tables, RDM_LEN (tables), NULL, 0, NULL);
                }
                if (rc == sOKAY)
                {
                    uint32_t ii;
                    /* Insert the rows in the table */
                    for (ii = 0; ii < RDM_LEN (infos) && rc == sOKAY; ii++)
                    {
                        rc = rdm_dbInsertRow (
                            db, TABLE_INFO, &infos[ii], sizeof (infos[ii]),
                            NULL);
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
                    /* Do a key search on all 'John' records.  We will set
                       the numKeyCols to 1 so that we will only use the name
                       portion of the key.  We will retrieve all rows that
                       have a mychar value of 'John' and a myage value of
                       anything */
                    strcpy (mykey.mychar, "John");
                    mykey._mychar_has_value = RDM_COL_HAS_VALUE;
                    key.value = &mykey;
                    key.bytesIn = sizeof (mykey);
                    key.numKeyCols = 1;
                    key.stringLen = 0;

                    /* get a lock on the table */
                    rc = rdm_dbStartRead (db, tables, RDM_LEN (tables), NULL);
                }
                if (rc == sOKAY)
                {
                    rc = rdm_dbGetRowsByKeyInSearchKeyRange (
                        db, KEY_INFO_MYKEY, &key, &key, &cursor);
                    if (rc == sOKAY)
                    {
                        rc = rdm_cursorMoveToFirst (cursor);

                        while (rc == sOKAY)
                        {
                            INFO sInfo = {"", 0, RDM_COL_IS_NULL,
                                          RDM_COL_IS_NULL};
                            /* Read the full content of the current row */
                            rc = rdm_cursorReadRow (
                                cursor, &sInfo, sizeof (sInfo), NULL);
                            if (rc == sOKAY)
                            {
                                printf (
                                    "Row found doing a search key range "
                                    "lookup: "
                                    "%s, %d\n",
                                    sInfo.mychar, sInfo.myage);
                            }

                            /* Go to the next row in key order. */
                            if (rc == sOKAY)
                            {
                                rc = rdm_cursorMoveToNext (cursor);
                            }
                        }

                        /* We expect to break out of the loop when rc =
                         * sENDOFCURSOR
                         */
                        if (rc == sENDOFCURSOR)
                        {
                            rc = sOKAY;
                        }

                        /* Free the cursor allocated in the rdm_dbGet call */
                        rdm_cursorFree (cursor);
                    }

                    /* Free the lock on the table */
                    rdm_dbEnd (db);
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

RDM_STARTUP_EXAMPLE (core16ExampleTutorial)
