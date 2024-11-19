#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core07_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core07_cat.h"

RDM_CMDLINE cmd;
const char *const description = "Demonstrates using an index for ordering";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core07 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (all existing contents removed)
 * \li Several new student records are added to the database
 * \li Several new course records are added to the database
 * \li Students are registered for various courses
 * \li A list of all the students and the courses they are registered for is
 *      displayed
 * \li A list of all the courses and the students thare are registered for
 *      them is displayed
 *
 * \par Table of Contents
 *
 * - \subpage hDB
 * - \subpage hPGMfunc
 *
 * For additional information, please refer to the product documentation at
 * http://docs.raima.com/.
 *
 * \page hDB Database Schema
 *
 * \par Database Schema Definition
 *
 * The DDL (Database Definition Language) specification for the database used
 * in this example is located in the file \c core07.sdl.
 *
 * \include core07.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s option
 * to generate C-structures for interfacing with the database.
 *
 * \code rdm-compile -s core07.sdl \endcode
 *
 * Each of these functions returns an integer status code, where the
 * value sOKAY indicates a successful call.
 *
 * The actual database will be stored in a directory named 'core06.rdm' in the
 * project directory.
 *
 * \page hPGMfunc Program Functions
 *
 * For simplicity, this example does not check all return codes, but good
 * programming practices would dictate that they are checked after each
 * RDM call.
 *
 * \li register_for_course() - \copybrief register_for_course
 * \li main() - \copybrief main
 */

/* \brief Create a few students in the database
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertStudents (RDM_DB hDB)
{
    RDM_RETCODE rc;
    STUDENT student_rec;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        /* Create a few student records */
        strcpy (student_rec.name, "Jeff");
        rc = rdm_dbInsertRow (
            hDB, TABLE_STUDENT, &student_rec, sizeof (student_rec), NULL);
        print_error (rc);

        if (rc == sOKAY)
        {
            strcpy (student_rec.name, "Brooke");
            rc = rdm_dbInsertRow (
                hDB, TABLE_STUDENT, &student_rec, sizeof (student_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (student_rec.name, "Jonah");
            rc = rdm_dbInsertRow (
                hDB, TABLE_STUDENT, &student_rec, sizeof (student_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (student_rec.name, "Norah");
            rc = rdm_dbInsertRow (
                hDB, TABLE_STUDENT, &student_rec, sizeof (student_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (student_rec.name, "Micah");
            rc = rdm_dbInsertRow (
                hDB, TABLE_STUDENT, &student_rec, sizeof (student_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }
    return rc;
}

/* \brief Create a few classes in the database
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertClasses (RDM_DB hDB)
{
    RDM_RETCODE rc;
    CLASS course_rec;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        /* Create a few classes */
        if (rc == sOKAY)
        {
            strcpy (course_rec.id, "ACCTG1A");
            strcpy (course_rec.title, "Principles of Accounting");
            rc = rdm_dbInsertRow (
                hDB, TABLE_CLASS, &course_rec, sizeof (course_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (course_rec.id, "MATH037");
            strcpy (course_rec.title, "Finite Mathematics");
            rc = rdm_dbInsertRow (
                hDB, TABLE_CLASS, &course_rec, sizeof (course_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (course_rec.id, "CAOTO15");
            strcpy (course_rec.title, "Business Communications");
            rc = rdm_dbInsertRow (
                hDB, TABLE_CLASS, &course_rec, sizeof (course_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (course_rec.id, "CBIS36");
            strcpy (course_rec.title, "Systems Analysis and Design");
            rc = rdm_dbInsertRow (
                hDB, TABLE_CLASS, &course_rec, sizeof (course_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            strcpy (course_rec.id, "IBUS1");
            strcpy (course_rec.title, "Introduction to International Business");
            rc = rdm_dbInsertRow (
                hDB, TABLE_CLASS, &course_rec, sizeof (course_rec), NULL);
            print_error (rc);
        }

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }
    return rc;
}

/* \brief Register the student for a class
 */
RDM_RETCODE register_for_course (
    const char *student_name, /*< [in] Student name */
    const char *course_id,    /*< [in] Class id */
    RDM_DB hDB)               /*< [in] Open database handle */
{
    RDM_RETCODE rc;
    RDM_CURSOR studentCursor = NULL;
    RDM_CURSOR classCursor = NULL;
    RDM_CURSOR enrollmentCursor = NULL;
    CLASS_ID_KEY classKey;
    STUDENT_NAME_KEY studentKey;
    ENROLLMENT enroll_rec;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        /* Find the student record and setup a cursor pointing to it */
        strcpy (studentKey.name, student_name);
        rc = rdm_dbGetRowsByKeyAtKey (
            hDB, KEY_STUDENT_NAME, &studentKey, sizeof (studentKey),
            &studentCursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            /* Find the course record and setup a cursor pointing to it */
            strcpy (classKey.id, course_id);
            rc = rdm_dbGetRowsByKeyAtKey (
                hDB, KEY_CLASS_ID, &classKey, sizeof (classKey), &classCursor);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            /* Create the enrollment record */
            /* Set the enrollment date to today and mark the column that it
             * contains a value */
            rdm_dateToday (0, &enroll_rec.begin_date);
            enroll_rec._begin_date_has_value = true;

            /* Set the status to enrolled and mark the column that it contains a
             * value
             */
            strcpy (enroll_rec.status, "enrolled");
            enroll_rec._status_has_value = true;

            /* mark the rest of the data columns empty */
            enroll_rec._end_date_has_value = false;
            enroll_rec._current_grade_has_value = false;

            /* mark the set relationships as not-linked */
            enroll_rec._class_students_has_value = false;
            enroll_rec._student_classes_has_value = false;

            /* create a new course_registration record */
            rc = rdm_dbInsertRow (
                hDB, TABLE_ENROLLMENT, &enroll_rec, sizeof (enroll_rec),
                &enrollmentCursor);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            /* Associate the student and the course via the course_registartion
             * sets */
            rc = rdm_cursorLinkRow (
                enrollmentCursor, REF_ENROLLMENT_CLASS_STUDENTS, classCursor);
            print_error (rc);
        }
        if (rc == sOKAY)
        {
            rc = rdm_cursorLinkRow (
                enrollmentCursor, REF_ENROLLMENT_STUDENT_CLASSES,
                studentCursor);
            print_error (rc);
        }

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }
    return rc;
}

RDM_RETCODE registerForClasses (RDM_DB hDB)
{
    RDM_RETCODE rc;

    rc = register_for_course ("Jeff", "IBUS1", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Jeff", "CAOTO15", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Jeff", "ACCTG1A", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Brooke", "CBIS36", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Brooke", "IBUS1", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Jonah", "MATH037", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Jonah", "CBIS36", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Norah", "IBUS1", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Norah", "ACCTG1A", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Norah", "MATH037", hDB);
    if (rc == sOKAY)
        rc = register_for_course ("Micah", "ACCTG1A", hDB);

    return rc;
}

RDM_RETCODE displayClassRoster (RDM_DB hDB)
{
    RDM_RETCODE rc;
    STUDENT student_rec;
    CLASS course_rec;

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        RDM_CURSOR studentCursor = NULL;
        RDM_CURSOR classCursor = NULL;
        RDM_CURSOR enrollmentCursor = NULL;

        printf ("\nList of courses each student is registered for\n");

        /* Read each student name from the STUDENT table */
        rc = rdm_dbGetRows (hDB, TABLE_STUDENT, &studentCursor);
        print_error (rc);
        for (rc = rdm_cursorMoveToFirst (studentCursor); rc == sOKAY;
             rc = rdm_cursorMoveToNext (studentCursor))
        {
            rc = rdm_cursorReadRow (
                studentCursor, &student_rec, sizeof (student_rec), NULL);
            print_error (rc);
            printf ("%s\n", student_rec.name);

            /* For each enrollment record associated with the STUDENT */
            rc = rdm_cursorGetMemberRows (
                studentCursor, REF_ENROLLMENT_STUDENT_CLASSES,
                &enrollmentCursor);
            print_error (rc);
            for (rc = rdm_cursorMoveToFirst (enrollmentCursor); rc == sOKAY;
                 rc = rdm_cursorMoveToNext (enrollmentCursor))
            {
                ENROLLMENT enroll_rec;
                char begin_date[100];

                rc = rdm_cursorReadRow (
                    enrollmentCursor, &enroll_rec, sizeof (enroll_rec), NULL);
                print_error (rc);
                rc = rdm_dateToString (
                    enroll_rec.begin_date, RDM_YYYYMMDD, '-', begin_date,
                    sizeof (begin_date), NULL);
                print_error (rc);

                /* Find the COURSE name associated with the enrollment record
                 * read */
                rc = rdm_cursorGetOwnerRow (
                    enrollmentCursor, REF_ENROLLMENT_CLASS_STUDENTS,
                    &classCursor);
                rc = rdm_cursorReadRow (
                    classCursor, &course_rec, sizeof (course_rec), NULL);
                print_error (rc);
                printf (
                    "\t%s: %s\t%s\n", begin_date, course_rec.id,
                    course_rec.title);
            }
            /* a successful navigation of the cursor list for ENROLLMENT will
             * return sENDOFCURSOR */
            if (rc == sENDOFCURSOR)
                rc = sOKAY;
        }
        /* a successful navigation of the cursor list for STUDENT will return
         * sENDOFCURSOR */
        if (rc == sENDOFCURSOR)
            rc = sOKAY;

        /* free the cursor resources that were allocated */
        if (classCursor)
            rdm_cursorFree (classCursor);
        if (studentCursor)
            rdm_cursorFree (studentCursor);
        if (enrollmentCursor)
            rdm_cursorFree (enrollmentCursor);

        rdm_dbEnd (hDB);
    }
    return rc;
}

/* \brief Main function for core07 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core07 (int32_t argc, const char *const *argv)
{
    RDM_TFS hTFS;
    RDM_DB hDB;
    RDM_RETCODE rc;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);
    if (rc == sOKAY)
    {
        rc = exampleOpenEmptyDatabase (&hTFS, &hDB, "core07", core07_cat);
        if (rc == sOKAY)
        {
            rc = insertStudents (hDB);
            if (rc == sOKAY)
            {
                rc = insertClasses (hDB);
            }
            if (rc == sOKAY)
            {
                rc = registerForClasses (hDB);
            }
            if (rc == sOKAY)
            {
                rc = displayClassRoster (hDB);
            }
            exampleCleanup (hTFS, hDB);
        }
    }

    return (int) rc;
}

RDM_STARTUP_EXAMPLE (core07)
