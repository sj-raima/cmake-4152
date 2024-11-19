/*
 * Raima Database Manager
 *
 * Copyright (C) 2020 Raima Inc.,  All rights reserved.
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the Raima LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 */

/** \file 4787Test.c
 *  \brief 
 */

#include "rdmrdmapi.h"
#include "rdmtfsapi.h"
#include "rdmretcodeapi.h"

#include <stdio.h>

int main (int argc, char *argv[])
{
    RDM_RETCODE rc;
    RDM_TFS tfs;
    RDM_TFS tfs2;

    RDM_UNREF (argc);
    RDM_UNREF (argv);
    rc = rdm_rdmAllocTFS (&tfs);
    if (rc == sOKAY)
    {
        rc = rdm_tfsSetOption (tfs, "listen", "shm");
        if (rc == sOKAY)
        {
            rc = rdm_tfsInitialize (tfs);
        }

        rc = rdm_rdmAllocTFS (&tfs2);
        if (rc == sOKAY)
        {
            rc = rdm_tfsSetOption (tfs2, "listen", "shm");
            if (rc == sOKAY)
            {
                rc = rdm_tfsInitialize (tfs2);
            }
            if (rc == eTFS_DOCROOTUSED)
            {
                rc = rdm_tfsFree (tfs2);
            }
            if (rc != sOKAY)
            {
                printf ("ERROR: rdm_tfsFree failed (%s, %s)\n", rdm_retcodeGetName (rc),
                        rdm_retcodeGetDescription (rc));
                return 1;
            }
        }
        rc = rdm_tfsFree (tfs);
    }

    return 0;
}
