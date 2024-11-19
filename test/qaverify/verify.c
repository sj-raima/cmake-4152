#include "../../source/psp/features_types.h"

#if defined(RDM_LINUX)
#include "psptypes.h"
#include "verify_types.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <dlfcn.h>

typedef enum
{
    QA_VERIFY_OK = 0X0,
    QA_VERIFY_FILE = 0X1,
    QA_VERIFY_DIR = 0X2,
    QA_VERIFY_DIR_NOT_EMPTY = 0X4
} QA_VERIFY_FILE_TYPE;

#define QA_VERIFY_NAME_MAX 100
#define NAME_ENTRIES 8000

typedef struct
{
    char name[QA_VERIFY_NAME_MAX];
    QA_VERIFY_FILE_TYPE type;
} QA_VERIFY_FILE_ENTRY;

QA_VERIFY_FILE_ENTRY qa_verify_file_entries[NAME_ENTRIES];
int qa_verify_num_file_entries;
QA_VERIFY_STATE qa_verify_state = QA_VERIFY_NO;

static int verbose;

char cwd_path[1024];
int cwd_path_length;

const char *qa_verify_file;
int qa_verify_line;
const char *qa_verify_cleanup_file;
int qa_verify_cleanup_line;

bool qa_verifyHeader (const char *file, int line, const char *test_name)
{
    int base_length = strlen (test_name);
    if (base_length > 100)
    {
        fprintf (stderr, "\n%s:%d: ERROR: Test name too long (%s)", file, line, test_name);

        return false;
    }
    if (test_name[base_length - 1] >= '0' && test_name[base_length - 1] <= '9')
    {
        while (test_name[base_length - 1] >= '0' && test_name[base_length - 1] <= '9')
        {
            base_length--;
        }
        if (test_name[base_length - 1] == '_')
        {
            base_length--;
        }
        else
        {
            base_length = strlen (test_name);
        }
    }

    if (base_length >= 5)
    {
        base_length -= 5;
    }

    if (strncmp (test_name + base_length, "_test", 5) != 0)
    {
        fprintf (stderr, "\n%s:%d: ERROR: Test name does not end in _test (%s)", file, line, test_name);

        return false;
    }
    else
    {
        char base_name[100];
        strcpy (base_name, test_name);
        base_name[base_length] = '\0';
        fprintf (stderr, "\n%s:%d: ERROR: Test need cleanup (or destroy)", file, line);
        if (qa_verify_cleanup_file)
        {
            fprintf (stderr, "\n%s:%d: INFO: Add code like this:", qa_verify_cleanup_file, qa_verify_cleanup_line);
        }
        else
        {
            fprintf (stderr, "\nINFO: Add code like this:\nstatic void %s_cleanup (QA_CASE_PARAM)\n{", base_name);
        }

        return true;
    }
}

void qa_verifyEnd (const char *file, int line, const char *test_name)
{
    int i;
    bool found = false;

    for (i = 0; i < qa_verify_num_file_entries; i++)
    {
        if (qa_verify_file_entries[i].type & QA_VERIFY_FILE)
        {
            if (found == false)
            {
                if (qa_verifyHeader (file, line, test_name) == false)
                {
                    return;
                }
                found = true;
            }
            fprintf (stderr, "\npsp_fileRemove (QA_SPRINT (\"%%N%s\"));", qa_verify_file_entries[i].name);
        }
        if (qa_verify_file_entries[i].type & (QA_VERIFY_DIR | QA_VERIFY_DIR_NOT_EMPTY))
        {
            if (found == false)
            {
                if (qa_verifyHeader (file, line, test_name) == false)
                {
                    return;
                }
                found = true;
            }
            if (strcmp (qa_verify_file_entries[i].name + strlen (qa_verify_file_entries[i].name) - 4, ".rdm") == 0)
            {
                fprintf (stderr, "\n    (void) rdm_tfsDropDatabase (QA_SPRINT (\"%%U%%N%s\"));",
                         qa_verify_file_entries[i].name);
            }
            else
            {
                fprintf (stderr, "\n    (void) psp_fileDirRemove (QA_SPRINT (\"%%N%s\"));",
                         qa_verify_file_entries[i].name);
            }
            if (qa_verify_file_entries[i].type & QA_VERIFY_DIR_NOT_EMPTY)
            {
                fprintf (stderr, " // Directory not empty");
            }
        }
        if (qa_verify_file_entries[i].type & QA_VERIFY_DIR_NOT_EMPTY)
        {
            fprintf (stderr, "\n%s:%d: ERROR: Test name too long (%s)", file, line, test_name);
        }
    }

    if (found == true && !qa_verify_cleanup_file)
    {
        fprintf (stderr, "\n}\n...\n    QA_NEW_CLEANUP (%s_cleanup);", test_name);
    }
    qa_verify_num_file_entries = 0;
}

const char *qa_verifyStateToString (QA_VERIFY_STATE state)
{
    const char *str;
    switch (state)
    {
        case QA_VERIFY_NO:
            str = "QA_VERIFY_NO";
            break;
        case QA_VERIFY_SETUP:
            str = "QA_VERIFY_SETUP";
            break;
        case QA_VERIFY_UNSETUP:
            str = "QA_VERIFY_UNSETUP";
            break;
        case QA_VERIFY_TEST:
            str = "QA_VERIFY_TEST";
            break;
        case QA_VERIFY_CLEANUP:
            str = "QA_VERIFY_CLEANUP";
            break;
        case QA_VERIFY_END:
            str = "QA_VERIFY_END";
            break;
    }

    return str;
}

void qa_verifyCase (const char *file, int line)
{
    qa_verify_file = file;
    qa_verify_line = line;
    if (qa_verify_state == QA_VERIFY_CLEANUP)
    {
        qa_verify_cleanup_file = file;
        qa_verify_cleanup_line = line;
    }
}

void qa_verifyStateChange (QA_VERIFY_STATE state, const char *file, int line, const char *test_name)
{
    qa_verify_file = file;
    qa_verify_line = line;
    if (state == QA_VERIFY_SETUP)
    {
        qa_verify_cleanup_file = NULL;
        qa_verify_cleanup_line = 0;
    }
    if (verbose)
    {
        fprintf (stderr, "\nState changed to %s", qa_verifyStateToString (state));
    }

    if (state == QA_VERIFY_END)
    {
        qa_verifyEnd (file, line, test_name);
        qa_verify_state = QA_VERIFY_NO;
    }
    else
    {
        qa_verify_state = state;
    }
}

const char *qa_verifyProcessPath (const char *path)
{
    const char *pathFromCWD;
    if (path[0] != '/')
    {
        pathFromCWD = path;
    }
    else if (strncmp (cwd_path, path, cwd_path_length) == 0)
    {
        pathFromCWD = path + cwd_path_length + 1;
    }
    else
    {
        fprintf (stderr, "\n%s: ERROR: Path outside CWD", path);
        return NULL;
    }

    if (strchr (pathFromCWD, '/') == NULL)
    {
        return pathFromCWD;
    }

    return NULL;
}

void qa_verifyCreate (const char *path, QA_VERIFY_FILE_TYPE file_type)
{
    if (qa_verify_state == QA_VERIFY_TEST || qa_verify_state == QA_VERIFY_SETUP || qa_verify_state == QA_VERIFY_UNSETUP)
    {
        int i;
        const char *name = qa_verifyProcessPath (path);

        if (name)
        {
            if (qa_verify_state == QA_VERIFY_TEST)
            {
                for (i = 0; i < qa_verify_num_file_entries; i++)
                {
                    if (strcmp (name, qa_verify_file_entries[i].name) == 0)
                    {
                        qa_verify_file_entries[i].type |= file_type;
                        return;
                    }
                }
                if (verbose)
                {
                    fprintf (stderr, "\nAdded %s", name);
                }
                if (qa_verify_num_file_entries < NAME_ENTRIES)
                {
                    if (strlen (name) < QA_VERIFY_NAME_MAX)
                    {
                        strcpy (qa_verify_file_entries[qa_verify_num_file_entries].name, name);
                        qa_verify_file_entries[qa_verify_num_file_entries].type = file_type;
                        qa_verify_num_file_entries++;
                    }
                    else
                    {
                        fprintf (stderr, "\n%s:%d: ERROR: File name too long to handle (%s)", qa_verify_file,
                                 qa_verify_line, name);
                    }
                }
                else
                {
                    fprintf (stderr, "\n%s:%d: ERROR: Too many file names to handle (%s)", qa_verify_file,
                             qa_verify_line, name);
                }
            }
            else
            {
                fprintf (stderr, "\n%s:%d: ERROR: File operations not allowed in a (un)setup case (%s)", qa_verify_file,
                         qa_verify_line, name);
            }
        }
    }
}

void qa_verifyDelete (const char *path, QA_VERIFY_FILE_TYPE file_type)
{
    if (qa_verify_state == QA_VERIFY_CLEANUP || qa_verify_state == QA_VERIFY_SETUP ||
        qa_verify_state == QA_VERIFY_UNSETUP)
    {
        const char *name = qa_verifyProcessPath (path);

        if (name)
        {
            if (qa_verify_state == QA_VERIFY_CLEANUP)
            {
                int i;
                for (i = 0; i < qa_verify_num_file_entries; i++)
                {
                    if (strcmp (name, qa_verify_file_entries[i].name) == 0)
                    {
                        qa_verify_file_entries[i].type &= ~file_type;
                        if (verbose)
                        {
                            fprintf (stderr, "\nRemoved %s (%d)", name, qa_verify_file_entries[i].type);
                        }
                    }
                }
            }
            else
            {
                fprintf (stderr, "\n%s:%d: ERROR: File operations not allowed in a (un)setup case (%s)", qa_verify_file,
                         qa_verify_line, name);
            }
        }
    }
}

void qa_verifyStat (const char *path)
{
#if 1
    RDM_UNREF (path);
    /* Stating a file or a directory could be done to determine wheter
     * anything needs to be cleaned up */
    return;
#else
    if (qa_verify_state == QA_VERIFY_CLEANUP)
    {
        const char *name = qa_verifyProcessPath (path);

        if (name)
        {
            int i;
            for (i = 0; i < qa_verify_num_file_entries; i++)
            {
                if (strcmp (name, qa_verify_file_entries[i].name) == 0)
                {
                    qa_verify_file_entries[i].type = 0;
                }
            }
        }
    }
#endif
}

static void help (void)
{
    printf ("\n"
            "Usage:\n"
            "\n"
            "    LD_PRELOAD=.../librdmqaverify-14.so QA_VERIFY=\"[OPTION]...\" program\n"
            "    LD_LIBRARY_PATH=... LD_PRELOAD=librdmqaverify-14.so QA_VERIFY=\"[OPTION]...\" program\n"
            "\n"
            "Preload this library to get information about proper cleanup from QA Framework tests.\n"
            "\n"
            "  -h, --help        Print out this help text\n"
            "  -v, --verbose     Print out every intercepted call\n"
            "\n");
}

static int parse_option (const char **pArgs, const char *option)
{
    int len = strlen (option);
    int found = 0;

    if (strncmp (*pArgs, option, len) == 0 && ((*pArgs)[len] == '\0' || (*pArgs)[len] == ' '))
    {
        found = 1;
        (*pArgs) += len;

        while (**pArgs == ' ')
        {
            (*pArgs)++;
        }
    }

    return found;
}

static void parse_options (void)
{
    const char *args = getenv ("QA_VERIFY");

    if (args)
    {
        while (*args)
        {
            if (parse_option (&args, "-h") || parse_option (&args, "--help"))
            {
                help ();
            }
            else if (parse_option (&args, "-v") || parse_option (&args, "--verbose"))
            {
                verbose = 1;
            }
        }
    }
}

void qa_verify_setup (void)
{
    static int setup;
    if (!setup)
    {
        setup = 1;
        (void) getcwd (cwd_path, sizeof (cwd_path));
        cwd_path_length = strlen (cwd_path);
        parse_options ();
    }
}

int open64 (const char *pathname, int oflag, ...)
{
    int fd;
    va_list args;
    mode_t mode;
    static ssize_t (*open_real) (const char *pathname, int oflag, mode_t mode);

    va_start (args, oflag);

    if (!open_real)
    {
#ifdef RDM_64BIT
        open_real = (ssize_t (*) (const char *pathname, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "open64");
#else
        open_real = (ssize_t (*) (const char *pathname, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "open");
#endif
        qa_verify_setup ();
    }

    mode = va_arg (args, mode_t);

    if (verbose)
    {
        fprintf (stderr, "\nopen %s", pathname);
    }

    qa_verifyCreate (pathname, QA_VERIFY_FILE);

    fd = open_real (pathname, oflag, mode);

    va_end (args);

    return fd;
}

int mkdir (const char *pathname, mode_t mode)
{
    int ret;
    static int (*mkdir_real) (const char *pathname, mode_t mode);

    if (!mkdir_real)
    {
        mkdir_real = (int (*) (const char *pathname, mode_t mode)) dlsym (RTLD_NEXT, "mkdir");
        qa_verify_setup ();
    }

    if (verbose)
    {
        fprintf (stderr, "\nmkdir %s", pathname);
    }

    qa_verifyCreate (pathname, QA_VERIFY_DIR);

    ret = mkdir_real (pathname, mode);

    return ret;
}

int stat (const char *pathname, struct stat *statbuf)
{
    int ret;
    static int (*stat_real) (const char *pathname, struct stat *statbuf);

    if (!stat_real)
    {
        stat_real = (int (*) (const char *pathname, struct stat *statbuf)) dlsym (RTLD_NEXT, "stat");
        qa_verify_setup ();
    }

    if (verbose)
    {
        fprintf (stderr, "\nstat %s", pathname);
    }

    qa_verifyStat (pathname);
    ret = stat_real (pathname, statbuf);

    return ret;
}

int unlink (const char *pathname)
{
    int ret;
    static int (*unlink_real) (const char *pathname);

    if (!unlink_real)
    {
        unlink_real = (int (*) (const char *pathname)) dlsym (RTLD_NEXT, "unlink");
        qa_verify_setup ();
    }

    if (verbose)
    {
        fprintf (stderr, "\nunlink %s", pathname);
    }

    qa_verifyDelete (pathname, QA_VERIFY_FILE);
    ret = unlink_real (pathname);

    return ret;
}

int rmdir (const char *pathname)
{
    int ret;
    static int (*rmdir_real) (const char *pathname);

    if (!rmdir_real)
    {
        rmdir_real = (int (*) (const char *pathname)) dlsym (RTLD_NEXT, "rmdir");
        qa_verify_setup ();
    }

    if (verbose)
    {
        fprintf (stderr, "\nrmdir %s", pathname);
    }

    ret = rmdir_real (pathname);
    if (ret == ENOTEMPTY)
    {
        qa_verifyDelete (pathname, QA_VERIFY_DIR_NOT_EMPTY);
    }
    else
    {
        qa_verifyDelete (pathname, QA_VERIFY_DIR);
    }

    return ret;
}

#endif
