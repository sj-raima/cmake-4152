/*
 * Demonstrates client/server with writing/reading and snapshot reads
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rdm.h"
#include "core38Example_structs.h"
#include "core38Example_cat.h"
#include "example_fcns.h"
#include "rdmstartupapi.h"

RDM_CMDLINE cmd;
const char* const description = "Demonstrates client/server with writing/reading and snapshot reads";
const RDM_CMDLINE_OPT opts[] = { 
    {"", "writer", NULL, "Start writer process with tfs_emded"},
    {"", "reader", "s=*", "Start reader process and connect to given URI"},
    {"", "snapshot", "s=*", "Start snapshot reader process and connect to given URI"},
    {"", "timeout", "i=#", "Set writer timeout in seconds"},
    {"", "end", "i=#", "Stop after NN minutes"},
    {NULL, NULL, NULL, NULL} };

static uint32_t timeout = 0;
static uint32_t minutes = 5;

static RDM_RETCODE setTimeout(RDM_DB db)
{
    char timeoutString[100];

    printf("Timeout: %u Second\n", timeout);
    sprintf(timeoutString, "%u", timeout);
    return rdm_dbSetOption(db, "timeout", timeoutString);

}

static RDM_RETCODE startEmbedTFS(RDM_TFS* pTFS)
{
    RDM_RETCODE rc;

    rc = rdm_rdmAllocTFS(pTFS);
    print_error(rc);
    if (rc == sOKAY)
    {
        rc = rdm_tfsSetOptions(*pTFS, "tfstype=embed;listen=on");
        print_error(rc);
        if (rc == sOKAY)
        {
            rc = rdm_tfsInitialize(*pTFS);
            print_error(rc);
        }

        /* If any setup options have occurred, free the handle */
        if (rc != sOKAY)
            rdm_tfsFree(*pTFS);
    }
    return rc;
}

static RDM_RETCODE startRemoteTFS(RDM_TFS* pTFS)
{
    RDM_RETCODE rc;

    rc = rdm_rdmAllocTFS(pTFS);
    print_error(rc);
    if (rc == sOKAY)
    {
        rc = rdm_tfsSetOptions(*pTFS, "tfstype=remote");
        print_error(rc);
        if (rc == sOKAY)
        {
            rc = rdm_tfsInitialize(*pTFS);
            print_error(rc);
        }

        /* If any setup options have occurred, free the handle */
        if (rc != sOKAY)
            rdm_tfsFree(*pTFS);
    }
    return rc;
}

static void shutdownEmbedTFS(RDM_TFS tfs)
{
    RDM_RETCODE rc;

    /* shutdown ability for new clients to connect */
    puts("** Disable new connections.");
    rc = rdm_tfsDisableListener(tfs);
    print_error(rc);
    puts("** Drop existing connections.");
    /* drop all currently connected clients */
    rc = rdm_tfsKillAllRemoteConnections(tfs, "core38");
    print_error(rc);
    puts("** Remove database image.");
    rc = rdm_tfsDropDatabase(tfs, "core38");
    print_error(rc);
}

static RDM_RETCODE writerProcess(RDM_DB db)
{
    RDM_RETCODE rc;
    perfTimer_t timer;
    READINGS recordData;
    uint32_t exitTime = minutes * 60 * 1000;  /* change time to millisecs */
    uint32_t counter = 0;

    puts("Starting Writer Process");
    timeMeasureBegin(&timer);
    while (true)
    {
        rc = rdm_dbStartUpdate(db, RDM_LOCK_ALL, 0, NULL, 0, NULL);
        if (rc == eUNAVAIL)
        {
            printf("Timeout after inserting %u rows\n", counter);
            counter = 0;
        }
        else if (rc == sOKAY)
        {
            rdm_timestampNow(0, &recordData.r_time);
            recordData.r_value = rand();
            strcpy(recordData.r_desc, "Sample description");
            rc = rdm_dbInsertRow(db, TABLE_READINGS, &recordData, sizeof(recordData), NULL);
            print_error(rc);
            if (rc == sOKAY)
            {
                counter++;
                rdm_dbEnd(db);
            }
            else
            {
                rdm_dbEndRollback(db);
            }
        }
        else
        {
            print_error(rc);
            break;
        }
        timeMeasureEnd(&timer);
        if (timeMeasureDiff(&timer) >= exitTime)
        {
            printf("Normal Shutdown after inserting %u rows\n", counter);
            break;
        }
    }
    return rc;
}

static RDM_RETCODE readerProcess(RDM_DB db)
{
    RDM_RETCODE rc;
    perfTimer_t timer;
    uint32_t exitTime = minutes * 60 * 1000;  /* change time to millisecs */

    puts("Starting Reader Process");
    timeMeasureBegin(&timer);
    while (true)
    {
        rc = rdm_dbStartRead(db, RDM_LOCK_ALL, 0, NULL);
        print_error(rc);
        if (rc == eUNAVAIL)
        {
            puts("Lock timeout");
        }
        else if (rc == sOKAY)
        {
            RDM_CURSOR cursor = NULL;
            uint32_t counter = 0;

            rc = rdm_dbGetRows(db, TABLE_READINGS, &cursor);
            print_error(rc);
            if (rc == sOKAY)
            {
                for (rc = rdm_cursorMoveToFirst(cursor); rc == sOKAY; rc = rdm_cursorMoveToNext(cursor))
                {
                    READINGS recordData;

                    rc = rdm_cursorReadRow(cursor, &recordData, sizeof(recordData), NULL);
                    /* All we'll do here is just read the row */
                    if (rc == sOKAY)
                        counter++;
                }

                /* sENDOFCURSOR is the expected return code */
                if (rc == sENDOFCURSOR)
                    rc = sOKAY;
                print_error(rc);

                rdm_dbEnd(db);
                printf("Read %u rows\n", counter);

            }
        }
        else
        {
            break;
        }

        /* See if it is time to exit this loop */
        timeMeasureEnd(&timer);
        if (timeMeasureDiff(&timer) >= exitTime)
        {
            puts("Normal Shutdown");
            break;
        }
    }
    return rc;
}

static RDM_RETCODE snapshotProcess(RDM_DB db)
{
    RDM_RETCODE rc;
    perfTimer_t timer;
    uint32_t exitTime = minutes * 60 * 1000;  /* change time to millisecs */

    puts("Starting Reader Process");
    timeMeasureBegin(&timer);
    while (true)
    {
        rc = rdm_dbStartSnapshot(db, RDM_LOCK_ALL, 0, NULL);
        print_error(rc);
        if (rc == eUNAVAIL)
        {
            puts("Lock timeout");
        }
        else if (rc == sOKAY)
        {
            RDM_CURSOR cursor = NULL;
            uint32_t counter = 0;

            rc = rdm_dbGetRows(db, TABLE_READINGS, &cursor);
            print_error(rc);
            if (rc == sOKAY)
            {
                for (rc = rdm_cursorMoveToFirst(cursor); rc == sOKAY; rc = rdm_cursorMoveToNext(cursor))
                {
                    READINGS recordData;

                    rc = rdm_cursorReadRow(cursor, &recordData, sizeof(recordData), NULL);
                    /* All we'll do here is just read the row */
                    if (rc == sOKAY)
                        counter++;
                }

                /* sENDOFCURSOR is the expected return code */
                if (rc == sENDOFCURSOR)
                    rc = sOKAY;
                print_error(rc);

                rdm_dbEnd(db);
                printf("Read %u rows\n", counter);

            }
        }
        else
        {
            break;
        }
        
        /* See if it is time to exit this loop */
        timeMeasureEnd(&timer);
        if (timeMeasureDiff(&timer) >= exitTime)
        {
            puts("Normal Shutdown");
            break;
        }
    }
    return rc;
}

/* Start Embed TFS and being inserting rows into the database.
*/
static RDM_RETCODE startWriterWithEmbeddedServer(void)
{
    RDM_RETCODE rc;
    RDM_TFS tfs;
    RDM_DB db;
    
    printf("Writer with embedded TFS\n\nShutdown in %u minutes\n", minutes);
    rc = startEmbedTFS(&tfs);
    if (rc == sOKAY)
    {
        rc = exampleOpenNextDatabase(tfs, &db, "core38", core38Example_cat);
        if (rc == sOKAY)
        {
            rc = setTimeout(db);
            print_error(rc);
        }
        if (rc == sOKAY)
        {
            rc = writerProcess(db);
            rdm_dbClose(db);
            shutdownEmbedTFS(tfs);
        }
        rdm_tfsFree(tfs);
    }
    return rc;
}

static RDM_RETCODE startReader(const char* uri)
{
    RDM_RETCODE rc;
    RDM_TFS tfs;
    RDM_DB db;

    printf("Reader connecting to TFS at %s\n\nShutdown in %u minutes\n", uri, minutes);
    rc = startRemoteTFS(&tfs);
    if (rc == sOKAY)
    {
        char dbname[1000];
        sprintf(dbname, "%s/core38", uri);
        printf("Attempting to open: %s", dbname);
        rc = exampleOpenNextDatabase(tfs, &db, dbname, NULL);
        if (rc == sOKAY)
        {
            rc = setTimeout(db);
            print_error(rc);
        }
        if (rc == sOKAY)
        {
            rc = readerProcess(db);
            rdm_dbClose(db);
        }
        rdm_tfsFree(tfs);
    }
    return rc;
}

static RDM_RETCODE startSnapshotReader(const char *uri)
{
    RDM_RETCODE rc;
    RDM_TFS tfs;
    RDM_DB db;

    printf("Snapshot Reader connecting to TFS at %s\n\nShutdown in %u minutes\n", uri, minutes);
    rc = startRemoteTFS(&tfs);
    if (rc == sOKAY)
    {
        char dbname[1000];
        sprintf(dbname, "%s/core38", uri);
        printf("Attempting to open: %s", dbname);
        rc = exampleOpenNextDatabase(tfs, &db, dbname, NULL);
        if (rc == sOKAY)
        {
            rc = setTimeout(db);
            print_error(rc);
        }
        if (rc == sOKAY)
        {
            rc = snapshotProcess(db);
            rdm_dbClose(db);
        }
        rdm_tfsFree(tfs);
    }
    return rc;
}



RDM_EXPORT int32_t main_core38(int32_t argc, const char* const* argv)
{
    RDM_RETCODE rc;
    const char* opt;
    const char* optarg;
    char cMode = 0;

    rc = rdm_cmdlineInit(&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error(rc);

    if (rc == sOKAY)
    {
        while ((opt = rdm_cmdlineNextLongOption(&cmd, &optarg)) != 0)
        {
            switch (*opt)
            {
            case 'w':
                if (cMode)
                {
                    puts("TFS mode already set.");
                    rc = sCMD_USAGE;
                }
                else
                {
                    cMode = *opt;
                }
                break;
            case 'r':
                if (cMode)
                {
                    puts("TFS mode already set.");
                    rc = sCMD_USAGE;
                }
                else
                {
                    cMode = *opt;
                }
                break;
            case 's':
                if (cMode)
                {
                    puts("TFS mode already set.");
                    rc = sCMD_USAGE;
                }
                else
                {
                    cMode = *opt;
                }
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 'e':
                minutes = atoi(optarg);
                break;
            default:
                break;
            }
        }

        if (rc == sOKAY)
        {
            switch (cMode)
            {
            case 'w':
                rc = startWriterWithEmbeddedServer();
                break;
            case 'r':
                rc = startReader(optarg);
                break;
            case 's':
                rc = startSnapshotReader(optarg);
                break;
            default:
                puts("No Action Defined. Exiting.");
                break;
            }
        }
    }
    return (int)rc;
}

RDM_STARTUP_EXAMPLE (core38)
