#include <stdio.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core24_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core24_cat.h"

RDM_CMDLINE cmd;
const char *const description =
    "Performance Test (inmemory): comparing transaction sizes";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/* Number of records to insert */
#define NUM_RECORDS 50000

/*
 * \mainpage core24 Popcorn Example
 *
 * The core24 example performs a number of atomic database operations
 * on a very simple database schema using the RDM Core C APIs.
 *
 * Each time you run this example
 *   * The database is initialized (removing all existing data)
 *   * 50,000 new records are created using different transaction block sizes
 *       * 50,000
 * 10,000
 *  5,000
 *  1,000
 *    500
 *    100
 *   * Database is cleaned up
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
 * in this example is located in the file \c core24.sdl. The example utilizes
 * the SQL DDL syntax and specifies database schema with a single table
 * (SIMPLE). The table has a single column (INT_COL). There are no index
 * definitions for this database.
 *
 * \include core24.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s and -a
 * options to generate an embedded database dictionary.
 *
 * \code rdm-compile -s -a core24.sdl \endcode
 *
 * The schema catalog information (\c core24_cat.c) is embedded inside the
 * application.
 *
 * \page hPGMfunc Program Functions
 *
 * For simplicity, this example does not check all return codes, but good
 * programming practices would dictate that they are checked after each
 * RDM call.
 *
 * \li add_records() - \copybrief add_records
 * \li delete_records() - \copybrief delete_records
 * \li main() - \copybrief main
 */

/* \brief Remove simple records into the core24 database
 *
 * This function empties the simple table database.  The
 *
 * @return Returns an RDM_RETCODE code (sOKAY if successful)
 */
RDM_RETCODE delete_records (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        /* empty the table */
        rc = rdm_dbDeleteAllRowsFromTable (hDB, TABLE_SIMPLE);
        print_error (rc);
        rdm_dbEnd (hDB);
    }
    return rc;
}

/* \brief Insert 50,000 simple records into the core24 database
 *
 * This function adds 50,000 simple records into the inmemory perf database. The
 * simple record contains only a single integer field with no index.  The
 * function will calculate and display the total time elapsed while inserting
 * the records
 *
 * @return Returns an RDM_RETCODE code (sOKAY if successful)
 */
RDM_RETCODE add_records (
    unsigned int trans_size, /*< [in] Transaction block size */
    RDM_DB hDB)              /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc = sOKAY;
    SIMPLE simple_rec = {0};
    perfTimer_t timer;
    unsigned int count = 0;
    unsigned int ii;

    printf (
        "Add %d records with %-5d per transaction:\t", NUM_RECORDS, trans_size);

    timeMeasureBegin (&timer);

    /* Loop to add 50,000 new simple record */
    for (ii = 0, count = 0; ii < NUM_RECORDS && rc == sOKAY; ii++)
    {
        if (count == 0)
        {
            /* Start an update transaction and lock the table */
            rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            /* Set values for the new record */
            simple_rec.int_col = 1;

            /* Add the record to the database */
            rc = rdm_dbInsertRow (
                hDB, TABLE_SIMPLE, &simple_rec, sizeof (simple_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            /* Is it time to commit the transaction? */
            if (++count == trans_size)
            {
                rc = rdm_dbEnd (hDB);
                print_error (rc);
                count = 0;
            }
        }
    }
    timeMeasureEnd (&timer);

    printf ("%u milliseconds\n", timeMeasureDiff (&timer));

    return rc;
}

/* \brief Main function for core24 example
 *
 * The function initializes the RDM environment and runs the insert operations.
 * over a number of transaction block sizes
 *
 * @return Returns the RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core24 (int32_t argc, const char *const *argv)
{
    RDM_DB hDB;
    RDM_TFS hTFS;
    RDM_RETCODE rc;
    unsigned int trans_size[] = {50000, 10000, 5000, 1000, 500, 100};

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        /* Initialize the TFS, task, and open/initialize the database */
        rc = exampleOpenEmptyDatabaseInMemory (
            &hTFS, &hDB, "core24", core24_cat);
        if (rc == sOKAY)
        {
            int ii;
            for (ii = 0; rc == sOKAY && ii < RLEN (trans_size); ii++)
            {
                /* delete all records before testing insert */
                rc = delete_records (hDB);

                if (rc == sOKAY)
                {
                    /* Add records to the database */
                    rc = add_records (trans_size[ii], hDB);
                }
            }
            exampleCleanup (hTFS, hDB);
        }
    }
    return (int) rc;
}
RDM_STARTUP_EXAMPLE (core24)
