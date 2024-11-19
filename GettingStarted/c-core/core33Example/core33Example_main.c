/*
 * UNIQUE KEY
 * ----------
 *
 * This document describes the process to create a simple database
 * containing a unique key, insert records, and read data through the
 * key.
 *
 * For a full description of RDM keys, see Database Design in the
 * Users Guide.
 *
 * This example contains a single record type with a single field
 * which is designated as a unique key. This means that every record
 * of type info that is successfully inserted into the database will
 * have a unique value. If an insertion is attempted with a key field
 * value that already exists in the database, the insertion is
 * rejected and an error code is returned from the d_fillnew function.
 */

#include "rdm.h"
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core33Example_structs.h"
#include "core33Example_cat.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char szData[][34] = {
    "John", "James", "Duncan",
    "Bill", "John", /* Inserting this should fail, referential integrity rules
                       violated. */
    "Paul", "David"};

RDM_EXPORT int32_t main_core33ExampleTutorial (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
    uint32_t ii;
    INFO sInfo;
    RDM_TFS tfs;
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
            RDM_DB db;
            /* Allocate a database hande */
            rc = rdm_tfsAllocDatabase (tfs, &db);
            if (rc == sOKAY)
            {
                rc = rdm_dbSetCatalog (db, core33Example_cat);

                if (rc == sOKAY)
                {
                    /* Open the database. */
                    rc = rdm_dbOpen (db, "core33", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("Can't open the core33 database.");
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
                    /* Start an update transaction and lock the table */
                    rc = rdm_dbStartUpdate (
                        db, tables, RDM_LEN (tables), NULL, 0, NULL);
                }
                if (rc == sOKAY)
                {
                    /* Add rows to the info table */
                    for (ii = 0; ii < RDM_LEN (szData) && rc == sOKAY; ii++)
                    {
                        strcpy (sInfo.mychar, szData[ii]);
                        sInfo._mychar_has_value = RDM_COL_HAS_VALUE;
                        rc = rdm_dbInsertRow (
                            db, TABLE_INFO, &sInfo, sizeof (sInfo), NULL);

                        if (rc == eDUPLICATE)
                        {
                            printf (
                                "%s is a duplicate record.\n", sInfo.mychar);
                            rc = sOKAY; /* This was expected, re-set error code.
                                         */
                        }
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
                    INFO_MYCHAR_KEY keyVal1 = {"", RDM_COL_HAS_VALUE};

                    /* Get a key cursor positioned to the first row with the
                     * request key value */
                    strcpy (keyVal1.mychar, szData[3]);
                    rc = rdm_dbGetRowsByKeyAtKey (
                        db, KEY_INFO_MYCHAR, &keyVal1, sizeof (keyVal1), &cursor);
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
                            INFO_MYCHAR_KEY keyVal2 = {"Jim", RDM_COL_HAS_VALUE};
                            rc = rdm_cursorMoveToKey (
                                cursor, KEY_INFO_MYCHAR, &keyVal2,
                                sizeof (keyVal2));

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
                                        "Row found doing a key scan lookup: "
                                        "%s\n",
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
                        }

                        /* Free the cursor allocated by rdm_dbGetRowsByKeyAtKey
                         */
                        rdm_cursorFree (cursor);
                    }
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

RDM_STARTUP_EXAMPLE (core33ExampleTutorial)
