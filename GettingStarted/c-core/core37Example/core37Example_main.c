/*
 * HELLO WORLD
 * -----------
 *
 * This document describes the process to create a simple database,
 * insert a record containing a text field, read the text field from
 * database and print it out.
 */

#include "rdm.h"                 /* The RDM API. */
#include "core37Example_structs.h" /* The core01Example database definitions. */
#include "core37Example_cat.h"     /* The core01Example database catalog */
#include "rdmstartupapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBNAME "core37"
#define SAMPLETEXT "The quick brown fox jumps over the lazy dog. "

/* Remove the database to cleanup the example */
static void cleanup(RDM_TFS tfs)
{
    rdm_tfsDropDatabase(tfs, DBNAME);
}


static RDM_RETCODE updateClob(RDM_CURSOR cursor, RDM_COLUMN_ID col)
{
    RDM_RETCODE rc = sOKAY;
    char buffer[] = { SAMPLETEXT };
    uint64_t offset;
    size_t bufSize;

    /* Setting bufSize to the length of the actual data 
    *  rather than the length of the buffer. For this 
    *  example we are excluding the NULL byte at the end 
    *  of the input string so the strings will be appended 
    *  together.
    */
    bufSize = strlen(buffer);

    /* Appending characters into CLOB until approximately 2000 characters reached */
    for (offset = 0; offset < 2000 && rc == sOKAY; offset += bufSize)
    {
        rc = rdm_cursorUpdateBlob(cursor, col, offset, buffer, bufSize);
    }
    return rc;
}

static RDM_RETCODE readClob(RDM_CURSOR cursor, RDM_COLUMN_ID col)
{
    RDM_RETCODE rc = sOKAY;
    char buffer[1000];
    uint64_t offset;
    size_t bytesOut;
    size_t charactersOut;
    uint64_t numCharacters;

    /* Read entire contents of CLOB using 1000 byte buffer */
    rc = rdm_cursorGetClobSize(cursor, col, &numCharacters);
    if (rc == sOKAY && numCharacters > 0)
    {
        /* Only read blob if there are characters stored. A
         * zero numCharacters indicates the CLOB is empty.
         */
        for (offset = 0; rc == sOKAY; offset += charactersOut )
        {
            /* in the following, the bytesIn value is reduced by 1 character 
            *  to allow a NULL byte to be appended to the buffer later
            */
            rc = rdm_cursorReadClob(cursor, col, offset, buffer, sizeof(buffer)-1, &bytesOut, &charactersOut);
            if (bytesOut == 0)
                break;          /* bytesOut == 0 indicates end of CLOB has been reached */

            /* Since we put the string into the CLOB w/o a NULL terminator,
            *  we need to add it to the end of buffer we are printing.
            */
            buffer[bytesOut] = '\0';
            printf("%s", buffer);
        }
    }
    return rc;
}



static RDM_RETCODE readARow (RDM_DB db, uint32_t id)
{
    RDM_TABLE_ID tables[] = {TABLE_INFO};
    RDM_RETCODE rc;

    /* Start an update transaction and lock the table */
    rc = rdm_dbStartRead (db, tables, RDM_LEN (tables), NULL);
    if (rc == sOKAY)
    {
        INFO_ID_KEY idLookup;
        RDM_CURSOR cursor = NULL;

        /* Find the requested row */
        idLookup.id = id;
        rc = rdm_dbGetRowsByKeyAtKey(db, KEY_INFO_ID, &idLookup, sizeof(idLookup), &cursor);
        if (rc == sOKAY)
        {
            INFO infoRead; /* Row buffer */

            /* Read the full content of the current record */
            rc = rdm_cursorReadRow (cursor, &infoRead, sizeof (infoRead), NULL);
            if (rc == sOKAY)
            {
                printf ("ID\t: %08u\n", infoRead.id);
                printf ("Model\t: %s\n", infoRead.model);
                printf ("Comment\t: \n");
                if (infoRead._description_has_value == true)
                {
                    rc = readClob(cursor, COL_INFO_DESCRIPTION);
                }
                else
                {
                    puts("**NULL DESCRIPTION**");
                }
                printf("\n");
            }
        }
        if (cursor)
        {
            /* Free the cursor */
            rdm_cursorFree (cursor);
        }
        /* Free the read lock on the table */
        rdm_dbEnd (db);
    }
    return rc;
}


static RDM_RETCODE writeARow (RDM_DB db, uint32_t id)
{
    RDM_TABLE_ID tables[] = {TABLE_INFO};
    RDM_CURSOR cursor = NULL;
    RDM_RETCODE rc;

    /* Start an update transaction and lock the table */
    rc = rdm_dbStartUpdate (db, tables, RDM_LEN (tables), NULL, 0, NULL);
    if (rc == sOKAY)
    {
        INFO infoInserted; /* Row buffer */

        /* Populate the mychar column in the Row buffer */
        infoInserted.id = id;
        sprintf(infoInserted.model, "Model #%x", id);
        infoInserted._description_has_value = false;

        /* Insert a row into the table */
        rc = rdm_dbInsertRow (
            db, TABLE_INFO, &infoInserted, sizeof (infoInserted), &cursor);
        if (rc == sOKAY)
        {
            uint32_t task = (id % 2);

            /* The example will add a Description CLOB to every other ID */
            if (task == 0)
            {
                rc = updateClob(cursor, COL_INFO_DESCRIPTION);
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
    return rc;
}

RDM_EXPORT int32_t main_core37 (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc; /* Status/Error Return Code */
    RDM_TFS tfs;    /* TFS Handle */

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
                /* Associate the catalog with the database handle */
                rc = rdm_dbSetCatalog(db, core37Example_cat);
                if (rc == sOKAY)
                {
                    /* Open the database */
                    rc = rdm_dbOpen(db, DBNAME, RDM_OPEN_SHARED);
                    if (rc != sOKAY)
                    {
                        printf("I can't open the " DBNAME " database.\n");
                    }
                    else
                    {
                        /* insert 10 rows into the database */
                        for (uint32_t ii = 0; ii < 10 && rc == sOKAY; ii++)
                        {
                            rc = writeARow(db, 1000+ii);
                        }
                        /* Read ids #4 and #5 */
                        readARow(db, 1004);
                        readARow(db, 1005);

                        rdm_dbClose(db);
                    }
                }
                rdm_dbFree (db);
            }
            cleanup (tfs);
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

RDM_STARTUP_EXAMPLE (core37)
