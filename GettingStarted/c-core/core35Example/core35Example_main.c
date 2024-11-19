#include <stdio.h>
#include <stdarg.h>

#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core35Example_structs.h"
#include "core35Example_cat.h"
#include "rdmstartupapi.h"

/*  \brief Simple r-tree example

    This example will show you how to manage 2 dimensional data, using an
    r-tree index.

    The example will insert a couple of rows containing rectangle data which
    will be put into an r-tree.

    The example will then perform a series of queries to retrieve the data
    that meet specific criteria.
*/

/* \brief Print message to stdout
 */
PSP_PRIVATE void PrintMessage (const char *format, ...)
{
    va_list args;

    va_start (args, format);
    vfprintf (stdout, format, args);
    va_end (args);
}

/* \brief Print message to stdout and then flush
 */
PSP_PRIVATE void PrintMessageNow (const char *format, ...)
{
    va_list args;

    va_start (args, format);
    vfprintf (stdout, format, args);
    va_end (args);

    fflush (stdout);
}

/* \brief Print message to stderr
 */
PSP_PRIVATE void PrintError (const char *format, ...)
{
    va_list args;

    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
}

/* \brief Print database error message to stderr
 */
PSP_PRIVATE void PrintDbError (RDM_RETCODE rc, const char *msg)
{
    PrintError (
        "There was an error (%s - %s) %s\n", rdm_retcodeGetName (rc),
        rdm_retcodeGetDescription (rc), msg);
}

/* \brief Delete all data from the database

     This function will remove all rows from all tables in the database.

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE delete_data (RDM_DB db)
{
    RDM_RETCODE rc;

    PrintMessageNow ("Removing all data from the database\n");

    /* Start an update transaction */
    rc = rdm_dbStartUpdate (db, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    if (rc == sOKAY)
    {
        /* Remove all rows from all tables */
        rc = rdm_dbDeleteAllRowsFromDatabase (db);
        if (rc == sOKAY)
        {
            /* commit the transaction */
            rc = rdm_dbEnd (db);
        }
        else
        {
            /* Rollback on error */
            (void) rdm_dbEndRollback (db);
        }
    }

    if (rc != sOKAY)
    {
        PrintDbError (rc, "Error in delete_data");
    }

    return rc;
}

/* \brief Print the contents of a rectangle
 */
static void printRectangle (int count, double rect[12])
{
    int ii;
    PrintMessage ("{ ");
    for (ii = 0; ii < count; ii += 2)
    {
        if (ii)
            PrintMessage (", ");

        PrintMessage ("(%3.2lf, %3.2lf)", rect[ii], rect[ii + 1]);
    }
    PrintMessage (" }");
}

/* \brief Insert rows into the RTREE_TABLE table

     This function will insert two rows into the RTREE_TABLE table.

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE insert_data (RDM_DB db)
{
    RDM_RETCODE rc;
    RTREE_TABLE row_data;

    /* First we will remove all existing data from the database */
    rc = delete_data (db);

    if (rc == sOKAY)
    {
        PrintMessageNow ("Inserting data...\n");

        /* Start a transaction */
        rc = rdm_dbStartUpdate (db, RDM_LOCK_ALL, 0, NULL, 0, NULL);
        if (rc == sOKAY)
        {
            /* Insert the first row */
            row_data.id = 1;
            row_data.rect[0] = 5;  /* Min X value */
            row_data.rect[1] = 5;  /* Min Y value */
            row_data.rect[2] = 10; /* Max X value */
            row_data.rect[3] = 15; /* Max Y value */

            rc = rdm_dbInsertRow (
                db, TABLE_RTREE_TABLE, &row_data, sizeof (row_data), NULL);

            if (rc == sOKAY)
            {
                PrintMessage (
                    "    ** Inserted Row id: %d, rect = ", row_data.id);
                printRectangle (4, row_data.rect);
                PrintMessageNow ("\n");

                /* Insert the second row */
                row_data.id = 2;
                row_data.rect[0] = 12; /* Min X value */
                row_data.rect[1] = 7;  /* Min Y value */
                row_data.rect[2] = 18; /* Max X value */
                row_data.rect[3] = 20; /* Max Y value */

                rc = rdm_dbInsertRow (
                    db, TABLE_RTREE_TABLE, &row_data, sizeof (row_data), NULL);
            }
            if (rc == sOKAY)
            {
                PrintMessage (
                    "    ** Inserted Row id: %d, rect = ", row_data.id);
                printRectangle (4, row_data.rect);
                PrintMessageNow ("\n");

                /* Insert the third row */
                row_data.id = 3;
                row_data.rect[0] = 14; /* Min X value */
                row_data.rect[1] = 11; /* Min Y value */
                row_data.rect[2] = 17; /* Max X value */
                row_data.rect[3] = 14; /* Max Y value */

                rc = rdm_dbInsertRow (
                    db, TABLE_RTREE_TABLE, &row_data, sizeof (row_data), NULL);
                if (rc == sOKAY)
                {
                    PrintMessage (
                        "    ** Inserted Row id: %d, rect = ", row_data.id);
                    printRectangle (4, row_data.rect);
                    PrintMessageNow ("\n");
                }
            }

            if (rc == sOKAY)
            {
                /* commit the transaction */
                rc = rdm_dbEnd (db);
            }
            else
            {
                /* On error rollback the transaction */
                (void) rdm_dbEndRollback (db);
            }
        }
    }

    if (rc != sOKAY)
    {
        PrintDbError (rc, "Error in insert_data");
    }

    return rc;
}

/* \brief Print the current for of a cursor based on the RTREE_TABLE

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE printCurrentRtreeTableRow (RDM_CURSOR cursor)
{
    RDM_RETCODE rc;
    RTREE_TABLE row;

    /* Read the value of the current row into a structure */
    rc = rdm_cursorReadRow (cursor, &row, sizeof (row), NULL);
    if (rc == sOKAY)
    {
        /* Print the row that was read */
        PrintMessage ("    ** Found Row id: %d, rect = ", row.id);
        printRectangle (4, row.rect);
        PrintMessageNow ("\n");
    }

    if (rc != sOKAY)
    {
        PrintDbError (rc, "Error in printCurrentRtreeTableRow");
    }

    return rc;
}

/* \brief Look up rectangles that meet the specified radius criteria

     This function will read create a cursor containing all rectangles in the
     r-tree that meet the radius bounding box criteria.  It will then iterate
   through the cursor and display the rows.

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayRectanglesRadius (
    RDM_DB db,             /*< [in] database handle */
    RDM_RTREE_TYPE type,   /*< [in] The query type */
    double boundingBox[4], /*< [in] The bounding box */
    double radius          /*< [in] Radius */
)
{
    RDM_RETCODE rc;
    RDM_RTREE_KEY key = {0};
    RDM_CURSOR cursor = NULL;
    RDM_TABLE_ID tablesToLock[] = {TABLE_RTREE_TABLE};
    bool found = false;

    key.type = type;
    key.value = boundingBox;
    key.bytesIn = sizeof (double[2]);
    key.filter.radius = radius;

    /* Get a read lock on the RTREE_TABLE table */
    rc = rdm_dbStartRead (db, tablesToLock, RDM_LEN (tablesToLock), NULL);
    if (rc == sOKAY)
    {
        /* Create the cursor using the r-tree */
        rc = rdm_dbGetRowsByKeyInRtreeKeyRange (
            db, KEY_RTREE_TABLE_RECT, &key, &cursor);
        if (rc == sOKAY)
        {
            /* Move to the first resulting row */
            rc = rdm_cursorMoveToFirst (cursor);
            while (rc == sOKAY)
            {
                found = true;

                /* Print the current cursor row */
                rc = printCurrentRtreeTableRow (cursor);
                if (rc == sOKAY)
                {
                    /* Move to the next row */
                    rc = rdm_cursorMoveToNext (cursor);
                }
            }

            /* We expect to break out of the loop with sENDOFCURSOR*/
            if (rc == sENDOFCURSOR)
            {
                rc = sOKAY;
            }
        }

        /* Free the locks */
        (void) rdm_dbEnd (db);
    }

    /* Free the cursor */
    if (cursor)
    {
        (void) rdm_cursorFree (cursor);
    }

    /* Display a message if we didn't retrieve any rows */
    if (rc == sOKAY && found == false)
    {
        PrintMessage ("    ** No Rows Found\n");
    }

    /* Display a message if there was an error */
    if (rc != sOKAY)
    {
        PrintDbError (rc, "Error in displayRectangles");
    }

    return rc;
}

/* \brief Look up rectangles that meet the specified radius criteria

     This function will read create a cursor containing all rectangles in the
     r-tree that meet the radius bounding box criteria.  It will then iterate
   through the cursor and display the rows.

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayRectanglesPolygon (
    RDM_DB db,              /*< [in] database handle */
    RDM_RTREE_TYPE type,    /*< [in] The query type */
    double boundingBox[12], /*< [in] The bounding box */
    int8_t numVertices      /*< [in] numVertices */
)
{
    RDM_RETCODE rc;
    RDM_RTREE_KEY key = {0};
    RDM_CURSOR cursor = NULL;
    RDM_TABLE_ID tablesToLock[] = {TABLE_RTREE_TABLE};
    bool found = false;

    key.type = type;
    key.value = boundingBox;
    key.bytesIn = (size_t) (sizeof (double) * numVertices * 2);
    key.filter.numVertices = numVertices;

    /* Get a read lock on the RTREE_TABLE table */
    rc = rdm_dbStartRead (db, tablesToLock, RDM_LEN (tablesToLock), NULL);
    if (rc == sOKAY)
    {
        /* Create the cursor using the r-tree */
        rc = rdm_dbGetRowsByKeyInRtreeKeyRange (
            db, KEY_RTREE_TABLE_RECT, &key, &cursor);
        if (rc == sOKAY)
        {
            /* Move to the first resulting row */
            rc = rdm_cursorMoveToFirst (cursor);
            while (rc == sOKAY)
            {
                found = true;

                /* Print the current cursor row */
                rc = printCurrentRtreeTableRow (cursor);
                if (rc == sOKAY)
                {
                    /* Move to the next row */
                    rc = rdm_cursorMoveToNext (cursor);
                }
            }

            /* We expect to break out of the loop with sENDOFCURSOR*/
            if (rc == sENDOFCURSOR)
            {
                rc = sOKAY;
            }
        }

        /* Free the locks */
        (void) rdm_dbEnd (db);
    }

    /* Free the cursor */
    if (cursor)
    {
        (void) rdm_cursorFree (cursor);
    }

    /* Display a message if we didn't retrieve any rows */
    if (rc == sOKAY && found == false)
    {
        PrintMessage ("    ** No Rows Found\n");
    }

    /* Display a message if there was an error */
    if (rc != sOKAY)
    {
        PrintDbError (rc, "Error in displayRectangles");
    }

    return rc;
}

/* \brief Look up rectangles that meet the specified criteria

     This function will read create a cursor containing all rectangles in the
     r-tree that meet the bounding box criteria.  It will then iterate through
     the cursor and display the rows.

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayRectangles (
    RDM_DB db,            /*< [in] database handle */
    RDM_RTREE_TYPE type,  /*< [in] The query type */
    double boundingBox[4] /*< [in] The bounding box */
)
{
    RDM_RETCODE rc;
    RDM_RTREE_KEY key = {0};
    RDM_CURSOR cursor = NULL;
    RDM_TABLE_ID tablesToLock[] = {TABLE_RTREE_TABLE};
    bool found = false;

    key.type = type;
    key.value = boundingBox;
    key.bytesIn = sizeof (double[4]);

    /* Get a read lock on the RTREE_TABLE table */
    rc = rdm_dbStartRead (db, tablesToLock, RDM_LEN (tablesToLock), NULL);
    if (rc == sOKAY)
    {
        /* Create the cursor using the r-tree */
        rc = rdm_dbGetRowsByKeyInRtreeKeyRange (
            db, KEY_RTREE_TABLE_RECT, &key, &cursor);
        if (rc == sOKAY)
        {
            /* Move to the first resulting row */
            rc = rdm_cursorMoveToFirst (cursor);
            while (rc == sOKAY)
            {
                found = true;

                /* Print the current cursor row */
                rc = printCurrentRtreeTableRow (cursor);
                if (rc == sOKAY)
                {
                    /* Move to the next row */
                    rc = rdm_cursorMoveToNext (cursor);
                }
            }

            /* We expect to break out of the loop with sENDOFCURSOR*/
            if (rc == sENDOFCURSOR)
            {
                rc = sOKAY;
            }
        }

        /* Free the locks */
        (void) rdm_dbEnd (db);
    }

    /* Free the cursor */
    if (cursor)
    {
        (void) rdm_cursorFree (cursor);
    }

    /* Display a message if we didn't retrieve any rows */
    if (rc == sOKAY && found == false)
    {
        PrintMessage ("    ** No Rows Found\n");
    }

    /* Display a message if there was an error */
    if (rc != sOKAY)
    {
        PrintDbError (rc, "Error in displayRectangles");
    }

    return rc;
}
/* \brief Look up rectangles contained by the specified polygon bounding box

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayPolygonContainedRectangles (
    RDM_DB db,             /*< [IN} The database handle */
    double boundingBox[4], /*< The bounding box value */
    int8_t numVertices     /*< [in] number of vertices */
)
{
    return displayRectanglesPolygon (
        db, RDM_RTREE_POLYGON_CONTAINS, boundingBox, numVertices);
}

/* \brief Look up rectangles overlapping the specified radius bounding box

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayPolygonOverlappingRectangles (
    RDM_DB db,             /*< [IN} The database handle */
    double boundingBox[4], /*< The bounding box value */
    int8_t numVertices     /*< [in] number of vertices */
)
{
    return displayRectanglesPolygon (
        db, RDM_RTREE_POLYGON_OVERLAP, boundingBox, numVertices);
}

/* \brief Look up rectangles contained by the specified radius bounding box

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayRadiusContainedRectangles (
    RDM_DB db,             /*< [IN} The database handle */
    double boundingBox[4], /*< The bounding box value */
    double radius          /*< [in] Radius */
)
{
    return displayRectanglesRadius (
        db, RDM_RTREE_RADIUS_CONTAINS, boundingBox, radius);
}

/* \brief Look up rectangles overlapping the specified radius bounding box

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayRadiusOverlappingRectangles (
    RDM_DB db,             /*< [IN} The database handle */
    double boundingBox[4], /*< The bounding box value */
    double radius          /*< [in] Radius */
)
{
    return displayRectanglesRadius (
        db, RDM_RTREE_RADIUS_OVERLAP, boundingBox, radius);
}

/* \brief Look up rectangles contained by the specified bounding box

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayContainedRectangles (
    RDM_DB db,            /*< [IN} The database handle */
    double boundingBox[4] /*< The bounding box value */
)
{
    return displayRectangles (db, RDM_RTREE_CONTAINS, boundingBox);
}

/* \brief Look up rectangles overlapping the specified bounding box

     \returns A RDM_RETCODE value
     \retval sOKAY Success
*/
static RDM_RETCODE displayOverlappingRectangles (
    RDM_DB db,            /*< [IN] The database handle */
    double boundingBox[4] /*< [IN] The bounding box value */
)
{
    return displayRectangles (db, RDM_RTREE_OVERLAP, boundingBox);
}

/* \brief The main function

     This function will do initialization and cleanup as well as call
     the functions that insert/retrieve data
*/
RDM_EXPORT int32_t main_rtree (int32_t argc, const char *const *argv)
{
    RDM_RETCODE rc;
    RDM_TFS tfs = NULL;
    RDM_DB db = NULL;
    double boundingBox[12];

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS (&tfs);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (tfs);
    }
    if (rc == sOKAY)
    {
        /* Allocate a database handle from out TFS */
        rc = rdm_tfsAllocDatabase (tfs, &db);
        if (rc == sOKAY)
        {
            /* Assign a catalog to the database handle */
            rc = rdm_dbSetCatalog (db, core35Example_cat);
        }

        if (rc == sOKAY)
        {
            /* Open the database */
            rc = rdm_dbOpen (db, "core35", RDM_OPEN_SHARED);
        }
    }

    if (rc == sOKAY)
    {
        /* Insert data */
        rc = insert_data (db);

        if (rc == sOKAY)
        {
            /* Do lookups with a bounding box that contains both of the rows
               in the table */
            boundingBox[0] = 3;  /* Min X Value */
            boundingBox[1] = 3;  /* Min Y Value */
            boundingBox[2] = 25; /* Max X Value */
            boundingBox[3] = 25; /* Max Y Value */

            PrintMessage (
                "\n\nLooking for rows CONTAINED by the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 3 rows found\n");
            rc = displayContainedRectangles (db, boundingBox);

            PrintMessage ("\n\nLooking for rows OVERLAPPING the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 3 rows found\n");
            rc = displayOverlappingRectangles (db, boundingBox);
        }

        if (rc == sOKAY)
        {
            /* Do lookups with a bounding box that contains only one of the rows
               in the table, but overlaps them both */
            boundingBox[0] = 6;  /* Min X Value */
            boundingBox[1] = 6;  /* Min Y Value */
            boundingBox[2] = 25; /* Max X Value */
            boundingBox[3] = 25; /* Max Y Value */

            PrintMessage (
                "\n\nLooking for rows CONTAINED by the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 2 rows found\n");
            rc = displayContainedRectangles (db, boundingBox);

            PrintMessage ("\n\nLooking for rows OVERLAPPING the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 3 rows found\n");
            rc = displayOverlappingRectangles (db, boundingBox);
        }

        if (rc == sOKAY)
        {
            /* Do lookups with a bounding box that doesn't contain either of the
               rows in the table, but overlaps them both */
            boundingBox[0] = 6;  /* Min X Value */
            boundingBox[1] = 14; /* Min Y Value */
            boundingBox[2] = 19; /* Max X Value */
            boundingBox[3] = 25; /* Max Y Value */

            PrintMessage (
                "\n\nLooking for rows CONTAINED by the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 0 rows found\n");
            rc = displayContainedRectangles (db, boundingBox);

            PrintMessage ("\n\nLooking for rows OVERLAPPING the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 3 rows found\n");
            rc = displayOverlappingRectangles (db, boundingBox);
        }

        if (rc == sOKAY)
        {
            /* Do lookups with a bounding box that doesn't contain either of the
               rows in the table and doesn't overlap either of them */
            boundingBox[0] = 25; /* Min X Value */
            boundingBox[1] = 25; /* Min Y Value */
            boundingBox[2] = 35; /* Max X Value */
            boundingBox[3] = 35; /* Max Y Value */

            PrintMessage (
                "\n\nLooking for rows CONTAINED by the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 0 rows found\n");
            rc = displayContainedRectangles (db, boundingBox);

            PrintMessage ("\n\nLooking for rows OVERLAPPING the bounding box ");
            printRectangle (4, boundingBox);
            PrintMessageNow ("\nThere should be 0 rows found\n");
            rc = displayOverlappingRectangles (db, boundingBox);
        }

        if (rc == sOKAY)
        {
            /* Do lookups with a bounding box that doesn't contain either of the
               rows in the table and doesn't overlap either of them */
            boundingBox[0] = 14; /* Min X Value */
            boundingBox[1] = 11; /* Min Y Value */

            PrintMessage (
                "\n\nLooking for rows CONTAINED by the RADIUS bounding box ");
            printRectangle (2, boundingBox);
            PrintMessageNow ("\nThere should be 1 rows found\n");
            rc = displayRadiusContainedRectangles (db, boundingBox, 4.5);

            PrintMessage (
                "\n\nLooking for rows OVERLAPPING the RADIUS bounding box ");
            printRectangle (2, boundingBox);
            PrintMessageNow ("\nThere should be 3 rows found\n");
            rc = displayRadiusOverlappingRectangles (db, boundingBox, 4.0);
        }

        if (rc == sOKAY)
        {
            /* Do lookups with a bounding box that doesn't contain either of the
               rows in the table and doesn't overlap either of them */
            boundingBox[0] = 6;  /* Vertice 1 X Value */
            boundingBox[1] = 6;  /* Virtice 1 Y Value */
            boundingBox[2] = 15; /* Virtice 2 X Value */
            boundingBox[3] = 8;  /* Virtice 2 Y Value */
            boundingBox[4] = 20; /* Virtice 3 X Value */
            boundingBox[5] = 10; /* Virtice 3 Y Value */
            boundingBox[6] = 20; /* Virtice 4 X Value */
            boundingBox[7] = 20; /* Virtice 4 Y Value */
            boundingBox[8] = 10; /* Virtice 5 X Value */
            boundingBox[9] = 20; /* Virtice 5 Y Value */

            PrintMessage (
                "\n\nLooking for rows CONTAINED by the POLYGON bounding box ");
            printRectangle (10, boundingBox);
            PrintMessageNow ("\nThere should be 1 rows found\n");
            rc = displayPolygonContainedRectangles (db, boundingBox, 5);

            PrintMessage (
                "\n\nLooking for rows OVERLAPPING the POLYGON bounding box ");
            printRectangle (10, boundingBox);
            PrintMessageNow ("\nThere should be 3 rows found\n");
            rc = displayPolygonOverlappingRectangles (db, boundingBox, 5);
        }
    }
    else
    {
        PrintDbError (rc, "initialing the example");
    }

    if (db)
    {
        /* Close the database and free the db handle */
        (void) rdm_dbClose (db);
        (void) rdm_dbFree (db);
    }

    if (tfs)
    {
        /* Free the tfs handle */
        rdm_tfsFree (tfs);
    }

    return 0;
}

RDM_STARTUP_EXAMPLE (rtree)
