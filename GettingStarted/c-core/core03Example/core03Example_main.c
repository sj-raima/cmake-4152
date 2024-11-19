#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core03Example_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core03Example_cat.h"

RDM_CMDLINE cmd;
const char *const description =
    "Demonstrates associating rows in one table with a row in another table";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core03 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (all existing contents removed)
 * \li  Several new artists are added to the database
 * \li  For each artist a couple albums are added to the database
 * \li  A list of each artist and their albums are displayed
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
 * in this example is located in the file \c core03.sdl.
 *
 * \include core03.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with options
 * to generate C-structures and catalog files for interfacing with the database.
 * The option to generate lowercase struct member names was also used
 *
 * \code rdm-compile --c-structs --catalog --lc-struct-members core03.sdl
 * \endcode
 *
 * \page hPGMfunc Program Functions
 * \li insertArtist() - \copybrief insertArtist
 * \li readAllAlbums() - \copybrief readAllAlbums
 * \li main() - \copybrief main
 */

/* \brief Insert an artist and all of the albums associated with the artist
 *
 * This function adds one more row to the ARTIST table in the core03 database
 * and one to many rows to the ALBUM table associated with the given artist.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertArtist (
    RDM_DB hDB,               /*< [in] RDM db handle with the database open */
    const char *artistName,   /*< [in] Artist name to insert */
    RDM_CURSOR *artistCursor) /*< [out] Cursor position of artist just added */
{
    RDM_RETCODE rc;
    ARTIST artist_rec;

    artist_rec.id = 0;
    strncpy (artist_rec.name, artistName, sizeof (artist_rec.name));
    rc = rdm_dbInsertRow (
        hDB, TABLE_ARTIST, &artist_rec, sizeof (artist_rec), artistCursor);
    print_error (rc);
    return rc;
}

/* \brief Insert an albums and associated with the artist
 *
 * This function adds new ALBUM rows to the database and associates (links) them
 * with the specific ARTIST using the cursor position provided.
 *
 * \note Notice in function that we do not explicitly allocate the cursor passed
 * to the rdm_dbInsertRow() function. A "shortcut" for cursor association
 * functions is to pass a cursor set to NULL to the function and it will be
 * allocated for you. You still free the cursor object when finished to release
 * the resources associated with the cursor.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertAlbums (
    RDM_DB hDB,              /*< [in] RDM db handle with the database open */
    RDM_CURSOR artistCursor, /*< [in] Cursor position of artist just added */
    const char **albumList,  /*< [in] List of album names for the artist */
    size_t listSize)         /*< [in] Number of album names in the list */
{
    RDM_RETCODE rc = sOKAY;
    int ii;

    for (ii = 0; ii < (int) listSize; ii++)
    {
        ALBUM album_rec;
        RDM_CURSOR albumCursor = NULL;

        strncpy (album_rec.title, albumList[ii], sizeof (album_rec.title));
        album_rec._id_has_value = false; /* Mark the row as unlinked */
        rc = rdm_dbInsertRow (
            hDB, TABLE_ALBUM, &album_rec, sizeof (album_rec), &albumCursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            /* associate the added row to the artist */
            rc = rdm_cursorLinkRow (albumCursor, REF_ALBUM_ID, artistCursor);
            print_error (rc);

            rdm_cursorFree (albumCursor);
        }
    }
    return rc;
}

/* \brief Reads and displays all rows in the core03 database
 *
 * This function reads and displays each row from the ARTIST table in the core03
 * database and the ALBUMS associated with the artist.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE readAllAlbums (
    RDM_DB hDB) /*< [in] RDM db handle with the database open */
{
    RDM_RETCODE rc;
    RDM_CURSOR artistCursor = NULL;
    RDM_CURSOR albumCursor = NULL;

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);

    if (rc == sOKAY)
    {
        /* read the ARTIST table in table order */
        rc = rdm_dbGetRows (hDB, TABLE_ARTIST, &artistCursor);
        print_error (rc);
    }

    if (rc == sOKAY)
    {
        for (rc = rdm_cursorMoveToFirst (artistCursor); rc == sOKAY;
             rc = rdm_cursorMoveToNext (artistCursor))
        {
            char Artist[RDM_COLUMN_SIZE (ARTIST, name)];

            /* We found an artist row, read the contents */
            rc = rdm_cursorReadColumn (
                artistCursor, COL_ARTIST_NAME, Artist, sizeof (Artist), NULL);
            print_error (rc);

            if (rc == sOKAY)
            {
                printf ("\nArtist: %s\n", Artist);
                /* read the album rows associated with the artist*/
                rc = rdm_cursorGetMemberRows (
                    artistCursor, REF_ALBUM_ID, &albumCursor);
                print_error (rc);

                if (rc == sOKAY)
                {
                    printf ("\nAlbums:");
                    for (rc = rdm_cursorMoveToFirst (albumCursor); rc == sOKAY;
                         rc = rdm_cursorMoveToNext (albumCursor))
                    {
                        char Title[RDM_COLUMN_SIZE (ALBUM, title)];

                        /* We found an album row, read the contents*/
                        rc = rdm_cursorReadColumn (
                            albumCursor, COL_ALBUM_TITLE, Title, sizeof (Title),
                            NULL);
                        print_error (rc);

                        if (rc == sOKAY)
                        {
                            printf ("\t%s\n", Title);
                        }
                    }
                    /* We expect rc to be sENDOFCURSOR when we break out of the
                     * loop */
                    if (rc == sENDOFCURSOR)
                    {
                        rc = sOKAY; /* change status to sOKAY because
                                        sENDOFCURSOR was expected. */
                    }
                    else
                    {
                        print_error (rc);
                    }
                }
            }
        }
        /* We expect rc to be sENDOFCURSOR when we break out of the loop */
        if (rc == sENDOFCURSOR)
        {
            rc = sOKAY; /* change status to sOKAY because sENDOFCURSOR was
                            expected. */
        }
        else
        {
            print_error (rc);
        }
        if (albumCursor)
            rdm_cursorFree (albumCursor);
        if (artistCursor)
            rdm_cursorFree (artistCursor);

        rdm_dbEnd (hDB);
    }
    return rc;
}

/* List of albums for The Doors */
static const char *doors_albums[] = {"The Doors", "Strange Days",
                                     "Waiting for the Sun"};

/* List of albums for The Rolling Stones */
static const char *stones_albums[] = {"The Rolling Stones", "Out of Our Heads",
                                      "Beggars Banquet", "Tattoo You"};

/* List of albums for Nirvana */
static const char *nirvana_albums[] = {"Bleach", "Nevermind", "In Utero"};

/* \brief Insert an artist and albums and associated with the artist
 *
 * This function adds new ARTIST rows to the database and then adds ALBUMS using
 * the cursor position returned.
 *
 * \note If the insert of the ARTIST or their associated ALBUMS fails, the
 * transaction is rolled back.
 *
 * @return Returns an \c RDM_RETCODE code (\b sOKAY if successful)
 */
RDM_RETCODE insertAllArtists (RDM_DB hDB)
{
    RDM_RETCODE rc;
    RDM_CURSOR artistCursor = NULL;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        rc = insertArtist (hDB, "The Doors", &artistCursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            rc = insertAlbums (
                hDB, artistCursor, doors_albums,
                (sizeof (doors_albums) / sizeof (char *)));
        }

        /* if insert of both Artist and Album are ok,
         * commit the transaction, otherwise roll it back */
        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);
    }

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        rc = insertArtist (hDB, "The Rolling Stones", &artistCursor);
        if (rc == sOKAY)
        {
            rc = insertAlbums (
                hDB, artistCursor, stones_albums,
                (sizeof (stones_albums) / sizeof (char *)));
        }

        /* if insert of both Artist and Album are ok,
         * commit the transaction, otherwise roll it back */
        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);
    }

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        rc = insertArtist (hDB, "Nirvana", &artistCursor);
        if (rc == sOKAY)
        {
            rc = insertAlbums (
                hDB, artistCursor, nirvana_albums,
                (sizeof (nirvana_albums) / sizeof (char *)));
        }

        /* if insert of both Artist and Album are ok,
         * commit the transaction, otherwise roll it back */
        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);
    }
    return rc;
}

/* \brief Main function for core03 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the \c RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core03 (int32_t argc, const char *const *argv)
{
    RDM_TFS hTFS;
    RDM_DB hDB;
    RDM_RETCODE rc;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);
    if (rc == sOKAY)
    {
        rc = exampleOpenEmptyDatabase (&hTFS, &hDB, "core03", core03Example_cat);
        if (rc == sOKAY)
        {
            rc = insertAllArtists (hDB);
            if (rc == sOKAY)
            {
                rc = readAllAlbums (hDB);
            }
            exampleCleanup (hTFS, hDB);
        }
    }

    return (int) rc;
}

RDM_STARTUP_EXAMPLE (core03)
