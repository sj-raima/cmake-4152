#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core05_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core05_cat.h"

RDM_CMDLINE cmd;
const char *const description = "Demonstrates using an index for ordering";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core05 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (if necessary)
 * \li 5 new person records are added to the database
 * \li A list of each person is displayed in index order.
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
 * in this example is located in the file \c core05.sdl.
 *
 * \include core05.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s option
 * to generate C-structures for interfacing with the database.
 *
 * \code rdm-compile -s core05.sdl \endcode
 *
 * Each of these functions returns an integer status code, where the
 * value sOKAY indicates a successful call.
 *
 * The actual database will be stored in a directory named 'core05.rdm' in the
 * project directory.
 *
 * \page hPGMfunc Program Functions
 * \li main() - \copybrief main
 */

/* \brief Insert one row into the core02 database
 *
 * This function adds one more row to the core02 database. The row contains only
 * a non-indexed character string.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertFiveRows (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    PERSON person_rec;
    RDM_RETCODE rc;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        int ii;

        printf ("Adding 5 new person records\n");
        for (ii = 0; ii < 5; ii++)
        {
            unsigned int value;

            value = (unsigned int) rand () % 10000;
            sprintf (person_rec.last_four_ssn, "%04u", value);
            rc = rdm_dbInsertRow (
                hDB, TABLE_PERSON, &person_rec, sizeof (person_rec), NULL);
            print_error (rc);
            printf ("\t%s\n", person_rec.last_four_ssn);
        }

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }

    return rc;
}

/* \brief Reads and displays all rows in the core02 database
 *
 * This function reads and displays all rows in the core02 database. Each time
 * the core02 program is executed, an additional row will be added to the
 * database.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE readAllRows (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    PERSON person_rec;
    RDM_CURSOR cursor = NULL;
    RDM_RETCODE rc;

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        printf ("\nList all records in index order\n");
        rc = rdm_dbGetRowsByKey (hDB, KEY_PERSON_LAST_FOUR_SSN, &cursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
                 rc = rdm_cursorMoveToNext (cursor))
            {
                rc = rdm_cursorReadRow (
                    cursor, &person_rec, sizeof (person_rec), NULL);
                print_error (rc);

                printf ("%s\n", person_rec.last_four_ssn);
            }
            rdm_cursorFree (cursor);
        }
        rdm_dbEnd (hDB);

        /* We expect rc to be sENDOFCURSOR when we break out of the loop */
        if (rc == sENDOFCURSOR)
        {
            rc = sOKAY; /* change status to sOKAY because sENDOFCURSOR was
                           expected. */
        }
        else
        {
            print_error (rc);
        }
    }
    return rc;
}

/* \brief Main function for core05 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the \c RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core05 (int32_t argc, const char *const *argv)
{
    RDM_TFS hTFS;
    RDM_DB hDB;
    RDM_RETCODE rc;

    /* Seed the random number generator */
    srand ((unsigned int) time (NULL));

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        rc = exampleOpenDatabase (&hTFS, &hDB, "core05", core05_cat);
        if (rc == sOKAY)
        {
            rc = insertFiveRows (hDB);
            if (rc == sOKAY)
            {
                rc = readAllRows (hDB);
            }
            exampleCleanup (hTFS, hDB);
        }
    }

    return (int) rc;
}

RDM_STARTUP_EXAMPLE (core05)
