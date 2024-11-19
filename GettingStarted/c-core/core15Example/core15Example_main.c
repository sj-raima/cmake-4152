/*
 * CIRCULAR TABLE
 * --------------
 * This document describes the process to create a circular record in a
 * database, insert a circular record, and read data from this database.
 */

#include "rdm.h" /* The RDM API.                 */
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core15Example_structs.h" /* The one_to_many database definitions. */
#include "core15Example_cat.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <stdlib.h>

static const ALBUM albums[] = {{"Eric Dolphy", RDM_COL_HAS_VALUE},
                               {"Miles Davis", RDM_COL_HAS_VALUE},
                               {"Charles Mingus", RDM_COL_HAS_VALUE},
                               {"Chico Hamilton", RDM_COL_HAS_VALUE},
                               {"John Coltrane", RDM_COL_HAS_VALUE}};

RDM_EXPORT int32_t main_circular_tableTutorial (int32_t argc, const char *const *argv)
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
                RDM_TABLE_ID tables[] = {TABLE_ALBUM};
                uint32_t ii;

                rc = rdm_dbSetCatalog (db, core15Example_cat);
                if (rc == sOKAY)
                {
                    /* Open the database. */
                    rc = rdm_dbOpen (db, "core15", RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf ("I can't open the core15 database.");
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

                /* Insert rows into the circular table */
                for (ii = 0; ii < RDM_LEN (albums) && rc == sOKAY; ii++)
                {
                    RDM_CURSOR cursor = NULL;

                    /* Start an update transaction and lock the table */
                    rc = rdm_dbStartUpdate (
                        db, tables, RDM_LEN (tables), NULL, 0, NULL);
                    if (rc == sOKAY)
                    {
                        rc = rdm_dbInsertRow (
                            db, TABLE_ALBUM, &albums[ii], sizeof (albums[ii]),
                            NULL);

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

                    /* Read and dump all album records. */
                    if (rc == sOKAY)
                    {
                        printf ("\nDUMPING ALL ROWS\n");
                        rc = rdm_dbGetRows (db, TABLE_ALBUM, &cursor);
                    }
                    if (rc == sOKAY)
                    {
                        /* Lock the table for reading */
                        rc = rdm_dbStartRead (
                            db, tables, RDM_LEN (tables), NULL);
                        if (rc == sOKAY)
                        {
                            for (rc = rdm_cursorMoveToFirst (cursor);
                                 rc == sOKAY;
                                 rc = rdm_cursorMoveToNext (cursor))
                            {
                                ALBUM sAlbum;
                                rc = rdm_cursorReadRow (
                                    cursor, &sAlbum, sizeof (sAlbum), NULL);
                                if (rc == sOKAY)
                                {
                                    printf ("   %s\n", sAlbum.artist);
                                }
                            }
                            /* Reset the rc value, the for loop will end with a
                             * sENDOFCURSOR when there are no more rows. */
                            if (rc == sENDOFCURSOR) /*lint !e850 */
                            {
                                rc = sOKAY;
                            }

                            /* Free the read locks */
                            rdm_dbEnd (db);
                        }

                        /* Free the cursor */
                        rdm_cursorFree (cursor);
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

RDM_STARTUP_EXAMPLE (circular_tableTutorial)
