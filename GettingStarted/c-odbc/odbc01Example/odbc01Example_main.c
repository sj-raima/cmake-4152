/*
**    HELLO WORLD SQL
**    ---------------
**    This document describes the process to create a simple database, insert
**    a record containing a text field, read the text field from database and
**    print it out.
*/

#include "sqlrext.h" /* The RDM SQL API. */
/*lint -e838 */

#include "hello_worldODBC_cat.h" /* Contains the database definition */
#include "rdmrdmapi.h"
#include "rdmtfsapi.h"
#include "rdmstartupapi.h"

#include <stdio.h>

#define SQL_EMPSTR ((SQLCHAR *) "") /* any string */

RDM_EXPORT int32_t main_hello_world_ODBCTutorial (int32_t argc, const char *const *argv)
{
    SQLRETURN rc; /* holds return value, 0 is success */
    SQLCHAR sz[32];
    SQLHDBC hCon;   /* connection handle  */
    SQLHSTMT hStmt; /* statement handle   */
    SQLHENV hEnv;   /* environment handle */
    SQLLEN iLen = 0;

    RDM_UNREF (argc);
    RDM_UNREF (argv);

    /* Allocate an environment handle */
    rc = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (SQL_SUCCEEDED (rc))
    {
        /* Set the version of ODBC - RDM ODBC currently only supports
           ODBC version 3 */
        (void) SQLSetEnvAttr (
            hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0);

        /* Allocate a connection handle */
        rc = SQLAllocHandle (SQL_HANDLE_DBC, hEnv, &hCon);
        if (SQL_SUCCEEDED (rc))
        {
            /* RDM-specific ODBC extension to specify the catalog to be used
               when opening the database */
            RDM_TFS hTFS;

            /* Allocate a TFS Handle */
            rc = rdm_rdmAllocTFS (&hTFS);
            if (rc == sOKAY)
            {
                rc = rdm_tfsInitialize (hTFS);
                {
                    SQLSetConnectAttr (
                        hCon, SQL_ATTR_RDM_TFS_HANDLE, (SQLPOINTER) hTFS,
                        SQL_IS_POINTER);

                    (void) SQLSetConnectAttr (
                        hCon, SQL_ATTR_RDM_CAT_BUFFER,
                        (SQLPOINTER) hello_worldODBC_cat, SQL_NTS);

                    /* Connect to server */
                    rc = SQLConnect (
                        hCon, SQL_EMPSTR, SQL_NTS, SQL_EMPSTR, SQL_NTS,
                        SQL_EMPSTR, SQL_NTS);
                    if (SQL_SUCCEEDED (rc))
                    {
                        /* Allocate a statement handle */
                        rc = SQLAllocHandle (SQL_HANDLE_STMT, hCon, &hStmt);
                        if (SQL_SUCCEEDED (rc))
                        {
                            /* Open the database */
                            rc = SQLExecDirect (
                                hStmt, (SQLCHAR *) "USE \"hello_worldODBC\"",
                                SQL_NTS);
                            if (SQL_SUCCEEDED (rc))
                            {
                                /* Create a Hello World record */
                                rc = SQLExecDirect (
                                    hStmt,
                                    (SQLCHAR *) "INSERT INTO info(myChar) "
                                                "VALUES('Hello World!')",
                                    SQL_NTS);
                            }

                            if (SQL_SUCCEEDED (rc))
                            {
                                /* Commit the insert, transaction is started by
                                   the first insert statement. */
                                rc = SQLEndTran (
                                    SQL_HANDLE_DBC, hCon, SQL_COMMIT);
                                if (!SQL_SUCCEEDED (rc))
                                {
                                    fprintf (
                                        stderr,
                                        "Sorry, I can't commit my changes "
                                        "to the database.");
                                }
                            }
                            else
                            {
                                (void) SQLEndTran (
                                    SQL_HANDLE_DBC, hCon, SQL_ROLLBACK);
                            }

                            if (SQL_SUCCEEDED (rc))
                            {
                                /* Query the database for the record created. */
                                rc = SQLExecDirect (
                                    hStmt,
                                    (SQLCHAR *) "SELECT myChar FROM info",
                                    SQL_NTS);
                                if (SQL_SUCCEEDED (rc))
                                {
                                    /* Bind SQL fields to program variables. */
                                    (void) SQLBindCol (
                                        hStmt, 1, SQL_C_CHAR, sz, sizeof (sz),
                                        &iLen);

                                    /* Fetch data from database into program
                                     * variables. */
                                    if (SQLFetch (hStmt) == SQL_SUCCESS)
                                    {
                                        printf ("%s\n", sz);
                                    }
                                    else
                                    {
                                        fprintf (
                                            stderr,
                                            "Sorry, I can't fetch any rows "
                                            "of data from the database.");
                                    }
                                }
                                else
                                {
                                    fprintf (
                                        stderr,
                                        "Sorry, I can't execute the query "
                                        "statement.");
                                }
                            }
                        }

                        /* Free the SQL statement handle. */
                        (void) SQLFreeHandle (SQL_HANDLE_STMT, hStmt);

                        /* Close the database. */
                        (void) SQLDisconnect (hCon);
                    }
                    else
                    {
                        fprintf (
                            stderr,
                            "Sorry, I can't allocate a SQL statement handle.");
                    }
                }
                (void) rdm_tfsFree (hTFS);
            }
        }
        else
        {
            fprintf (stderr, "Sorry, I can't open the hello_world database.");
        }

        /* Free the database handles. */
        (void) SQLFreeHandle (SQL_HANDLE_DBC, hCon);
        (void) SQLFreeHandle (SQL_HANDLE_ENV, hEnv);
    }
    else
    {
        fprintf (stderr, "Sorry, I can't allocate a database handle.");
    }

    return SQL_SUCCEEDED (rc) ? 0 : 1;
}

RDM_STARTUP_EXAMPLE (hello_world_ODBCTutorial)
