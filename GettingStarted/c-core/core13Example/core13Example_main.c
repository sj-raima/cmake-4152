#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core13_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core13_cat.h"

RDM_CMDLINE cmd;
const char *const description = "Demonstrates using an index for ordering";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core13 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (all existing contents removed)
 * \li Two RDM tasks are created
 * \li Each task opens the database is shared mode
 * \li Task 1 starts a transaction, locks the office record for writes,
 *      adds a few office records
 * \li Task 2 attempts to read the office records (fails while task 1 has lock)
 * \li Task 1 commits transaction
 * \li Task 2 is able to read the office records
 *
 * \par Table of Contents
 *
 * - \subpage hDB
 * - \subpage hPGMfunc
 *
 * For additional information, please refer to the product documentation at
 * http://docs.raima.com/.
 *
 * \page hDB Database Schema
 *
 * \par Database Schema Definition
 *
 * The DDL (Database Definition Language) specification for the database used
 * in this example is located in the file \c core13.sdl.
 *
 * \include core13.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s option
 * to generate C-structures for interfacing with the database.
 *
 * \code rdm-compile -s core13.sdl \endcode
 *
 * Each of these functions returns an integer status code, where the
 * value sOKAY indicates a successful call.
 *
 * The actual databases will be stored in a directories named 'na_sales_db' and
 * 'emea_sales_db' in the project directory.
 *
 * \page hPGMfunc Program Functions
 *
 * For simplicity, this example does not check all return codes, but good
 * programming practices would dictate that they are checked after each
 * RDM call.
 *
 * \li openEmptyDatabase() - \copybrief openEmptyDatabase
 * \li display_offices() - \copybrief display_offices
 * \li main() - \copybrief main
 */

/* \brief Display the contents of the database
 *
 * This function displays the contents of the database.
 * Exclusve mode does not require database locks or transactions.
 *
 * @return Returns RDM_RETCODE status code (sOKAY if successful)
 */
RDM_RETCODE display_offices (RDM_DB hDB) /*< [in] Database handle to be used */
{
    RDM_RETCODE rc;
    OFFICE office_rec;
    RDM_CURSOR cursor = NULL;
    RDM_TRANS hTRN;

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, &hTRN);
    if (rc == eUNAVAIL)
    {
        puts("The OFFICE table is not available for READ because of a WRITE lock.");
    }
    else
    {
        print_error(rc);
    }

    if (rc == sOKAY)
    {
        /* The following cursor association call will allocate the cursor
         *  if the cursor is set to NULL. This short-cut can eliminate the
         *  requirement to call rdm_dbAllocCursor() before using the cursor
         *  in this function */
        rc = rdm_dbGetRowsByKey (hDB, KEY_OFFICE_NAME, &cursor);
        print_error (rc);

        for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
             rc = rdm_cursorMoveToNext (cursor))
        {
            /* Read and display the current person record */
            rc = rdm_cursorReadRow (
                cursor, &office_rec, sizeof (office_rec), NULL);
            print_error (rc);
            printf ("%s\n", office_rec.name);
        }

        /* free the cursor if it was allocated */
        if (cursor)
            rdm_cursorFree (cursor);

        /* Expect rc to be sENDOFCURSOR when we exit the loop */
        if (rc == sENDOFCURSOR)
        {
            rc = sOKAY;
        }
        rdm_transEnd (hTRN);
    }
    return rc;
}

RDM_RETCODE insertOffices (RDM_DB hDB, const char **officeList, size_t listSize)
{
    RDM_RETCODE rc = sOKAY;
    OFFICE office_rec;
    int ii;

    for (ii = 0; ii < (int) listSize; ii++)
    {
        strncpy (office_rec.name, officeList[ii], sizeof (office_rec.name));
        rc = rdm_dbInsertRow (
            hDB, TABLE_OFFICE, &office_rec, sizeof (office_rec), NULL);
        print_error (rc);
    }
    return rc;
}

static const char *office_names[] = {"Seattle", "Boise", "San Francisco",
                                     "Dallas"};

/* \brief Macro to determine number of elements in an array of ptrs */
#define RLEN(x) (sizeof (x) / sizeof (x[0]))

/* \brief Main function for core13 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core13 (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
    RDM_TFS hTFS;
    RDM_DB hDB1 = NULL;
    RDM_DB hDB2 = NULL;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        /* create records in the database */
        rc = exampleOpenEmptyDatabase (&hTFS, &hDB1, "core13", core13_cat);
        if (rc == sOKAY)
        {
            rc = exampleOpenNextDatabase (hTFS, &hDB2, "core13", core13_cat);
            if (rc == sOKAY)
            {
                /* set timeout to 1 second so we do not need to wait 10s for
                 * error */
                rc = rdm_dbSetOption (hDB2, "timeout", "1");
                print_error (rc);
            }
        }

        if (rc == sOKAY)
        {
            rc = rdm_dbStartUpdate (hDB1, RDM_LOCK_ALL, 0, NULL, 0, NULL);
            print_error (rc);

            if (rc == sOKAY)
            {
                rc = insertOffices (hDB1, office_names, RLEN (office_names));
                if (rc == sOKAY)
                {
                    printf ("\nDisplaying Offices using database handle #1.\n");
                    rc = display_offices (hDB1);
                }
                /* Display all offices using handle 2 (should fail because we
                 * will not be able to get a read lock */
                printf ("\nDisplaying Offices using database handle #2.\n");
                rc = display_offices (hDB2);

                rc = rdm_dbEnd (hDB1);
                print_error (rc);

                if (rc == sOKAY)
                {
                    /* Display all offices */
                    printf ("\nDisplaying Offices using database handle #2.\n");
                    rc = display_offices (hDB2);
                }
            }
            exampleCleanup2 (hTFS, hDB1, hDB2);
        }
    }
    return rc;
}

RDM_STARTUP_EXAMPLE (core13)
