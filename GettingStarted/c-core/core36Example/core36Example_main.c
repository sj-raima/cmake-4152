/*
 * HELLO WORLD
 * -----------
 *
 * This document describes the process to create a simple database,
 * insert a record containing a text field, read the text field from
 * database and print it out.
 */

#include "rdm.h"                 /* The RDM API. */
#include "core36Example_structs.h" /* The core01Example database definitions. */
#include "core36Example_cat.h"     /* The core01Example database catalog */
#include "rdmstartupapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBNAME "core36"

static void cleanup(RDM_TFS tfs)
{
    rdm_tfsDropDatabase(tfs, DBNAME);
}

static void testEncryption(RDM_TFS tfs)
{
    RDM_RETCODE rc;

    RDM_DB db; /* Database Handle */

    /* Allocate a database hande */
    rc = rdm_tfsAllocDatabase(tfs, &db);
    if (rc == sOKAY)
    {
        /* Open the database */
        rc = rdm_dbOpen(db, DBNAME, RDM_OPEN_SHARED);
        if (rc == eINVENCRYPT)
        {
            printf("Database IS encrypted\n");
        }
        else
        {
            printf("Database IS NOT encrypted\n");
            rdm_dbClose(db);
        }
        rdm_dbFree(db);
    }
}

/* Read "helloworld" row to database */
static RDM_RETCODE readARow (RDM_DB db)
{
    RDM_TABLE_ID tables[] = {TABLE_INFO};
    RDM_RETCODE rc;

    /* Start an update transaction and lock the table */
    rc = rdm_dbStartRead (db, tables, RDM_LEN (tables), NULL);
    if (rc == sOKAY)
    {
        RDM_CURSOR cursor;

        rc = rdm_dbAllocCursor (db, &cursor);
        if (rc == sOKAY)
        {
            /* Fill a cursor with all the rows in a table */
            rc = rdm_dbGetRows (db, TABLE_INFO, &cursor);
            if (rc == sOKAY)
            {
                /* Move to the first row in the info table */
                rc = rdm_cursorMoveToFirst (cursor);
            }
            if (rc == sOKAY)
            {
                INFO infoRead; /* Row buffer */

                /* Read the full content of the current record */
                rc = rdm_cursorReadRow (
                    cursor, &infoRead, sizeof (infoRead), NULL);
                if (rc == sOKAY)
                {
                    printf ("%s\n", infoRead.mychar);
                }
            }
            /* Free the cursor */
            rdm_cursorFree (cursor);
        }
        /* Free the read lock on the table */
        rdm_dbEnd (db);
    }
    return rc;
}

/* Insert "helloworld" row to database */
static RDM_RETCODE writeARow (RDM_DB db)
{
    RDM_TABLE_ID tables[] = {TABLE_INFO};
    RDM_RETCODE rc;

    /* Start an update transaction and lock the table */
    rc = rdm_dbStartUpdate (db, tables, RDM_LEN (tables), NULL, 0, NULL);
    if (rc == sOKAY)
    {
        INFO infoInserted; /* Row buffer */

        /* Populate the mychar column in the Row buffer */
        strcpy (infoInserted.mychar, "Hello World! - using the embedded TFS");

        /* Insert a row into the table */
        rc = rdm_dbInsertRow (
            db, TABLE_INFO, &infoInserted, sizeof (infoInserted), NULL);
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

RDM_EXPORT int32_t main_hello_worldTutorial (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc; /* Status/Error Return Code */
    RDM_TFS tfs;    /* TFS Handle */
    RDM_ENCRYPT enc;

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS (&tfs);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (tfs);
        if (rc == sOKAY)
        {
            /* Allocate an encryption handle */
#if defined(LINK_ENC_SIMPLE)
            rc = rdm_tfsAllocEncrypt(tfs, "XOR:ThisIsThePassCode", &enc);
#else
            rc = rdm_tfsAllocEncrypt(tfs, "AES256:ThisIsThePassCode", &enc);
#endif
            if (rc == sOKAY)
            {
                RDM_DB db; /* Database Handle */

                /* Allocate a database hande */
                rc = rdm_tfsAllocDatabase (tfs, &db);
                if (rc == sOKAY)
                {
                    /* Associage the encryption context with the database handle */
                    rc = rdm_dbSetEncrypt (db, enc);
                    if (rc == sOKAY)
                    {
                        /* Associate the catalog with the database handle */
                        rc = rdm_dbSetCatalog(db, core36Example_cat);
                        if (rc == sOKAY)
                        {
                            /* Open the database */
                            rc = rdm_dbOpen(db, DBNAME, RDM_OPEN_SHARED);
                            if (rc != sOKAY)
                            {
                                printf("I can't open the core36Example database.\n");
                            }
                            else
                            {
                                /* Insert "helloworld" row to database */
                                rc = writeARow(db);
                                if (rc == sOKAY)
                                {
                                    /* Read "helloworld" row to database */
                                    rc = readARow(db);
                                }
                                rdm_dbClose(db);
                            }
                        }
                    }
                    rdm_dbFree (db);
                }
                rdm_encryptFree (enc);
            }
            testEncryption(tfs);
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

RDM_STARTUP_EXAMPLE (hello_worldTutorial)
