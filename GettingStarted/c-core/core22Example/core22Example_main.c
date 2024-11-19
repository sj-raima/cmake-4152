#include <stdio.h>
#include "rdm.h"
#include "example_fcns.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core22_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core22_cat.h"

RDM_CMDLINE cmd;
const char *const description =
    "Performance Test (inmemory): reading/deleting in key order";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage core22 Popcorn Example
 *
 * The core22 example performs a number of atomic database operations
 * on a very simple database schema using the RDM Core C APIs.
 *
 * Each time you run this example
 *   \li The database is initialized
 *   \li 50,000 new records are created
 *   \li Each record is read
 *   \li Each record is updated
 *   \li Each record is read again
 *   \li Each record is deleted
 *   \li Database is cleaned up
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
 * in this example is located in the file \c core22.sdl. The example utilizes
 * the SQL DDL syntax and specifies database schema with a single table
 * (SIMPLE). The table has a single column (INT_COL). There are no index
 * definitions for this database.
 *
 * \include core22.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s and -a
 * options to generate an embedded database dictionary.
 *
 * \code rdm-compile -s -a core22.sdl \endcode
 *
 * The schema catalog information (\c core22_cat.c) is embedded inside the
 * application.
 *
 * \page hPGMfunc Program Functions
 * \li add_records() - \copybrief add_records
 * \li read_records() - \copybrief read_records
 * \li update_records() - \copybrief update_records
 * \li delete_records() - \copybrief delete_records
 * \li main() - \copybrief main
 */

/* \brief Insert 50,000 simple records into the core22 database
 *
 * This function adds 50,000 simple records into the inmemory perf database. The
 * simple record contains only a single integer field with no index.  The
 * function will calculate and display the total time elapsed while inserting
 * the records
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE add_records (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc;
    SIMPLE simple_rec = {0};
    perfTimer_t timer;

    timeMeasureBegin (&timer);

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);

    if (rc == sOKAY)
    {
        /* Loop to add 50,000 new simple record */
        int ii;
        for (ii = 0; ii < 50000; ii++)
        {
            /* Set values for the new record */
            simple_rec.int_col = 1;

            /* Add the record to the database */
            rc = rdm_dbInsertRow (
                hDB, TABLE_SIMPLE, &simple_rec, sizeof (simple_rec), NULL);
            print_error (rc);
        }
        rdm_dbEnd (hDB);
    }

    timeMeasureEnd (&timer);
    printf (
        "Add 50,000 records:\t\t\t\t%u milliseconds\n",
        timeMeasureDiff (&timer));
    return rc;
}

/* \brief Read all simple records and calculate the sum of the int_col column
 *
 * This function iterates through and reads all simple records stored in the
 * core22 database.  The iteration is done in key order.  The function will
 * calculate the sum of the INT_COL field from all simple records in the
 * database.  The function will also calculate and display the total time
 * elapsed while reading the records.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE read_records (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc = sOKAY;
    SIMPLE simple_rec;
    RDM_CURSOR cursor;
    perfTimer_t timer;
    unsigned int total = 0;

    timeMeasureBegin (&timer);

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);

    if (rc == sOKAY)
    {
        /* allocate a cursor resource */
        rc = rdm_dbAllocCursor (hDB, &cursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            /* Fill a cursor with all the rows in a table */
            rc = rdm_dbGetRowsByKey (hDB, KEY_SIMPLE_INT_COL, &cursor);
            print_error (rc);

            if (rc == sOKAY)
            {
                /* Loop through each simple record in table order */
                for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
                     rc = rdm_cursorMoveToNext (cursor))
                {
                    /* Read the record */
                    rc = rdm_cursorReadRow (
                        cursor, &simple_rec, sizeof (simple_rec), NULL);
                    print_error (rc);

                    if (rc == sOKAY)
                    {
                        /* Add the value read to the running sum */
                        total += simple_rec.int_col;
                    }
                }
            }
            /* Free the cursor */
            rdm_cursorFree (cursor);
        }
        rdm_dbEnd (hDB);
    }

    timeMeasureEnd (&timer);

    /* We expect rc to be sENDOFCURSOR when we break out of the loop */
    if (rc == sENDOFCURSOR)
    {
        printf (
            "Scanning, reading, summing 50,000 records:\t%u milliseconds\n",
            timeMeasureDiff (&timer));
        printf ("\tSum = %u\n", total);
        rc = sOKAY;
    }
    return rc;
}

/* \brief Update all simple records in the database
 *
 * This function iterates through and updates all simple records stored in the
 * core22 database.  The iteration is done in table order.  The function will
 * read, increment, and then update the value stored in the INT_COL field.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE update_records (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc;
    SIMPLE simple_rec;
    RDM_CURSOR cursor;
    perfTimer_t timer;

    timeMeasureBegin (&timer);

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);

    if (rc == sOKAY)
    {
        /* allocate a cursor resource */
        rc = rdm_dbAllocCursor (hDB, &cursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            /* Fill a cursor with all the rows in a table */
            rc = rdm_dbGetRows (hDB, TABLE_SIMPLE, &cursor);
            print_error (rc);

            if (rc == sOKAY)
            {
                /* Loop through each simple record in table order */
                for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
                     rc = rdm_cursorMoveToNext (cursor))
                {
                    /* Read the record */
                    rc = rdm_cursorReadRow (
                        cursor, &simple_rec, sizeof (simple_rec), NULL);
                    print_error (rc);

                    if (rc == sOKAY)
                    {
                        /* increment the inc_col field */
                        simple_rec.int_col++;

                        /* Update the simple record with the new int_col value
                         */
                        rc = rdm_cursorUpdateRow (
                            cursor, &simple_rec, sizeof (simple_rec));
                        print_error (rc);
                    }
                }
            }
            /* Free the cursor */
            rdm_cursorFree (cursor);
        }
        rdm_dbEnd (hDB);
    }

    timeMeasureEnd (&timer);

    /* We expect rc to be sENDOFCURSOR when we break out of the loop */
    if (rc == sENDOFCURSOR)
    {
        printf (
            "Updating 50,000 records:\t\t\t%u milliseconds\n",
            timeMeasureDiff (&timer));
        rc = sOKAY;
    }
    return rc;
}

/* \brief Delete all simple records from the database
 *
 * This function will iterate and delete all simple records stored in the core22
 * database.  The iteration is done in reverse key order.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE delete_records (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc;
    RDM_CURSOR cursor = NULL;
    perfTimer_t timer;

    timeMeasureBegin (&timer);

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);

    if (rc == sOKAY)
    {
        /* Fill a cursor with all the rows in a table */
        rc = rdm_dbGetRowsByKey (hDB, KEY_SIMPLE_INT_COL, &cursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            /* Loop through each simple record in table order */
            for (rc = rdm_cursorMoveToLast (cursor); rc == sOKAY;
                 rc = rdm_cursorMoveToPrevious (cursor))
            {
                /* Read the record */
                rc = rdm_cursorDeleteRow (cursor);
                print_error (rc);
            }
            /* Free the cursor */
            rdm_cursorFree (cursor);
        }
        rdm_dbEnd (hDB);
    }

    timeMeasureEnd (&timer);

    /* We expect rc to be sENDOFCURSOR when we break out of the loop */
    if (rc == sENDOFCURSOR)
    {
        printf (
            "Deleting 50,000 records:\t\t\t%u milliseconds\n",
            timeMeasureDiff (&timer));
        rc = sOKAY;
    }
    return rc;
}

/* \brief Main function for core22 example
 *
 * The function initializes the RDM environment and runs the create, read,
 * update, and delete operations.
 *
 * @return Returns the \c RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core22 (int32_t argc, const char *const *argv)
{
    RDM_TFS hTFS;
    RDM_DB hDB;
    RDM_RETCODE rc;
    perfTimer_t timer;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        /* Initialize the handles and open the database */
        timeMeasureBegin (&timer);
        rc = exampleOpenEmptyDatabaseInMemory (
            &hTFS, &hDB, "core22", core22_cat);
        timeMeasureEnd (&timer);
        if (rc == sOKAY)
        {
            /* Display information */
            printf (
                "Preparing a new database:\t\t\t%u milliseconds\n",
                timeMeasureDiff (&timer));
            printf ("\tOne, simple record type.\n");
            printf ("\tIn-memory\n");
        }
        if (rc == sOKAY)
        {
            /* Add 50,000 records to the database */
            rc = add_records (hDB);
            if (rc == sOKAY)
            {
                /* Read all of the records in the database */
                rc = read_records (hDB);
                if (rc == sOKAY)
                {
                    /* Update all of the records in the database */
                    rc = update_records (hDB);
                    if (rc == sOKAY)
                    {
                        /* Read all of the records in the database */
                        rc = read_records (hDB);
                        if (rc == sOKAY)
                        {
                            /* Remove all of the records in the database */
                            rc = delete_records (hDB);
                        }
                    }
                }
            }
            timeMeasureBegin (&timer);
            exampleCleanup (hTFS, hDB);
            timeMeasureEnd (&timer);
            printf (
                "Cleanup:\t\t\t\t\t%u milliseconds\n",
                timeMeasureDiff (&timer));
        }
    }
    return (int) rc;
}
RDM_STARTUP_EXAMPLE (core22)
