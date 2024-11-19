#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core02Example_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core02Example_cat.h"

RDM_CMDLINE cmd;
const char *const description =
    "Demonstrates inserting a row and reading rows from database";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core02 Popcorn Example
 *
 * Each time you run this example, a new record is created in the
 * database, then all existing records are read from the database
 * and printed.
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
 * in this example is located in the file \c core02.sdl.
 *
 * \include core02.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with options
 * to generate C-structures and catalog files for interfacing with the database.
 * The option to generate lowercase struct member names was also used
 *
 * \code rdm-compile --c-structs --catalog --lc-struct-members core02.sdl
 * \endcode
 *
 * \page hPGMfunc Program Functions
 * \li insertOneRow() - \copybrief insertOneRow
 * \li readAllRows() - \copybrief readAllRows
 * \li main() - \copybrief main
 */

/* \brief Insert one row into the core02 database
 *
 * This function adds one more row to the core02 database. The row contains only
 * a non-indexed character string.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertOneRow (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    CORE02_RECORD core02_input;
    RDM_RETCODE rc;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        /* create a record in the new database */
        strncpy (core02_input.message, "Core02", sizeof (core02_input.message));
        rc = rdm_dbInsertRow (
            hDB, TABLE_CORE02_RECORD, &core02_input, sizeof (core02_input),
            NULL);
        print_error (rc);

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
    CORE02_RECORD core02_output;
    RDM_CURSOR cursor = NULL;
    RDM_RETCODE rc;

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        /* allocate a cursor resource */
        rc = rdm_dbAllocCursor (hDB, &cursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            /* setup the type of cursor navigation to be used */
            rc = rdm_dbGetRows (hDB, TABLE_CORE02_RECORD, &cursor);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            /* scan and read all created records */
            /* one additional record will be created each time you run this
             * example */
            for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
                 rc = rdm_cursorMoveToNext (cursor))
            {
                /* read the row at the current cursor position */
                rc = rdm_cursorReadRow (
                    cursor, &core02_output, sizeof (core02_output), NULL);
                print_error (rc);

                if (rc == sOKAY)
                {
                    printf ("%s\n", core02_output.message);
                }
            }

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
        if (cursor)
        {
            /* free the cursor resource when we're done */
            rdm_cursorFree (cursor);
        }
        rdm_dbEnd (hDB);
    }
    return rc;
}

/* \brief Main function for Core02 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the \c RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core02 (int32_t argc, const char *const *argv)
{
    RDM_TFS hTFS;
    RDM_DB hDB;
    RDM_RETCODE rc;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        rc = exampleOpenDatabase (&hTFS, &hDB, "core02", core02Example_cat);
        if (rc == sOKAY)
        {
            rc = insertOneRow (hDB);
            if (rc == sOKAY)
            {
                rc = readAllRows (hDB);
            }
            exampleCleanup (hTFS, hDB);
        }
    }

    return (int) rc;
}

RDM_STARTUP_EXAMPLE (core02)
