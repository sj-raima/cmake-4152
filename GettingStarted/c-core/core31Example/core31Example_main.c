/*
 * ONE-TO-MANY
 * -----------
 *
 * This document describes the process to create a simple one-to-many
 * relationship between two tables in a RDM database. We'll insert
 * rows and read them back from the database.
 */

#include "rdm.h" /* The RDM API.                 */
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core31Example_structs.h" /* The core31Example database definitions. */
#include "core31Example_cat.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <stdlib.h>

static const ONE ones[] = {{"John"}, {"James"}, {"Duncan"}};

static const MANY johns[] = {
    {"John's First", "John", RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"John's Second", "John", RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"John's Third", "John", RDM_COL_HAS_VALUE, RDM_COL_HAS_VALUE},
    {"", "John", RDM_COL_IS_NULL, RDM_COL_HAS_VALUE}};
static const MANY no_one[] = {
    {"No One's First", "", RDM_COL_HAS_VALUE, RDM_COL_IS_NULL},
    {"No One's Second", "", RDM_COL_HAS_VALUE, RDM_COL_IS_NULL}};
static const MANY duncans[] = {
    {"Duncan's First", "", RDM_COL_HAS_VALUE, RDM_COL_IS_NULL},
    {"Duncan's Second", "", RDM_COL_HAS_VALUE, RDM_COL_IS_NULL}};

RDM_EXPORT int32_t main_core31ExampleTutorial (int32_t argc, const char *const *argv)
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

    /* Allocate a database hande */
        if (rc == sOKAY)
        {
            RDM_DB db;
            rc = rdm_tfsAllocDatabase (tfs, &db);
            if (rc == sOKAY)
            {
                RDM_TABLE_ID tables[] = {TABLE_ONE, TABLE_MANY};
                RDM_TABLE_ID table_one[] = {TABLE_ONE};
                RDM_TABLE_ID table_many[] = {TABLE_MANY};

                rc = rdm_dbSetCatalog (db, core31Example_cat);

                if (rc == sOKAY)
                {
                    /* Open the database. */
                    rc = rdm_dbOpen (db, "core31", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("Can't open the core31 database.");
                    }
                }
                if (rc == sOKAY)
                {
                    /* Start an update transaction and lock ALL tables */
                    rc = rdm_dbStartUpdate (db, RDM_LOCK_ALL, 0, NULL, 0, NULL);
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

                /* Add all of the rows to the one table*/
                if (rc == sOKAY)
                {
                    /* Start an transaction and lock the "one" table for updates
                     */
                    rc = rdm_dbStartUpdate (
                        db, table_one, RDM_LEN (table_one), NULL, 0, NULL);
                }
                if (rc == sOKAY)
                {
                    uint32_t iO;
                    for (iO = 0; iO < RDM_LEN (ones) && rc == sOKAY; iO++)
                    {
                        /* Insert rows into the "one" table */
                        rc = rdm_dbInsertRow (
                            db, TABLE_ONE, &ones[iO], sizeof (ones[iO]), NULL);
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

                /* Add John's rows to the many table using referential integrity
                 */
                if (rc == sOKAY)
                {
                    /* Start a transaction and lock the "many" table for
                     * updates, and the "one" table for reads */
                    rc = rdm_dbStartUpdate (
                        db, table_many, RDM_LEN (table_many), table_one,
                        RDM_LEN (table_one), NULL);
                }
                if (rc == sOKAY)
                {
                    uint32_t iM;
                    for (iM = 0; iM < RDM_LEN (johns) && rc == sOKAY; iM++)
                    {
                        /* Insert rows into the "many" table */
                        rc = rdm_dbInsertRow (
                            db, TABLE_MANY, &johns[iM], sizeof (johns[iM]),
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

                /* Add "No One's" rows to the many table */
                if (rc == sOKAY)
                {
                    /* Start an transaction and lock the "many" table for
                     * updates */
                    rc = rdm_dbStartUpdate (
                        db, table_many, RDM_LEN (table_many), NULL, 0, NULL);
                }
                if (rc == sOKAY)
                {
                    uint32_t iM;
                    for (iM = 0; iM < RDM_LEN (no_one) && rc == sOKAY; iM++)
                    {
                        /* Insert rows into the "many" table */
                        rc = rdm_dbInsertRow (
                            db, TABLE_MANY, &no_one[iM], sizeof (no_one[iM]),
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

                /* Add "Duncan's" rows to the many table, assocate the with */
                if (rc == sOKAY)
                {
                    /* Start an transaction and lock the "many" table for
                       updates and the "one" table for reading */
                    rc = rdm_dbStartUpdate (
                        db, table_many, RDM_LEN (table_many), table_one,
                        RDM_LEN (table_one), NULL);
                }
                if (rc == sOKAY)
                {
                    RDM_CURSOR curOne = NULL;
                    RDM_CURSOR curMany = NULL;
                    uint32_t iM;

                    /* Get a cursor that is position to the "Duncan" row of the
                     * one table */
                    rc = rdm_dbGetRowsByKeyAtKey (
                        db, KEY_ONE_MYCHAR, &ones[2], sizeof (ones[2]),
                        &curOne);

                    for (iM = 0; iM < RDM_LEN (no_one) && rc == sOKAY; iM++)
                    {
                        /* Insert rows into the "many" table */
                        rc = rdm_dbInsertRow (
                            db, TABLE_MANY, &duncans[iM], sizeof (duncans[iM]),
                            &curMany);
                        if (rc == sOKAY)
                        {
                            rc = rdm_cursorLinkRow (
                                curMany, REF_MANY_MYCHAR_ONE, curOne);
                        }
                    }

                    /* Free the cursors used for link the one and many rows */
                    rdm_cursorFree (curOne);
                    rdm_cursorFree (curMany);
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

                    /* Display all of the rows in many table */
                    if (rc == sOKAY)
                    {
                        /* Start a transaction and lock the tables for reads */
                        rc = rdm_dbStartRead (
                            db, tables, RDM_LEN (tables), NULL);
                    }
                    if (rc == sOKAY)
                    {
                        RDM_CURSOR cursor = NULL;
                        rc = rdm_dbGetRows (db, TABLE_MANY, &cursor);
                        if (rc == sOKAY)
                        {
                            /* Navigate to the first row in the cursor */
                            printf ("Displaying all of the rows in the many "
                                    "table\n");
                            printf ("many.mychar_one       many.mychar\n");
                            printf (
                                "____________________  ____________________\n");
                            rc = rdm_cursorMoveToFirst (cursor);
                        }
                        while (rc == sOKAY)
                        {
                            MANY sMany;

                            /* Read the row column values */
                            rc = rdm_cursorReadRow (
                                cursor, &sMany, sizeof (sMany), NULL);
                            if (rc == sOKAY)
                            {
                                printf (
                                    "%-20s  %-20s\n",
                                    sMany._mychar_one_has_value ==
                                            RDM_COL_IS_NULL
                                        ? "**NULL**"
                                        : sMany.mychar_one,
                                    sMany._mychar_has_value == RDM_COL_IS_NULL
                                        ? "**NULL**"
                                        : sMany.mychar);

                                /* Move to the next row in the cursor */
                                rc = rdm_cursorMoveToNext (cursor);
                            }
                        }

                        /* We expect to break out of the loop with a
                         * sENDOFCURSOR code*/
                        if (rc == sENDOFCURSOR)
                        {
                            rc = sOKAY;
                        }

                        /* Free the cursor allocated in rdm_dbGetRows */
                        rdm_cursorFree (cursor);
                        /* release the read locks */
                        rdm_dbEnd (db);
                    }

                    /* Display all of the rows in one table, with all of the
                       many rows associated with a one row */
                    if (rc == sOKAY)
                    {
                        /* Start a transaction and lock the tables for reads */
                        rc = rdm_dbStartRead (
                            db, tables, RDM_LEN (tables), NULL);
                    }
                    if (rc == sOKAY)
                    {
                        RDM_CURSOR cursorOne = NULL;
                        RDM_CURSOR cursorMany = NULL;
                        rc = rdm_dbGetRows (db, TABLE_ONE, &cursorOne);
                        if (rc == sOKAY)
                        {
                            /* Navigate to the first row in the cursor */
                            printf ("\n\nDisplaying all of the rows in the one "
                                    "table (and their associated many rows)\n");
                            rc = rdm_cursorMoveToFirst (cursorOne);
                        }
                        while (rc == sOKAY)
                        {
                            ONE sOne;
                            /* Read the row column values */
                            rc = rdm_cursorReadRow (
                                cursorOne, &sOne, sizeof (sOne), NULL);
                            if (rc == sOKAY)
                            {
                                printf ("%s\n", sOne.mychar);

                                /* Get a cursor containing the many rows
                                   associated with the current one row */
                                rc = rdm_cursorGetMemberRows (
                                    cursorOne, REF_MANY_MYCHAR_ONE,
                                    &cursorMany);
                            }
                            if (rc == sOKAY)
                            {
                                rc = rdm_cursorMoveToFirst (cursorMany);
                            }
                            while (rc == sOKAY)
                            {
                                MANY sMany;

                                rc = rdm_cursorReadColumn (
                                    cursorMany, COL_MANY_MYCHAR, sMany.mychar,
                                    sizeof (sMany.mychar), NULL);
                                if (rc == sOKAY)
                                {
                                    printf ("    %s\n", sMany.mychar);
                                }
                                else if (rc == sNULLVAL)
                                {
                                    printf ("    **NULL**\n");
                                    rc = sOKAY;
                                }
                                if (rc == sOKAY)
                                {
                                    rc = rdm_cursorMoveToNext (cursorMany);
                                }
                            }

                            /* We will break out of the while loop with
                             * sENDOFCURSOR return code */
                            if (rc == sENDOFCURSOR)
                            {
                                rc = sOKAY;
                            }

                            /* Move to the next row in the cursor */
                            if (rc == sOKAY)
                            {
                                rc = rdm_cursorMoveToNext (cursorOne);
                            }
                        }

                        /* We expect to break out of the loop with a
                         * sENDOFCURSOR code*/
                        if (rc == sENDOFCURSOR)
                        {
                            rc = sOKAY;
                        }

                        /* Free the cursors allocated in rdm_dbGetRows and
                         * rdm_cursorGetMemberRows */
                        rdm_cursorFree (cursorOne);
                        rdm_cursorFree (cursorMany);

                        /* release the read locks */
                        rdm_dbEnd (db);
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

RDM_STARTUP_EXAMPLE (core31ExampleTutorial)
