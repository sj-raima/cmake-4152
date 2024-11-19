#include <stdio.h>
#include "example_fcns.h"
#include "rdmretcodeapi.h"
#include "rdmtfsapi.h"
#include "rdmdbapi.h"

/* \brief Standard error print format for popcorn examples
 *
 *  This demonstrates the usage of the rdm_retcodeGetName() and
 * rdm_retcodeGetDescription() functions.
 */
void print_errorEx (
    int rc,           /*< [in] RDM_RETCODE to analyze */
    const char *file, /*< [in] Sourcec filename */
    int line)         /*< [in] Lineno in source file */
{
    if (rc != sOKAY)
    {
        fprintf (
            stderr, "%s:%d:0: error: %d (%s): %s\n", file, line, rc,
            rdm_retcodeGetName (rc), rdm_retcodeGetDescription (rc));
    }
}

void timeMeasureBegin (perfTimer_t *timer)
{
    timer->start = rdm_timeMeasureMilliSecs ();
}

void timeMeasureEnd (perfTimer_t *timer)
{
    timer->end = rdm_timeMeasureMilliSecs ();
}

unsigned int timeMeasureDiff (perfTimer_t *timer)
{
    return (unsigned int) (timer->end - timer->start);
}

RDM_RETCODE exampleAllocTFS (RDM_TFS *pTFS)
{
    RDM_RETCODE rc;

    rc = rdm_rdmAllocTFS (pTFS);
    print_error (rc);
    if (rc == sOKAY)
    {
        rc = rdm_tfsInitialize (*pTFS);
        print_error (rc);
    }
    return rc;
}
/* \brief Initialize the RDM runtime library for use in the examples
 *rdm
 * This function initializes the RDM Transactional File Server (TFS) to use
 * the embedded implementation.  It also creates an RDM database handle
 * and opens the specified database in exclusive mode.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE exampleOpenNextDatabaseEx (
    RDM_TFS hTFS,          /*< [out] Pointer to the RDM TFS handle */
    RDM_DB *pDB,           /*< [out] Pointer to the RDM database handle */
    const char *dbName,    /*< [in] Database Name */
    const char *dbCatalog, /*< [in] schema catalog */
    int32_t empty,
    int32_t inmemory)
{
    RDM_RETCODE rc;

    rc = rdm_tfsAllocDatabase (hTFS, pDB);
    print_error (rc);

    if (rc == sOKAY)
    {
        /*
         *  Associate the compiled catalog with the DB handle.
         *  If the database does not exist on open, it will be created
         *  in the default DOCROOT directory.
         */
        rc = rdm_dbSetCatalog (*pDB, dbCatalog);
        print_error (rc);

        if (inmemory && rc == sOKAY)
        {
            rc = rdm_dbSetOption (*pDB, "storage", "inmemory_volatile");
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            rc = rdm_dbOpen (*pDB, dbName, RDM_OPEN_SHARED);
            print_error (rc);
        }

        if (empty && rc == sOKAY)
        {
            rc = rdm_dbStartUpdate (*pDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
            print_error (rc);

            if (rc == sOKAY)
            {
                rc = rdm_dbDeleteAllRowsFromDatabase (*pDB);
                print_error (rc);
                rdm_dbEnd (*pDB);
            }
        }

        /* If any of the above DB operations failed, free the DB handle
         */
        if (rc != sOKAY)
        {
            rdm_dbFree (*pDB);
        }
    }
    return rc;
}

RDM_RETCODE exampleOpenDatabaseEx (
    RDM_TFS *pTFS,         /*< [out] Pointer to the RDM TFS handle */
    RDM_DB *pDB,           /*< [out] Pointer to the RDM database handle */
    const char *dbName,    /*< [in] Database Name */
    const char *dbCatalog, /*< [in] schema catalog */
    int32_t empty,
    int32_t inmemory)
{
    RDM_RETCODE rc;

    rc = exampleAllocTFS (pTFS);
    if (rc == sOKAY)
    {
        if (inmemory)
        {
            /* since were testing an inmemory database, make sure there isn't an
             * old db in the default DOCROOT with the same name */
            rdm_tfsDropDatabase (*pTFS, dbName);
        }

        rc = exampleOpenNextDatabaseEx (
            *pTFS, pDB, dbName, dbCatalog, empty, inmemory);

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

void exampleCleanupEx (
    RDM_TFS hTFS, /*< [out] Pointer to the TFS handle to be terminated */
    RDM_DB hDB1,  /*< [out] Pointer to the DB handle to be terminated */
    RDM_DB hDB2)  /*< [out] Pointer to the DB handle to be terminated */
{
    /* close the database */
    rdm_dbClose (hDB1);
    if (hDB2)
        rdm_dbClose (hDB2);

    /* free the database handle */
    rdm_dbFree (hDB1);
    if (hDB2)
        rdm_dbFree (hDB2);

    /* free the TFS handle */
    rdm_tfsFree (hTFS);
}
