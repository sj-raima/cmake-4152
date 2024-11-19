#include <stdio.h>
#include <string.h>
#include "example_fcns.h"
#include "rdm.h"
#include "rdmstartupapi.h"

/* Generated \c struct and \c typedef definitions to be used with the RDM APIs
 */
#include "core10_structs.h"

/* Generated catalog definition to be used with the RDM rdm_dbSetCatalog() API
 */
#include "core10_cat.h"

RDM_CMDLINE cmd;
const char *const description = "Demonstrates using an index for ordering";
const RDM_CMDLINE_OPT opts[] = {{NULL, NULL, NULL, NULL}};

/*
 * \mainpage Core10 Popcorn Example
 *
 * Each time you run this example:
 * \li The database is initialized (all existing contents removed)
 * \li Several new artists are added to the database
 * \li For each artist a couple albums are added to the database
 * \li For each album a tracks are added to the database
 * \li A list of each artist, their albums, and the album's tracks is
 *      displayed
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
 * The DDL (Database Definition Language) specification for the database used
 * in this example is located in the file \c core10.sdl.
 *
 * \include core10.sdl
 *
 * The schema was compiled using the RDM rdm-compile utility with the -s option
 * to generate C-structures for interfacing with the database.
 *
 * \code rdm-compile -s core10.sdl \endcode
 *
 * Each of these functions returns an integer status code, where the
 * value sOKAY indicates a successful call.
 *
 * The actual database will be stored in a directory named 'core10.rdm' in the
 * project directory.
 *
 * \page hPGMfunc Program Functions
 *
 * For simplicity, this example does not check all return codes, but good
 * programming practices would dictate that they are checked after each
 * RDM call.
 *
 * \li insertArtist() - \copybrief insertArtist
 * \li insertAlbum() - \copybrief insertAlbum
 * \li main() - \copybrief main
 */

/* \brief Insert an artist
 *
 * This function adds one more row to the ARTIST table in the core03 database.
 *
 * @return Returns an RDM_RETCODE code (sOKAY if successful)
 */
RDM_RETCODE insertArtist (
    RDM_DB hDB,             /*< [in] Database handle to open database */
    const char *artistName) /*< [in] Artist name to insert */
{
    RDM_RETCODE rc;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        ARTIST artist_rec;

        artist_rec.artistid = 0;
        strncpy (artist_rec.name, artistName, sizeof (artist_rec.name));
        rc = rdm_dbInsertRow (
            hDB, TABLE_ARTIST, &artist_rec, sizeof (artist_rec), NULL);
        print_error (rc);

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }

    return rc;
}

/* \brief Insert an artist and all of the albums associated with the artist
 *
 * This function adds one more row to the ARTIST table in the core03 database
 * and one to many rows to the ALBUM table associated with the given artist.
 *
 * @return Returns an RDM_RETCODE code (sOKAY if successful)
 */
RDM_RETCODE insertAlbum (
    RDM_DB hDB,             /*< [in] Database handle to open database */
    const char *artistName, /*< [in] Name of the artist */
    const char *albumTitle, /*< [in] Artist name to insert */
    const char **trackList, /*< [in] List of album names for the artist */
    size_t listSize)        /*< [in] Number of album names in the list */
{
    RDM_RETCODE rc;
    ALBUM album_rec;
    RDM_CURSOR albumCursor = NULL;
    RDM_CURSOR artistCursor = NULL;
    ARTIST_NAME_KEY artistKey;

    rc = rdm_dbStartUpdate (hDB, RDM_LOCK_ALL, 0, NULL, 0, NULL);
    print_error (rc);
    if (rc == sOKAY)
    {
        strncpy (artistKey.name, artistName, sizeof (artistKey.name));
        rc = rdm_dbGetRowsByKeyAtKey (
            hDB, KEY_ARTIST_NAME, &artistKey, sizeof (artistKey),
            &artistCursor);
        print_error (rc);

        if (rc == sOKAY)
        {
            strncpy (album_rec.title, albumTitle, sizeof (album_rec.title));
            album_rec.albumid = 0;
            album_rec._artistid_has_value =
                false; /* Mark the row as unlinked */
            rc = rdm_dbInsertRow (
                hDB, TABLE_ALBUM, &album_rec, sizeof (album_rec), &albumCursor);
            print_error (rc);
        }

        if (rc == sOKAY)
        {
            /* associate the added row to the artist */
            rc = rdm_cursorLinkRow (
                albumCursor, REF_ALBUM_ARTISTID, artistCursor);
            print_error (rc);
        }
        if (rc == sOKAY)
        {
            int ii;

            for (ii = 0; ii < (int) listSize; ii++)
            {
                TRACK track_rec;
                RDM_CURSOR trackCursor = NULL;

                strncpy (
                    track_rec.title, trackList[ii], sizeof (track_rec.title));
                track_rec._albumid_has_value =
                    false; /* Mark the row as unlinked */
                rc = rdm_dbInsertRow (
                    hDB, TABLE_TRACK, &track_rec, sizeof (track_rec),
                    &trackCursor);
                print_error (rc);

                if (rc == sOKAY)
                {
                    /* associate the added row to the artist */
                    rc = rdm_cursorLinkRow (
                        trackCursor, REF_TRACK_ALBUMID, albumCursor);
                    print_error (rc);

                    rdm_cursorFree (trackCursor);
                }
            }
            if (albumCursor)
                rdm_cursorFree (albumCursor);
        }

        if (rc == sOKAY)
            rc = rdm_dbEnd (hDB);
        else
            rc = rdm_dbEndRollback (hDB);

        print_error (rc);
    }
    return rc;
}

RDM_RETCODE readInfoForTrack (RDM_DB hDB, const char *trackName)
{
    RDM_RETCODE rc;

    rc = rdm_dbStartRead (hDB, RDM_LOCK_ALL, 0, NULL);
    print_error (rc);

    if (rc == sOKAY)
    {
        RDM_CURSOR cursor = NULL;
        RDM_CURSOR artistCursor = NULL;
        RDM_CURSOR albumCursor = NULL;
        TRACK_TITLE_KEY trackKey;
        char albumTitle[RDM_COLUMN_SIZE (ALBUM, title)];
        char trackTitle[RDM_COLUMN_SIZE (TRACK, title)];
        char artistName[RDM_COLUMN_SIZE (ARTIST, name)];
        RDM_ROWID_T saveAlbumId;

        /* Find the request track using the track TITLE index */
        strncpy (trackKey.title, trackName, sizeof (trackKey.title));
        rc = rdm_dbGetRowsByKeyAtKey (
            hDB, KEY_TRACK_TITLE, &trackKey, sizeof (trackKey), &cursor);
        rc = rdm_cursorReadColumn (
            cursor, COL_TRACK_TITLE, trackTitle, RDM_COLUMN_SIZE (TRACK, title),
            NULL);
        printf ("\nInformation about the track \"%s\"\n", trackTitle);

        rc = rdm_cursorGetOwnerRow (cursor, REF_TRACK_ALBUMID, &albumCursor);
        rc = rdm_cursorReadColumn (
            albumCursor, COL_ALBUM_TITLE, albumTitle,
            RDM_COLUMN_SIZE (ALBUM, title), NULL);
        rc = rdm_cursorReadColumn (
            albumCursor, COL_ALBUM_ALBUMID, &saveAlbumId, sizeof (saveAlbumId),
            NULL);
        printf ("The track is from the album \"%s\"\n", albumTitle);

        rc = rdm_cursorGetOwnerRow (
            albumCursor, REF_ALBUM_ARTISTID, &artistCursor);
        rc = rdm_cursorReadColumn (
            artistCursor, COL_ARTIST_NAME, artistName,
            RDM_COLUMN_SIZE (ARTIST, name), NULL);
        printf ("The artist on the track is \"%s\"\n", artistName);

        printf ("\nHere are other albums by \"%s\"\n", artistName);
        rc =
            rdm_cursorGetSiblingRows (albumCursor, REF_ALBUM_ARTISTID, &cursor);
        for (rc = rdm_cursorMoveToFirst (cursor); rc == sOKAY;
             rc = rdm_cursorMoveToNext (cursor))
        {
            ALBUM album_rec;

            rc = rdm_cursorReadRow (
                cursor, &album_rec, sizeof (album_rec), NULL);
            if (album_rec.albumid == saveAlbumId)
                continue;
            printf ("\t%s\n", album_rec.title);
        }

        if (cursor)
            rdm_cursorFree (cursor);
        if (artistCursor)
            rdm_cursorFree (artistCursor);
        if (albumCursor)
            rdm_cursorFree (albumCursor);

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

        rdm_dbEnd (hDB);
    }

    return rc;
}

/* Track data to insert */
static const char *the_doors_tracks[] = {"Break on Through (To the Other Side)",
                                         "Soul Kitchen",
                                         "The Crystal Ship",
                                         "Twentieth Century Fox",
                                         "Alabama Song (Whisky Bar)",
                                         "Light My Fire",
                                         "Back Door Man",
                                         "I Looked at You",
                                         "End of the Night",
                                         "Take It as It Comes",
                                         "The End"};

static const char *strange_days_tracks[] = {"Strange Days",
                                            "You're Lost Little Girl",
                                            "Love Me Two Times",
                                            "Unhappy Girl",
                                            "Horse Latitudes",
                                            "Moonlight Drive",
                                            "People Are Strange",
                                            "My Eyes Have Seen You",
                                            "I Can't See Your Face in My Mind",
                                            "When the Music's Over"};

static const char *waiting_tracks[] = {
    "Hello, I Love You",    "Love Street",     "Not to Touch the Earth",
    "Summer's Almost Gone", "Wintertime Love", "The Unknown Soldier",
    "Spanish Caravan",      "My Wild Love",    "We Could Be So Good Together",
    "Yes, the River Knows", "Five to One"};

static const char *the_rolling_stones_tracks[] = {
    "Not Fade Away",
    "Route 66",
    "I Just Want to Make Love to You",
    "Honest I Do",
    "Now I've Got a Witness",
    "Little by Little",
    "I'm a King Bee",
    "Carol",
    "Tell Me",
    "Can I Get a Witness",
    "You Can Make It If You Try",
    "Walking the Dog"};

static const char *out_of_our_heads_tracks[] = {
    "She Said Yeah",
    "Mercy, Mercy",
    "Hitch Hike",
    "That's How Strong My Love Is",
    "Good Times",
    "Gotta Get Away",
    "Talkin' Bout You",
    "Cry to Me",
    "Oh, Baby (We Got a Good Thing Going)",
    "Heart of Stone",
    "The Under Assistant West Coast Promotion Man",
    "I'm Free"};

static const char *beggars_banquet_tracks[] = {
    "Sympathy for the Devil", "No Expectations", "Dear Doctor",
    "Parachute Woman",        "Jig-Saw Puzzle",  "Street Fighting Man",
    "Prodigal Son",           "Stray Cat Blues", "Factory Girl",
    "Salt of the Earth"};

static const char *tattoo_you_tracks[] = {"Start Me Up",
                                          "Hang Fire",
                                          "Slave",
                                          "Little T&A",
                                          "Black Limousine",
                                          "Neighbours",
                                          "Worried About You",
                                          "Tops",
                                          "Heaven"};

static const char *bleach_tracks[] = {
    "Blew",      "Floyd The Barber", "About a Girl",   "School",
    "Love Buzz", "Paper Cuts",       "Negative Creep", "Scoff",
    "Swap Meet", "Mr. Moustache",    "Sifting"};

static const char *nevermind_tracks[] = {"Smells Like Teen Spirit",
                                         "In Bloom",
                                         "Come as You Are",
                                         "Breed",
                                         "Lithium",
                                         "Polly",
                                         "Territorial Pissings",
                                         "Drain You",
                                         "Lounge Act",
                                         "Stay Away",
                                         "On a Plain",
                                         "Something in the Way",
                                         "Endless, Nameless"};

static const char *in_utero_tracks[] = {
    "Serve the Servants",
    "Scentless Apprentice",
    "Heart-Shaped Box",
    "Rape Me",
    "Frances Farmer Will Have Her Revenge on Seattle",
    "Dumb",
    "Very Ape",
    "Milk It",
    "Pennyroyal Tea",
    "Radio Friendly Unit Shifter",
    "tourette's",
    "All Apologies"};

/* \brief Macro to determine number of elements in an array of ptrs */
#define RLEN(x) (sizeof (x) / sizeof (x[0]))

/* \brief Insert all albums
 *
 * @return Returns an RDM_RETCODE code (sOKAY if successful)
 */
RDM_RETCODE insertAllAlbums (
    RDM_DB hDB) /*< [in] Database handle to open database */
{
    RDM_RETCODE rc;

    rc = insertAlbum (
        hDB, "The Doors", "The Doors", the_doors_tracks,
        RLEN (the_doors_tracks));

    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "The Doors", "Strange Days", strange_days_tracks,
            RLEN (strange_days_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "The Doors", "Waiting for the Sun", waiting_tracks,
            RLEN (waiting_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "The Rolling Stones", "The Rolling Stones",
            the_rolling_stones_tracks, RLEN (the_rolling_stones_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "The Rolling Stones", "Out of our Heads",
            out_of_our_heads_tracks, RLEN (out_of_our_heads_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "The Rolling Stones", "Beggars Banquest",
            beggars_banquet_tracks, RLEN (beggars_banquet_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "The Rolling Stones", "Tattoo You", tattoo_you_tracks,
            RLEN (tattoo_you_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "Nirvana", "Bleach", bleach_tracks, RLEN (bleach_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "Nirvana", "Nevermind", nevermind_tracks,
            RLEN (nevermind_tracks));
    }
    if (rc == sOKAY)
    {
        rc = insertAlbum (
            hDB, "Nirvana", "In Utero", in_utero_tracks,
            RLEN (in_utero_tracks));
    }

    return rc;
}

/* \brief Insert all artists
 *
 * @return Returns an RDM_RETCODE code (sOKAY if successful)
 */
RDM_RETCODE insertAllArtists (
    RDM_DB hDB) /*< [in] Database handle to open database */
{
    RDM_RETCODE rc;

    rc = insertArtist (hDB, "The Doors");

    if (rc == sOKAY)
    {
        rc = insertArtist (hDB, "The Rolling Stones");
    }
    if (rc == sOKAY)
    {
        rc = insertArtist (hDB, "Nirvana");
    }

    return rc;
}

/* \brief Main function for core10 example
 *
 * The function initializes the RDM environment and runs the create, read
 * operations.
 *
 * @return Returns the RDM_RETCODE on exit.
 */
RDM_EXPORT int32_t main_core10 (int32_t argc, const char *const *argv)
{
    RDM_TFS hTFS;
    RDM_DB hDB;
    RDM_RETCODE rc;

    rc = rdm_cmdlineInit (&cmd, argc, argv, description, opts);
    if (rc != sCMD_USAGE)
        print_error (rc);

    if (rc == sOKAY)
    {
        rc = exampleOpenEmptyDatabase (&hTFS, &hDB, "core10", core10_cat);
        if (rc == sOKAY)
        {
            rc = insertAllArtists (hDB);
        }
        if (rc == sOKAY)
        {
            rc = insertAllAlbums (hDB);
        }

        if (rc == sOKAY)
        {
            rc = readInfoForTrack (hDB, "Love Street");
        }

        if (rc == sOKAY)
        {
            rc = readInfoForTrack (hDB, "Carol");
        }

        exampleCleanup (hTFS, hDB);
    }
    return (int) rc;
}

RDM_STARTUP_EXAMPLE (core10)
