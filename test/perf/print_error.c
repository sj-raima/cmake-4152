/* ----------------------------------------------------------------------------
 *
 * Raima Database Manager
 *
 * Copyright (c) 2019 Raima Inc.,  All rights reserved.
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the Raima LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 *
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include "print_error.h"
#include "rdmretcodeapi.h"

/*! \brief Standard error print format for popcorn examples
 *
 *  This demonstrates the usage of the rdm_retcodeGetName() and rdm_retcodeGetDescription() functions.
 */
void print_errorEx (RDM_RETCODE rc,   /**< [in] RDM_RETCODE to analyze */
                    const char *file, /**< [in] Sourcec filename */
                    int line)         /**< [in] Lineno in source file */
{
    if (rc != sOKAY)
    {
        fprintf (stderr, "%s:%d:0: error: %d (%s): %s\n", file, line, rc, rdm_retcodeGetName (rc),
                 rdm_retcodeGetDescription (rc));
    }
}
