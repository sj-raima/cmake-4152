#include "../../source/psp/features_types.h"

#if defined(RDM_LINUX)
#include "psptypes.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>

#include <dlfcn.h>

static long long written_count = 0;

typedef struct
{
    char path[1024];
    long long count;
} file_range;

static file_range range[256];
static int verbose;
static int no_fsync;
static int do_fsync;
static int immediate;
static int exit_instead_of_return;
static int start_gdb;
static long long write_count = -1;

static void help (void)
{
    printf ("\n"
            "Usage:\n"
            "\n"
            "    LD_PRELOAD=.../librdmqawrite-14.so QA_WRITE=\"[OPTION]... [COUNT]\" program\n"
            "    LD_LIBRARY_PATH=... LD_PRELOAD=librdmqawrite-14.so QA_WRITE=\"[OPTION]... [COUNT]\" program\n"
            "\n"
            "Preload this library to get pwrite64 to fail and the process to exit\n"
            "at a certain byte written.\n"
            "\n"
            "  -h, --help        Print out this help text\n"
            "      --no-fsync    Do not profile fsync\n"
            "      --do-fsync    Do the actual fsync calls instead of ignoring them\n"
            "  -v, --verbose     Print out how many bytes we wrote for each file\n"
            "  -i, --immediate   Exit or return immediately after the requested bytes have been written\n"
            "  -x, --exit        Exit with a exit code of 1 instead of returning to the caller\n"
            "  --gdb             Start gdb when we fail\n"
            "  COUNT             The number of bytes written before it should fail\n"
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
    const char *args = getenv ("QA_WRITE");

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
            else if (parse_option (&args, "-i") || parse_option (&args, "--immediat"))
            {
                immediate = 1;
            }
            else if (parse_option (&args, "-x") || parse_option (&args, "--exit"))
            {
                exit_instead_of_return = 1;
            }
            else if (parse_option (&args, "--no-fsync"))
            {
                no_fsync = 1;
            }
            else if (parse_option (&args, "--do-fsync"))
            {
                do_fsync = 1;
            }
            else if (parse_option (&args, "--gdb"))
            {
                start_gdb = 1;
            }
            else if (*args)
            {
                sscanf (args, "%lld", &write_count);
                break;
            }
        }
    }
}

static void statistics (int fd)
{
    static int last_fd;
    if (fd != last_fd)
    {
        if (last_fd != -1 && verbose && range[last_fd].count != written_count)
        {
            fprintf (stderr, "qawrite: %s:  Write [%lld, %lld>\n", range[last_fd].path, range[last_fd].count,
                     written_count);
        }
        if (fd == -1)
        {
            last_fd = fd;
        }
        else
        {
            range[fd].count = written_count;
            last_fd = fd;
        }
    }
}

int open64 (const char *path, int oflag, ...)
{
    int fd;
    va_list argp;
    mode_t mode;
    static ssize_t (*open_real) (const char *path, int oflag, mode_t mode);

    va_start (argp, oflag);

    if (!open_real)
    {
        open_real = (ssize_t (*) (const char *path, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "open64");
        parse_options ();
    }

    mode = va_arg (argp, mode_t);

    fd = open_real (path, oflag, mode);

    if (fd >= 0 && fd < 256)
    {
        statistics (-1);
        if (verbose)
        {
            fprintf (stderr, "qawrite: %s:  Open == %d\n", path, fd);
        }
        strcpy (range[fd].path, path);
        range[fd].count = -9999;
    }
    va_end (argp);

    return fd;
}

int openat64 (int dirfd, const char *path, int oflag, ...)
{
    int fd;
    va_list argp;
    mode_t mode;
    static ssize_t (*openat_real) (int dirfd, const char *path, int oflag, mode_t mode);

    va_start (argp, oflag);

    if (!openat_real)
    {
        openat_real = (ssize_t (*) (int dirfd, const char *path, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "openat64");
        parse_options ();
    }

    mode = va_arg (argp, mode_t);

    fd = openat_real (dirfd, path, oflag, mode);

    if (fd >= 0 && fd < 256)
    {
        statistics (-1);
        if (verbose)
        {
            fprintf (stderr, "qawrite: %s:  Openat == %d\n", path, fd);
        }
        strcpy (range[fd].path, path);
        range[fd].count = -9999;
    }
    va_end (argp);

    return fd;
}

int close (int fd)
{
    int ret;
    static int (*close_real) (int fd);

    if (!close_real)
    {
        close_real = (int (*) (int fd)) dlsym (RTLD_NEXT, "close");
    }

    ret = close_real (fd);

    if (fd < 256)
    {
        statistics (-1);
        if (verbose)
        {
            fprintf (stderr, "qawrite: %s:  Close == %d\n", range[fd].path, fd);
        }
    }

    return ret;
}

static void unload_me (void)
{
    statistics (-1);
    printf ("\nqawrite: QA_WRITE=%lld\n", written_count);
}

static void statistics_fsync (int fd)
{
    if (range[fd].path[0])
    {
        if (no_fsync == 0)
        {
            statistics (-1);
            if (verbose)
            {
                fprintf (stderr, "qawrite: %s:  Fsync\n", range[fd].path);
            }
        }
    }
}

int fsync (int fd)
{
    static ssize_t (*fsync_real) (int fd);
    int ret;

    if (!fsync_real)
    {
        fsync_real = (ssize_t (*) (int fd)) dlsym (RTLD_NEXT, "fsync");
    }

    statistics_fsync (fd);

    if (do_fsync)
    {
        ret = fsync_real (fd);
    }
    else
    {
        ret = 0;
    }

    return ret;
}

int fdatasync (int fd)
{
    static ssize_t (*fdatasync_real) (int fd);
    int ret;

    if (!fdatasync_real)
    {
        fdatasync_real = (ssize_t (*) (int fd)) dlsym (RTLD_NEXT, "fdatasync");
    }

    statistics_fsync (fd);

    if (do_fsync)
    {
        ret = fdatasync_real (fd);
    }
    else
    {
        ret = 0;
    }

    return ret;
}

ssize_t pwrite64 (int fd, const void *buf, size_t count, off64_t offset)
{
    static ssize_t (*pwrite_real) (int fd, const void *buf, size_t count, off64_t offset);
    static void (*waitForGdb_real) (const char *progname);
    ssize_t written;

    if (!pwrite_real)
    {
        pwrite_real =
            (ssize_t (*) (int fd, const void *buf, size_t count, off64_t offset)) dlsym (RTLD_NEXT, "pwrite64");
        waitForGdb_real = (void (*) (const char *progname)) dlsym (RTLD_DEFAULT, "waitForGdb");
        atexit (unload_me);
    }

    statistics (fd);

    if (write_count < 0)
    {
        written = pwrite_real (fd, buf, count, offset);
        written_count += written;
    }
    else if (write_count > (long long) count || (write_count == (long long) count && immediate == 0))
    {
        write_count -= count;
        written = pwrite_real (fd, buf, count, offset);
        if (written > 0)
        {
            written_count += written;
        }
    }
    else
    {
        if (write_count > 0)
        {
            write_count = 0;
            written = pwrite_real (fd, buf, write_count, offset);
            if (written > 0)
            {
                written_count += written;
            }
        }
        else
        {
            written = 0;
        }
        if (exit_instead_of_return)
        {
            fflush (stdout);
            statistics (-1);

            fprintf (stderr, "\nqawrite: exiting abnormally at QA_WRITE=\"--exit %lld\"\n", written_count);

            if (waitForGdb_real)
            {
                waitForGdb_real (0);
            }
            _exit (1);
        }
        else
        {
            fprintf (stderr, "\nqawrite: return abnormally at QA_WRITE=\"%lld\"\n", written_count);
            if (written == 0)
            {
                errno = ENOSPC;
                written = -1;
            }
            if (waitForGdb_real && start_gdb)
            {
                raise (SIGUSR2);
            }
        }
    }

    return written;
}

ssize_t write (int fd, const void *buf, size_t count)
{
    static ssize_t (*write_real) (int fd, const void *buf, size_t count);
    static void (*waitForGdb_real) (const char *progname);
    ssize_t written;

    if (!write_real)
    {
        write_real = (ssize_t (*) (int fd, const void *buf, size_t count)) dlsym (RTLD_NEXT, "write");
        waitForGdb_real = (void (*) (const char *progname)) dlsym (RTLD_DEFAULT, "waitForGdb");
        atexit (unload_me);
    }

    statistics (fd);

    if (write_count < 0)
    {
        written = write_real (fd, buf, count);
        written_count += written;
    }
    else if (write_count > (long long) count || (write_count == (long long) count && immediate == 0))
    {
        write_count -= count;
        written = write_real (fd, buf, count);
        if (written > 0)
        {
            written_count += written;
        }
    }
    else
    {
        if (write_count > 0)
        {
            write_count = 0;
            written = write_real (fd, buf, write_count);
            if (written > 0)
            {
                written_count += written;
            }
        }
        else
        {
            written = 0;
        }
        if (exit_instead_of_return)
        {
            fflush (stdout);
            statistics (-1);

            fprintf (stderr, "\nqawrite: exiting abnormally at QA_WRITE=\"--exit %lld\"\n", written_count);

            if (waitForGdb_real)
            {
                waitForGdb_real (0);
            }
            _exit (1);
        }
        else
        {
            fprintf (stderr, "\nqawrite: return abnormally at QA_WRITE=\"%lld\"\n", written_count);
            if (written == 0)
            {
                errno = ENOSPC;
                written = -1;
            }
            if (waitForGdb_real && start_gdb)
            {
                raise (SIGUSR2);
            }
        }
    }

    return written;
}
#endif
