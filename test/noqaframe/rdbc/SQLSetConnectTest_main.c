/********************************************************
 * No QA FRAMEWORK test for testing the SQLSetConnectAttr
 * Test is used because many attributes require TFSINIT
 * access, but QA FRAMEWORK already initializes TFS
 * Attributes Tested : SQL_ATTR_RDM_TFSINIT_STDOUT,
 * SQL_ATTR_RDM_TFSINIT_VERBOSE, SQL_ATTR_RDM_TFSINIT_LOGFILE
 * SQL_ATTR_RDM_TFS_PORT
 * Date: 9/07/11
 * Author: Kevin Hooks
 ********************************************************/

/********************************************************
 * Includes
 *********************************************************/
#include "sqlrext.h"
#include "sql.h"
#include "rdm.h"
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <stdlib.h>

/***********************************************
 * Global Variables
 ***********************************************/
static const SQLCHAR *const commit = (const SQLCHAR *) "commit";

/**********************************************
 * Static Function Prototypes
 **********************************************/
static SQLRETURN init_case (RDM_TFS *);
SQLRETURN cleanup_case (RDM_TFS);
#if 0
static SQLRETURN TFS_VERBOSE_TEST (void);
#endif

/****************************************************************
 * init_case() : Main test function, since attr are set
 *               during initialization. Creates the database
 *               Puts tfs in verbose and the stdout is
 *               directed to txt.txt file instead of the terminal
 *****************************************************************/
static SQLRETURN init_case (RDM_TFS *tfs)
{
    SQLHENV hEnv = SQL_NULL_HENV;    /* Environment handle */
    SQLHDBC hCon = SQL_NULL_HDBC;    /* Connect Handle     */
    SQLHSTMT hStmt = SQL_NULL_HSTMT; /* Statement Handle   */

    SQLRETURN stat;
    /* Port number is left as default because of current TFS Set Up not capable of other ports */
    /* Make additions to test later */
    const SQLCHAR *createDB = (const SQLCHAR *) "create database Con_dB";
    const SQLCHAR *createTable = (const SQLCHAR *) "create table table01( \
                                    f00 rowid primary key, \
                                    f01 char, \
                                    f02 char(3), \
                                    f03 varchar(41), \
                                    f07 tinyint, \
                                    f08 smallint, \
                                    f09 integer, \
                                    f10 bigint, \
                                    f11 date, \
                                    f12 time, \
                                    f13 timestamp, \
                                    f14 double, \
                                    f15 double precision, \
                                    f16 float, \
                                    f17 real, \
                                    f18 long varchar \
                                )";

    stat = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv); /* Allocates the env Handle */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "env handle error");
        return stat;
    }

    stat = SQLSetEnvAttr (hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0); /* Sets ODBC Version */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "env attr error");
        return stat;
    }

    stat = SQLAllocHandle (SQL_HANDLE_DBC, hEnv, &hCon); /* Allocates Connection Handle */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "conn handle error");
        return stat;
    }
    /*******************************************
     * Sets connection attributes
     ********************************************/
    stat = rdm_rdmAllocTFS (tfs);
    if (stat == sOKAY)
    {
        stat = rdm_tfsInitialize (*tfs);
        if (stat != sOKAY)
        {
            rdm_tfsFree (*tfs);
        }
    }
    if (stat != sOKAY)
    {
        printf ("%s \n", "conn attr error");
        return stat;
    }

    stat = SQLSetConnectAttr (hCon, SQL_ATTR_RDM_TFS_HANDLE, (SQLPOINTER) *tfs, SQL_IS_POINTER);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "tfs alloc error");
        return stat;
    }
    /**********************************************/
    stat =
        SQLConnect (hCon, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS); /* Connects to database */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "connection error");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_STMT, hCon, &hStmt); /* Allocates statement handle */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "stmt handle error");
        return stat;
    }

    stat = SQLPrepare (hStmt, createDB, SQL_NTS); /* prepares statement to create database */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "create database error");
        return stat;
    }
    stat = SQLExecute (hStmt); /* executes the create database statement */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "execute stmt create db error");
        return stat;
    }
    stat = SQLPrepare (hStmt, createTable, SQL_NTS); /* prepares statement to create table */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "create table error");
        return stat;
    }
    stat = SQLExecute (hStmt); /* Executes the create table statement */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "execute stmt create table error");
        return stat;
    }

    stat = SQLPrepare (hStmt, commit, SQL_NTS); /* Allocates commits database changes statement */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "commit fail");
        return stat;
    }

    stat = SQLExecute (hStmt); /* Executes commit statement */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "execute commit fail");
        return stat;
    }

    stat = SQLDisconnect (hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "disconnect fail");
        return stat;
    }
    (void) SQLFreeHandle (SQL_HANDLE_DBC, hCon);
    (void) SQLFreeHandle (SQL_HANDLE_ENV, hEnv);

    return stat;
}

#if defined(NOT_USED)
/*************************************************************
 * inmem_test() : Function that tests the attribute
 *                SQL_ATTR_RDM_TFSINIT_DISKLESS, by putting the
 *                database in memory.
 **************************************************************/

static SQLRETURN inmem_case (void)
{
    SQLHENV hEnv = SQL_NULL_HENV;    /* Environment handle */
    SQLHDBC hDbc = SQL_NULL_HDBC;    /* Connect Handle     */
    SQLHSTMT hStmt = SQL_NULL_HSTMT; /* Statement Handle   */
    SQLRETURN stat;

    const char *createDB = "create database Con_dB inmemory";
    const char *createTable = "create table table01( \
                                    f00 rowid, \
                                    f01 char, \
                                    f02 char(3), \
                                    f06 wvarchar(20), \
                                    f07 tinyint, \
                                    f08 smallint, \
                                    f09 integer, \
                                    f10 bigint, \
                                    f11 date, \
                                    f12 time, \
                                    f13 timestamp, \
                                    f14 double, \
                                    f15 double precision, \
                                    f16 float, \
                                    f17 real, \
                                    f18 long varchar \
                                )";

    stat = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv); /* Allocates the env Handle */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "env handle error");
        return stat;
    }

    stat = SQLSetEnvAttr (hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0); /* Sets ODBC Version */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "env attr error");
        return stat;
    }

    stat = SQLAllocHandle (SQL_HANDLE_DBC, hEnv, &hDbc); /* Allocates Connection Handle */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "conn handle error");
        return stat;
    }

    /*******************/
    stat = SQLSetConnectAttr (hDbc, SQL_ATTR_RDM_TFSINIT_DISKLESS, (SQLPOINTER *) SQL_TRUE, 0);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "conn attr error");
        return stat;
    }

    /******************/

    stat =
        SQLConnect (hDbc, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS); /* Connects to database */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "connection error");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_STMT, hDbc, &hStmt); /* Allocates statement handle */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "stmt handle error");
        return stat;
    }

    stat = SQLExecDirect (hStmt, createDB, SQL_NTS); /* prepares statement to create database */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "create database error");
        return stat;
    }

    stat = SQLExecDirect (hStmt, createTable, SQL_NTS); /* prepares statement to create table */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "create table error");
        return stat;
    }

    stat = SQLExecDirect (hStmt, commit, SQL_NTS); /* Allocates commits database changes statement */
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "commit fail");
        return stat;
    }

    stat = SQLDisconnect (hDbc);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "disconnect fail");
        return stat;
    }

    (void) SQLFreeHandle (SQL_HANDLE_DBC, hDbc);
    (void) SQLFreeHandle (SQL_HANDLE_ENV, hEnv);

    return stat;
}

/************************************************************
 * TFS_VERBOSE_TEST() : Function that tests the
 *                       SQL_ATTR_RDM_TFSINIT_VERBOSE attribute
 *                      in SQLSetConnectAttr.
 *                      commented for later  use 9/13/11
 *************************************************************/

static SQLRETURN TFS_VERBOSE_TEST (void)
{
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hCon = SQL_NULL_HDBC;
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    RDM_TFS tfs = NULL;
    SQLRETURN stat;
    const SQLCHAR *insert = (const SQLCHAR *) "insert into table01 (f07) values (?)";
    /*const SQLCHAR *select = (const SQLCHAR *) "select f07 from table01"; */
    uint16_t value = 35;
    /*SQLLEN         rowCount; */

    stat = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "env handle error");
        return stat;
    }

    stat = SQLAllocHandle (SQL_HANDLE_DBC, hEnv, &hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "conn handle error");
        return stat;
    }
    stat = SQLSetConnectAttr (hCon, SQL_ATTR_RDM_TFSINIT_VERBOSE, (SQLPOINTER *) SQL_TRUE, 0);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "conn attr error");
        return stat;
    }

    stat = rdm_rdmAllocTFS ("", &tfs);
    if (stat != sOKAY)
    {
        printf ("%s \n", "conn attr error");
        return stat;
    }

    stat = SQLSetConnectAttr (hCon, SQL_ATTR_RDM_TFS_HANDLE, (SQLPOINTER) tfs, SQL_IS_POINTER);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "tfs alloc error");
        return stat;
    }

    stat = SQLConnect (hCon, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "connect error");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_STMT, hCon, &hStmt);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "stmt handle error");
        return stat;
    }
    stat = SQLExecDirect (hStmt, "USE Con_dB", SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "open db error");
        return stat;
    }
    stat = SQLPrepare (hStmt, insert, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "insert error");
        return stat;
    }
    stat = SQLBindParameter (hStmt, 1, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 1, 0, &value, 0, NULL);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "bind param error");
        return stat;
    }
    stat = SQLExecute (hStmt);

    SQLDisconnect (hCon);                 /* Disconnects from server, closing database */
    SQLFreeHandle (SQL_HANDLE_DBC, hCon); /* Frees remaining handles       */
    SQLFreeHandle (SQL_HANDLE_ENV, hEnv);

    return stat;
}
#endif

/*****************************************************************************
 * rdbcTFST_case() : Simple case that sets the tfs type to TFST
 *
 ************************************************************************/
static SQLRETURN rdbcTFST_case (RDM_TFS tfs)
{
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hCon = SQL_NULL_HDBC;
    SQLHSTMT hStmt;
    SQLRETURN stat;

    const SQLCHAR *select = (const SQLCHAR *) "select * from table01";
    const SQLCHAR *open = (const SQLCHAR *) "USE Con_dB";

    stat = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Allocate Env Handle Fail");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_DBC, hEnv, &hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Allocate Conn Handle Fail");
        return stat;
    }

    stat = SQLSetConnectAttr (hCon, SQL_ATTR_RDM_TFS_HANDLE, (SQLPOINTER) tfs, SQL_IS_POINTER);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "conn attr error");
        return stat;
    }

    stat = SQLConnect (hCon, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Connect to DB Fail");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_STMT, hCon, &hStmt);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Allocate Statement Handle Fail");
        return stat;
    }
    stat = SQLExecDirect (hStmt, open, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s fail\n", open);
        return stat;
    }
    stat = SQLExecDirect (hStmt, select, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Open DB Fail");
        return stat;
    }

    stat = SQLDisconnect (hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Disconnect Fail");
        return stat;
    }

    stat = SQLFreeHandle (SQL_HANDLE_DBC, hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Free Conn Handle Fail");
        return stat;
    }
    stat = SQLFreeHandle (SQL_HANDLE_ENV, hEnv);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Free Env Handle Fail");
        return stat;
    }

    return stat;
}

#if defined(NOT_USED)
/*****************************************************************************
 * rdbcTFSR_case() : Simple case that sets the tfs type to TFSR
 *                   Creates a database using TFSR,
 *                   If database cannot be created using TFSR then the
 *                   attribute has not been set correctly.
 ************************************************************************/
static SQLRETURN rdbcTFSR_case (void)
{
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hCon = SQL_NULL_HDBC;
    SQLHSTMT hStmt;
    SQLRETURN stat;

    const char *createDB = "create database Con_dBTFSR";
    const char *createTable = "create table table01( \
                                    f00 rowid primary key, \
                                    f01 char, \
                                    f02 char(3), \
                                    f03 varchar(41), \
                                    f07 tinyint, \
                                    f08 smallint, \
                                    f09 integer, \
                                    f10 bigint, \
                                    f11 date, \
                                    f12 time, \
                                    f13 timestamp, \
                                    f14 double, \
                                    f15 double precision, \
                                    f16 float, \
                                    f17 real, \
                                    f18 long varchar \
                                )";

    stat = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Allocate Env Handle Fail");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_DBC, hEnv, &hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Allocate Conn Handle Fail");
        return stat;
    }
    stat = SQLSetConnectAttr (hCon, SQL_ATTR_RDM_TFSINIT_TYPE, (SQLPOINTER) SQL_TFSTYPE_REMOTE, 0);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Set ConnectAttr Fail");
        return stat;
    }
    stat = SQLConnect (hCon, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS, SQL_EMPTSTR, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Connect to DB Fail");
        return stat;
    }
    stat = SQLAllocHandle (SQL_HANDLE_STMT, hCon, &hStmt);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Allocate Statement Handle Fail");
        return stat;
    }
    stat = SQLExecDirect (hStmt, createDB, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Create DB Fail");
        return stat;
    }
    stat = SQLExecDirect (hStmt, createTable, SQL_NTS);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Create Table Fail");
        return stat;
    }

    stat = SQLDisconnect (hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Disconnect Fail");
        return stat;
    }

    stat = SQLFreeHandle (SQL_HANDLE_DBC, hCon);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Free Conn Handle Fail");
        return stat;
    }
    stat = SQLFreeHandle (SQL_HANDLE_ENV, hEnv);
    if (!SQL_SUCCEEDED (stat))
    {
        printf ("%s \n", "Free Env Handle Fail");
        return stat;
    }

    return stat;
}
#endif

/***************************************
 * cleanup_case() : Simple case to remove
 *                  old database files
 *                  Specific to this test
 ****************************************/
SQLRETURN cleanup_case (RDM_TFS tfs)
{
    RDM_RETCODE rc;

    rc = rdm_tfsDropDatabase (tfs, "CON_DB");
    /*rdm_tfsDropDatabase (tfs, "Con_dBTFSR"); */
    if (rc == sOKAY)
    {
        rdm_tfsFree (tfs);
    }

    return SQL_SUCCESS;
}

/******************************************************
 * main() : main function that performs the rest of the
 *          cases and checks to see if they succeed.
 * Author : Kevin Hooks
 * Date   : 9/12/11
 *******************************************************/
RDM_C_EXPORT int32_t main_SQLSetConnectTest (int32_t argc, const char *const *argv)
{
    SQLRETURN stat;
    int32_t ret = EXIT_SUCCESS;
    RDM_TFS tfs;

    ((void) argc);
    ((void) argv);

    stat = init_case (&tfs); /* creates the database */
    if (!SQL_SUCCEEDED (stat))
    {
        ret = EXIT_FAILURE;
        printf ("%s \n", "fail");
    }
    else
    {
        printf ("%s \n", "Create database Success");
    }

#if 0
    stat = inmem_case();               /* creates the database */
    if(!SQL_SUCCEEDED(stat)) {
        printf("%s \n", "fail");
    } else {
        printf("%s \n", "In memory success");
    }
#endif

    stat = rdbcTFST_case (tfs);
    if (!SQL_SUCCEEDED (stat))
    {
        ret = EXIT_FAILURE;
        printf ("%s \n", "fail");
    }
    else
    {
        printf ("%s \n", "TFST Success");
    }

#if 0
    stat = rdbcTFSR_case();
    if(!SQL_SUCCEEDED(stat)) {
            printf("%s \n", "fail");
    } else {
            printf("%s \n", "TFSR Success");
    }
#endif

    stat = cleanup_case (tfs);
    if (!SQL_SUCCEEDED (stat))
    {
        ret = EXIT_FAILURE;
        printf ("%s \n", "fail");
    }
    else
    {
        printf ("%s \n", "Cleanup database Success");
    }

    return ret;
}

RDM_STARTUP_TEST (SQLSetConnectTest)
