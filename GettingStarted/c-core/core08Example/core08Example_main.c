#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core08_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core08_cat.h"

RDM_CMDLINE cmd;
const char *const description = "Demonstrates using an index for ordering";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core08 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (all existing contents removed)
 * \li Several new offices are added to the NA database
 * \li Several new offices are added to the EMEA database
 * \li Data is queried from the NA office
 * \li Data is queried from the EMEA office
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
 * in this example is located in the file \c core08.sdl.
 *
 * \include core08.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s option
 * to generate C-structures for interfacing with the database.
 *
 * \code rdm-compile -s core08.sdl \endcode
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

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);

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

        /* Expect rc to be S_NOTFOUND when we exit the loop */
        if (rc == sENDOFCURSOR)
        {
            rc = sOKAY;
        }
        rdm_dbEnd (hDB);
    }
    return rc;
}

RDM_RETCODE insertOffices (RDM_DB hDB, const char **officeList, size_t listSize)
{
    RDM_RETCODE rc;
    OFFICE office_rec;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        int ii;
        for (ii = 0; ii < (int) listSize; ii++)
        {
            strncpy (office_rec.name, officeList[ii], sizeof (office_rec.name));
            rc = rdm_dbInsertRow (
                hDB, TABLE_OFFICE, &office_rec, sizeof (office_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }
    return rc;
}

static const char *na_office_names[] = {"Seattle", "Boise", "San Francisco",
                                        "Dallas"};
static const char *emea_office_names[] = {"Paris", "London", "Dublin", "Zurich",
                                          "Madrid"};

/* \brief Macro to determine number of elements in an array of ptrs */
#define RLEN(x) (sizeof (x) / sizeof (x[0]))

/* \brief Main function for core08 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core08 (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
    RDM_TFS hTFS;
    RDM_DB hNADB = NULL;
    RDM_DB hEMEADB = NULL;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        rc = exampleAllocTFS (&hTFS);

        /* Open the NA database for exclusive access, this will create the
         * database dictionary. */
        if (rc == sOKAY)
            rc = exampleOpenNextEmptyDatabase (
                hTFS, &hNADB, "core08NA", core08_cat);

        /* Open the EMEA database, this will create the database
         * dictionary. */
        if (rc == sOKAY)
            rc = exampleOpenNextEmptyDatabase (
                hTFS, &hEMEADB, "core08EMEA", core08_cat);

        /* create records in the NA database */
        if (rc == sOKAY)
            rc = insertOffices (hNADB, na_office_names, RLEN (na_office_names));

        /* create records in the EMEA database */
        if (rc == sOKAY)
            rc = insertOffices (
                hEMEADB, emea_office_names, RLEN (emea_office_names));

        /* Display all offices in North America */
        printf ("\nNorth American Offices\n");
        if (rc == sOKAY)
            rc = display_offices (hNADB);

        /* Display All offices in EMEA */
        printf ("\nEMEA Offices\n");
        if (rc == sOKAY)
            rc = display_offices (hEMEADB);

        /* close the database */
        rdm_dbFree (hEMEADB);
        rdm_dbFree (hNADB);
        rdm_tfsFree (hTFS);
    }

    return 0;
}

RDM_STARTUP_EXAMPLE (core08)
