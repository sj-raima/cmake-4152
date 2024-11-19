#include "rdm.h"
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "rdmbcdapi.h"
#include "core18Example_structs.h"
#include "core18Example_cat.h"
#include "pspplatpackage.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <stdlib.h>

RDM_EXPORT int32_t main_encryptionTutorial (int32_t argc, const char *const *argv)
{
    RDM_TFS tfs;
    RDM_RETCODE rc;

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS (&tfs);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (tfs);
        if (rc == sOKAY)
        {
            RDM_ENCRYPT enc;

            /* Allocate an encryption handle */
#if defined(LINK_ENC_SIMPLE)
            rc = rdm_tfsAllocEncrypt (tfs, "XOR:testing", &enc);
#else
            rc = rdm_tfsAllocEncrypt (tfs, "AES128:testing", &enc);
#endif
            if (rc == sOKAY)
            {
                RDM_DB db;
                /* Allocate a database hande */
                rc = rdm_tfsAllocDatabase (tfs, &db);

                if (rc == sOKAY)
                {
                    RDM_TABLE_ID tables[] = {TABLE_ENCRYPTIONTABLE};
                    ENCRYPTIONTABLE insertions[3];
                    RDM_CURSOR cursor = NULL;
                    bool decrypt = false;

                    /* Associate the catalog with the database handle */
                    rc = rdm_dbSetCatalog (db, core18Example_cat);

                    if (rc == sOKAY)
                    {
                        /* Open the database - try without any encryption */
                        rc = rdm_dbOpen (
                            db, "core18", RDM_OPEN_EXCLUSIVE);
                    }
                    if (rc == sOKAY)
                    {
                        printf ("Opened the core18 database to be "
                                "encrypted.\n");
                    }
                    else if (rc == eINVENCRYPT)
                    {
                        /* The database is already encrypted so we will decrypt
                           it, start by Associating the encryption handle with
                           the database handle so we can open the encrypted
                           database */
                        rc = rdm_dbSetEncrypt (db, enc);
                        if (rc == sOKAY)
                        {
                            /* Open the database - try with encryption */
                            rc = rdm_dbOpen (
                                db, "core18", RDM_OPEN_EXCLUSIVE);
                            if (rc == sOKAY)
                            {
                                printf (
                                    "Opened the core18Example database "
                                    "to be decrypted.\n");
                                decrypt = true;
                            }
                        }
                    }

                    /* We will now either encrypt or decrypt the open database
                     */
                    if (rc == sOKAY)
                    {
                        if (decrypt == true)
                        {
                            /* Decrypt the database by specifying a NULL
                             * encryption handle */
                            rc = rdm_dbEncrypt (db, NULL, "");
                        }
                        else
                        {
                            /* Encrypt the database by specifying the encryption
                             * handle */
                            rc = rdm_dbEncrypt (db, enc, "");
                        }

                        /* Close the database */
                        rdm_dbClose (db);
                    }

                    /* If we encrypted the database we must add the encryption
                       key to the database handle in order to open it, if we
                       decrypted the database we don't need to do anything */
                    if (rc == sOKAY && decrypt == false)
                    {
                        rc = rdm_dbSetEncrypt (db, enc);
                    }

                    if (rc == sOKAY)
                    {
                        /* Open the database */
                        rc = rdm_dbOpen (
                            db, "core18", RDM_OPEN_SHARED);
                    }
                    if (rc == sOKAY)
                    {
                        /* Start a database transaction and remove any existing
                         * data. */
                        rc = rdm_dbStartUpdate (
                            db, tables, RDM_LEN (tables), NULL, 0, NULL);
                    }
                    if (rc == sOKAY)
                    {
                        /* Remove all rows from the database */
                        rc = rdm_dbDeleteAllRowsFromDatabase (db);

                        if (rc == sOKAY)
                        {
                            rc = rdm_dbEnd (db);
                        }
                        else
                        {
                            rdm_dbEndRollback (db);
                        }
                    }

                    /* Insert some rows into the table */
                    if (rc == sOKAY)
                    {
                        RDM_BCD_T rdmBCD;
                        (void) rdm_bcdFromString ("72.5", &rdmBCD);
                        insertions[0].id = rdmBCD;
                        (void) rdm_bcdFromInt32 (343, &rdmBCD);
                        insertions[1].id = rdmBCD;
                        (void) rdm_bcdFromDouble (1248.16, &rdmBCD);
                        insertions[2].id = rdmBCD;

                        /* Start a database transaction to insert some rows. */
                        rc = rdm_dbStartUpdate (
                            db, tables, RDM_LEN (tables), NULL, 0, NULL);
                    }
                    if (rc == sOKAY)
                    {
                        uint32_t counter;

                        for (counter = 0;
                             rc == sOKAY && counter < RDM_LEN (insertions);
                             counter++)
                        {
                            char outBuffer[100];
                            (void) rdm_bcdToString (
                                &insertions[counter].id, false, outBuffer,
                                sizeof (outBuffer), NULL);
                            printf ("BCD Inserted: %s\n", outBuffer);
                            rc = rdm_dbInsertRow (
                                db, TABLE_ENCRYPTIONTABLE, &insertions[counter],
                                sizeof (insertions[counter]), NULL);
                        }

                        if (rc == sOKAY)
                        {
                            rc = rdm_dbEnd (db);
                        }
                        else
                        {
                            rdm_dbEndRollback (db);
                        }
                    }
                    /* Read the rows from the table */
                    if (rc == sOKAY)
                    {
                        /* Allocate a cursor */
                        rc = rdm_dbAllocCursor (db, &cursor);
                    }
                    if (rc == sOKAY)
                    {
                        /* Start a read transaction */
                        rc = rdm_dbStartRead (
                            db, tables, RDM_LEN (tables), NULL);
                        if (rc == sOKAY)
                        {
                            /* Fill the cursor with rows */
                            if (rc == sOKAY)
                            {
                                rc = rdm_dbGetRows (
                                    db, TABLE_ENCRYPTIONTABLE, &cursor);
                            }

                            if (rc == sOKAY)
                            {
                                /* navigate to the first row in the cursor */
                                rc = rdm_cursorMoveToFirst (cursor);
                            }

                            while (rc == sOKAY)
                            {
                                ENCRYPTIONTABLE row;

                                /* Read the entire row */
                                rc = rdm_cursorReadRow (
                                    cursor, &row, sizeof (row), NULL);
                                if (rc == sOKAY)
                                {
                                    char outBuffer[100];
                                    (void) rdm_bcdToString (
                                        &row.id, false, outBuffer,
                                        sizeof (outBuffer), NULL);
                                    printf ("BCD Retrieved: %s\n", outBuffer);

                                    /* navigate to the next row in the cursor */
                                    rc = rdm_cursorMoveToNext (cursor);
                                }
                            }

                            /* We expect to break out of the loop with a
                             * sENDOFCURSOR */
                            if (rc == sENDOFCURSOR)
                            {
                                rc = sOKAY;
                            }
                            /* End the read transaction */
                            rdm_dbEnd (db);
                        }
                        /* free the cursor */
                        rdm_cursorFree (cursor);
                    }

                    rdm_dbFree (db);
                }
                rdm_encryptFree (enc);
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

RDM_STARTUP_EXAMPLE (encryptionTutorial)
