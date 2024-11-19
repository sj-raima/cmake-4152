#define RDM_NO_UNICODE 1
#include <stdio.h>
#include <string.h>
#include "rdm.h"
#include "..\..\..\source\base\rettables.h"

#define MADCAPSTYLE "<link href=\"../Resources/TableStyles/RefErrorTable.css\" rel=\"stylesheet\" MadCap:stylesheetType=\"table\" />"
#include "madcaphtml.h"
#include "errorSort.h"


#define FIRST_RSQL_ERRCODE  errINVHANDLE  /* from rsqlerrs.h */

static const char *const hdrRef = \
"        <table class=\"TableStyle-RefErrorTable\" cellspacing=\"0\" style=\"mc-table-style: url('../Resources/TableStyles/RefErrorTable.css');\">\n" \
"            <col class=\"TableStyle-RefErrorTable-Column-Column1\" />\n" \
"            <col class=\"TableStyle-RefErrorTable-Column-Column2\" />\n" \
"            <col class=\"TableStyle-RefErrorTable-Column-Column3\" />\n" \
"            <col class=\"TableStyle-RefErrorTable-Column-Column4\" />\n";
static const char *const hdrFnc = \
"        <table class=\"TableStyle-FncErrorTable\" cellspacing=\"0\" style=\"mc-table-style: url('../Resources/TableStyles/FncErrorTable.css');\">\n" \
"            <col class=\"TableStyle-FncErrorTable-Column-Column1\" />\n" \
"            <col class=\"TableStyle-FncErrorTable-Column-Column4\" />\n";
static const char *const theadRef = \
"            <thead>\n" \
"                <tr class=\"TableStyle-RefErrorTable-Head-Header1\">\n" \
"                    <th class=\"TableStyle-RefErrorTable-HeadE-Column1-Header1\">Name</th>\n" \
"                    <th class=\"TableStyle-RefErrorTable-HeadE-Column2-Header1\">Value</th>\n" \
"                    <th class=\"TableStyle-RefErrorTable-HeadE-Column3-Header1\">SQLState</th>\n" \
"                    <th class=\"TableStyle-RefErrorTable-HeadD-Column4-Header1\">Description</th>\n" \
"                </tr>\n" \
"            </thead>\n";
#if 0
static const char *const theadFnc = \
"            <thead>\n" \
"                <tr class=\"TableStyle-FncErrorTable-Head-Header1\">\n" \
"                    <th class=\"TableStyle-FncErrorTable-HeadE-Column1-Header1\">Name</th>\n" \
"                    <th class=\"TableStyle-FncErrorTable-HeadD-Column4-Header1\">Description</th>\n" \
"                </tr>\n" \
"            </thead>\n";
#endif
static const char *const tbody1 = \
        "            <tbody id=\"errors\">\n";
static const char *const tbody = "            <tbody>\n";
static const char *const col1Ref = \
        "                <tr class=\"TableStyle-RefErrorTable-Body-Body1\">\n" \
        "                    <td class=\"TableStyle-RefErrorTable-BodyB-Column1-Body1\">%s</td>\n";
static const char *const col1Fnc = \
"                <tr class=\"TableStyle-FncErrorTable-Body-Body1\">\n" \
"                    <td class=\"TableStyle-FncErrorTable-BodyB-Column1-Body1\">%s</td>\n";
static const char *const col2 = \
        "                    <td class=\"TableStyle-%sErrorTable-BodyB-Column2-Body1\">%d</td>\n";
static const char *const col3 = \
        "                    <td class=\"TableStyle-%sErrorTable-BodyB-Column3-Body1\">%s</td>\n";
static const char *const col4  = \
        "                    <td class=\"TableStyle-%sErrorTable-BodyA-Column4-Body1\">";
static const char *const emsg2  = \
        "                    </td>\n" \
        "                </tr>\n";
static const char *const footer = \
        "            </tbody>\n" \
    "        </table>\n";
#define ERRORTAB_SIZE (sizeof(sqlerrtab) / sizeof(RSQLERROR_DEFN))

static void buildFile (const char *name, int eval, const char *code, const char *msg)
{
    char fname[50];

    RDM_UNREF (eval);
    RDM_UNREF (code);
    sprintf (fname, "%s.flsnp", name);
#if 0
    auto fp = fopen(fname, "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s%s%s", head, hdrFnc, tbody);
        fprintf(fp, col1Fnc, name); /*lint !e905 */
        fprintf(fp, col4, "Fnc");
        fputConvert(fp, msg);
        fprintf(fp, "%s%s%s", emsg2, footer, tail);
        fclose(fp);
    }
#else
    RDM_UNREF (msg);
#endif
}

static const char *const ignoreTable[] =
{
    "eDDL_TBD",
    "eSQL_TBD",
    "eHA_",
    "eHTTP_",
    "eMIR_",
    "eREPLOG_",
    "eQA_",
    "eEMBED_",
	"eUNUSED_"
};
#define IGNORETAB_SIZE (sizeof(ignoreTable) / sizeof(char *))

static int filterCode(
    const char *name)
{
    size_t ii;
    size_t len;

    for (ii = 0; ii < IGNORETAB_SIZE; ii++)
    {
        len = strlen(ignoreTable[ii]);
        if (strncmp(name, ignoreTable[ii], len) == 0)
            return 1;
    }
    return 0;
}

int main(
    void)
{
    int                errCode;
    size_t             ii;
    FILE              *fp;
    CTX               *ctx;
    const char        *name;
    const char        *sqlstate;
    const char        *msg;
    unsigned int       errtab_size = (sizeof(rdm_errors) / sizeof(rdm_errors[0]));
    const RDM_ERRDEFN *pErr;

    ctx = errInit(errtab_size);
    for (ii = 0; ii < errtab_size; ii++)
    {
        pErr = &rdm_errors[ii];

        if (!filterCode(pErr->name))
        {
            errInsert(ctx, pErr->name, pErr->code, pErr->sqlstate, pErr->descr);
        }
    }

    /* output error codes and sorted by value */
    errSortByValue(ctx);
    errPosLast(ctx);
    fp = fopen("ErrorCodesByValue.flsnp", "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s%s%s%s", head, hdrRef, theadRef, tbody1);
        while (errGetPrev(ctx, &name, &errCode, &sqlstate, &msg) == 0)
        {
            buildFile(name, errCode, sqlstate, msg);
            if (errCode < 0) /* error codes > 0 */
            {
                fprintf(fp, col1Ref, name); /*lint !e905 */
                fprintf(fp, col2, "Ref", errCode); /*lint !e905 */
                fprintf(fp, col3, "Ref", sqlstate); /*lint !e905 */
                fprintf(fp, col4, "Ref");
                fputConvert(fp, msg);
                fprintf(fp, emsg2);
            }
        }

        fprintf(fp, "%s%s", footer, tail);
        fclose(fp);
    }

    /* output status codes sorted by value */
    fp = fopen("StatusCodes.flsnp", "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s%s%s%s", head, hdrRef, theadRef, tbody);
        errSortByValue(ctx);
        while (errGetNext(ctx, &name, &errCode, &sqlstate, &msg) == 0)
        {
            if (errCode >= 0) /* error codes > 0 */
            {
                fprintf(fp, col1Ref, name); /*lint !e905 */
                fprintf(fp, col2, "Ref", errCode); /*lint !e905 */
                fprintf(fp, col3, "Ref", sqlstate); /*lint !e905 */
                fprintf(fp, col4, "Ref");
                fputConvert(fp, msg);
                fprintf(fp, emsg2);
            }
        }

        fprintf(fp, "%s%s", footer, tail);
        fclose(fp);
    }

    /* output sorted by names table */
    fp = fopen("ErrorCodesByName.flsnp", "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s%s%s%s", head, hdrRef, theadRef, tbody);
        errSortByName(ctx);
        while (errGetNext(ctx, &name, &errCode, &sqlstate, &msg) == 0)
        {
            if (errCode < 0)
            {
                fprintf(fp, col1Ref, name); /*lint !e905 */
                fprintf(fp, col2, "Ref", errCode); /*lint !e905 */
                fprintf(fp, col3, "Ref", sqlstate); /*lint !e905 */
                fprintf(fp, col4, "Ref");
                fputConvert(fp, msg);
                fprintf(fp, emsg2);
            }
        }

        fprintf(fp, "%s%s", footer, tail);
        fclose(fp);
    }

    errTerm(ctx);
}
