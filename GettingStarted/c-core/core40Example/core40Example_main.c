#include "rdm.h"
#include "rdmapi.h"
#include "rdmtfsapi.h"
#include "core40Example_cat.h"
#include "privatekey.h"
#include "certificate.h"
#include "rdmstartupapi.h"

#include <stdio.h>
#include <stdlib.h>

RDM_EXPORT int32_t main_sslTFSTutorial(int32_t argc, const char* const* argv)
{
    RDM_RETCODE rc;
    RDM_TFS tfs = NULL;
    RDM_DB db = NULL;

    RDM_UNREF(argc);
    RDM_UNREF(argv);

    /* Allocate a TFS Handle */
    rc = rdm_rdmAllocTFS(&tfs);
    if (rc == sOKAY)
    {
        /* Setup TFS to use ONLY SSL encrypted connections.
        *  If this is not set, the TFS will allow SSL and non-SSL
        *  connections.
        */
        rc = rdm_tfsSetOption (tfs, "force_ssl", "1");

        /* Set an SSL private key for this RDM_TFS instance. 
        *  The pointer to the privateKeyBuffer must remain active
        *  for the duration of this RDM_TFS instance.
        */
        if (rc == sOKAY)
        {
            rc = rdm_tfsSetKey (tfs, privateKeyBuffer);
        }

        /* Set an SSL certificate for this RDM_TFS instance.
        *  The pointer to the certificateBuffer must remain active
        *  for the duration of this RDM_TFS instance.
        */
        if (rc == sOKAY)
        {
            rc = rdm_tfsSetCertificate (tfs, certificateBuffer);
        }

        /* Setup TFS to listen for connection on startup.
        *  If not set, the rdm_tfsEnableListener() function can be used
        *  start the listener.
        */
        if (rc == sOKAY)
        {
            rc = rdm_tfsSetOption (tfs, "listen", "1");
        }

        /* Setup TFS to port/name to listen on (default is 21553) */
        if (rc == sOKAY)
        {
            rc = rdm_tfsSetOption (tfs, "name", "21560");
        }

        /* Initialize the TFS handle */
        if (rc == sOKAY)
        {
            rc = rdm_tfsInitialize (tfs);
        }

        /*
        * The RaimaDB TFS is accepting connections when we reach this point.
        * We can connect to the running TFS instance within this application
        * or we can wait until we're signalled to shutdown.
        */

        /* Allocate a database handle */
        if (rc == sOKAY)
        {
            rc = rdm_tfsAllocDatabase (tfs, &db);
            if (rc == sOKAY)
            {
                rc = rdm_dbSetCatalog (db, core40Example_cat);
            }
            /* Setup DB to use SSL encrypted connection.
             *  If not set, the rdm_dbOpen will attempt a non-SSL connection.
             */
            rc = rdm_dbSetOption (db, "use_ssl", "1");

            if (rc == sOKAY)
            {
                /* Open the database using a TCP/IP URI specification */
                rc = rdm_dbOpen (
                    db, "tfs-tcp://localhost:21560/core40", RDM_OPEN_EXCLUSIVE);
                if (rc == sOKAY)
                {
                    size_t outSize;
                    size_t outSize1;
                    char *buffer;

                    printf ("Connected to the remote TFS using tcp/ip "
                            "(tfs-tcp://localhost:21560/core40)\n");

                    /*
                    ** Retrieve the certificate from the TFS
                    *
                    * The first call to rdm_dbGetCertificate below retrieves the
                    *size buffer needed to hold the entire certificate.
                    */
                    rc = rdm_dbGetCertificate (db, NULL, 0, &outSize);
                    if (rc == sOKAY)
                    {
                        /* Allocate a buffer to receive the certificate */
                        buffer = malloc (outSize);
                        rc = rdm_dbGetCertificate (
                            db, buffer, outSize, &outSize1);
                        puts (buffer);
                        free (buffer);
                    }

                    /* Close the database */
                    rdm_dbClose (db);
                }
                else if (rc == eTX_CONNECT)
                {
                    printf ("Could not connect to the remote TFS, make sure "
                            "one has "
                            "been started.\n");
                }
                rdm_dbFree (db);
            }
            rdm_tfsFree (tfs); /* The will shutdown the listening TFS */
        }
    }

    printf (
        "Exit Status (%s -%s)\n", rdm_retcodeGetName (rc),
        rdm_retcodeGetDescription (rc));

    if (rc != sOKAY && rc != eNOTIMPLEMENTED_TRANSPORT_SSL)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

RDM_STARTUP_EXAMPLE(sslTFSTutorial)
