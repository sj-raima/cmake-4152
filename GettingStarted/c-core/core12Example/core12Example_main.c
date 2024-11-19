#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core12_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core12_cat.h"

RDM_CMDLINE cmd;
const char *const description = "Demonstrates transaction commit/rollback";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core12 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (all existing contents removed)
 * \li A transaction is started
 * \li Several NA offices are added to the database
 * \li All offices in the database are displayed (including uncommitted)
 * \li The transaction is committed
 * \li All offices in the database are displayed (all now committed)
 * \li Another transaction is started
 * \li Several EMEA offices are added to the database
 * \li All offices in the database are displayed (including uncommitted)
 * \li The transaction is rolled back
 * \li All offices in the database are displayed (uncommitted EMEA offices
 *      are no longer in the database)
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
 * in this example is located in the file \c core12.sdl.
 *
 * \include core12.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s option
 * to generate C-structures for interfacing with the database.
 *
 * \code rdm-compile -s core12.sdl \endcode
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

/* \brief Initialize the RDM runtime library for use in the core12 example
 *
 * This function initializes the RDM Transactional File Server (TFS) to use
 * the EMBEDED TFS implementation.  It also allocates a database handle
 * and opens the "core12" database in exclusive mode.
 * Exclusve mode does not require database locks or transactions.
 *
 * @return Returns RDM_RETCODE status code (sOKAY if successful)
 */

RDM_RETCODE openEmptyDatabase (
    RDM_TFS *pTFS, /*< [out] Pointer to the RDM TFS handle */
    RDM_DB *pDB)   /*< [out] Pointer to the RDM database handle */
{
    RDM_RETCODE rc;

    rc = rdm_rdmAllocTFS (pTFS);
    print_error (rc);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (*pTFS);
        print_error (rc);
        if (rc == sOKAY)
        {
            rc = rdm_tfsAllocDatabase (*pTFS, pDB);
            print_error (rc);

            if (rc == sOKAY)
            {
                /*
                 *  Associate the compiled catalog with the DB handle.
                 *  If the database does not exist on open, it will be created
                 *  in the default DOCROOT directory.
                 */
                rc = rdm_dbSetCatalog (*pDB, core12_cat);
                print_error (rc);

                if (rc == sOKAY)
                {
                    rc = rdm_dbOpen (*pDB, "core12", RDM_OPEN_EXCLUSIVE);
                    print_error (rc);
                }

                if (rc == sOKAY)
                {
                    rc = rdm_dbStartUpdate (
                        *pDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
                    print_error (rc);
                }
                if (rc == sOKAY)
                {
                    rc = rdm_dbDeleteAllRowsFromDatabase (*pDB);
                    print_error (rc);
                    rdm_dbEnd (*pDB);
                }

                /* If any of the above DB operations failed, free the DB handle
                 */
                if (rc != sOKAY)
                {
                    rdm_dbFree (*pDB);
                }
            }
        }

        /* If any of the above DB operations failed, free the TFS handle */
        if (rc != sOKAY)
        {
            rdm_tfsFree (*pTFS);
        }
    }

    return rc;
}

/* \brief Cleanup the RDM runtime library
 *
 * This functions closes all open databases and cleans up the RDM TFS and
 * database handles used in the core20 example.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
void cleanup (
    RDM_TFS hTFS, /*< [out] Pointer to the TFS handle to be terminated */
    RDM_DB hDB)   /*< [out] Pointer to the DB handle to be terminated */
{
    /* close the database */
    rdm_dbClose (hDB);

    /* free the database handle */
    rdm_dbFree (hDB);

    /* free the TFS handle */
    rdm_tfsFree (hTFS);
}

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
        rc = rdm_cursorReadRow (cursor, &office_rec, sizeof (office_rec), NULL);
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
    return rc;
}

RDM_RETCODE insertOffices (RDM_DB hDB, const char **officeList, size_t listSize)
{
    RDM_RETCODE rc = sOKAY;
    OFFICE office_rec;
    uint32_t ii;

    for (ii = 0U; ii < listSize; ii++)
    {
        strncpy (office_rec.name, officeList[ii], sizeof (office_rec.name));
        rc = rdm_dbInsertRow (
            hDB, TABLE_OFFICE, &office_rec, sizeof (office_rec), NULL);
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

/* \brief Main function for core12 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core12 (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
    RDM_TFS hTFS;
    RDM_DB hDB;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        rc = openEmptyDatabase (&hTFS, &hDB);
        if (rc == sOKAY)
        {
            rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
            print_error (rc);

            if (rc == sOKAY)
            {
                insertOffices (hDB, na_office_names, RLEN (na_office_names));
                printf ("\nAll Offices before transaction commit.\n");
                display_offices (hDB);

                rdm_dbEnd (hDB);
                print_error (rc);

                if (rc == sOKAY)
                {
                    /* Display all offices */
                    printf ("\nAll Offices after transaction commit.\n");
                    display_offices (hDB);
                }
            }
            if (rc == sOKAY)
            {
                rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
                print_error (rc);

                if (rc == sOKAY)
                {
                    insertOffices (
                        hDB, emea_office_names, RLEN (emea_office_names));

                    printf ("\nAll Offices before transaction commit.\n");
                    display_offices (hDB);

                    rc = rdm_dbEndRollback (hDB);
                    print_error (rc);

                    /* Display all offices */
                    printf ("\nAll Offices after transaction rollback.\n");
                    display_offices (hDB);
                }
            }
        }
        cleanup (hTFS, hDB);
    }
    return (int) rc;
}

RDM_STARTUP_EXAMPLE (core12)
