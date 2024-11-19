/*
 * Raima Database Manager
 *
 * Copyright (c) 2019 Raima Inc.,  All rights reserved.
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the Birdstep LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 */
#include "qa.h"

RDM_RETCODE qa_threadBegin (
    PSP_THREAD_FCN fcn,      /**< [in] The entry and termination point for the execution of the thread */
    uint32_t stacksize,      /**< [in] The stack space to allocate for the new thread */
    void *arg,               /**< [in] Argument that will be passed on to the entry point function */
    PSP_THREAD_PTR_T pThread /**< [in] The created thread */
)
{
    return psp_threadBegin (pThread, fcn, stacksize, arg, 0, "");
}