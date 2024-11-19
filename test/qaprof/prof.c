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
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>

typedef struct
{
    char path[1024];
    long long read_times;
    long long read_blocks;
    long long read_bytes;
    long long read_time;
    long long written_times;
    long long written_blocks;
    long long written_bytes;
    long long write_time;
    long long fsyncs;
    long long fsync_time;
} file_stats;

static int file_stats_max;
static file_stats all_stats[100000];
static file_stats *fd_stats[10000];

static int no_fsync;
static int no_time;
static int initialized;
static int full_path;

static void help (void)
{
    printf ("\n"
            "Usage:\n"
            "\n"
            "    LD_PRELOAD=.../librdmqaprof-14.so QA_PROF=\"[OPTION]...\" program\n"
            "    LD_LIBRARY_PATH=... LD_PRELOAD=librdmqaprof-14.so QA_PROF=\"[OPTION]...\" program\n"
            "\n"
            "Preload this library to get statistics for pread64, pwrite64, and fsync.\n"
            "\n"
            "  -h, --help        Print out this help text\n"
            "      --no-fsync    Don't do the actual fsync calls\n"
            "      --no-time     Don't trace the time\n"
            "      --path        Print the full path for files\n"
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
    const char *args = getenv ("QA_PROF");

    if (args)
    {
        while (*args)
        {
            if (parse_option (&args, "-h") || parse_option (&args, "--help"))
            {
                help ();
            }
            else if (parse_option (&args, "--no-fsync"))
            {
                no_fsync = 1;
            }
            else if (parse_option (&args, "--path"))
            {
                full_path = 1;
            }
            else if (parse_option (&args, "--no-time"))
            {
                no_time = 1;
            }
            else
            {
                fprintf (stderr, "Illegal option '%s', --help for help", args);
                break;
            }
        }
    }
}

static void unload_me (void)
{
    int i;
    long long read_times = 0;
    long long read_blocks = 0;
    long long read_bytes = 0;
    long long read_time = 0;
    long long written_times = 0;
    long long written_blocks = 0;
    long long written_bytes = 0;
    long long write_time = 0;
    long long fsyncs = 0;
    long long fsync_time = 0;

    fprintf (stderr, "Reads using pread64                      Time in | Writes using pwrite64                   Time "
                     "in | Syncs           Time in |\n");
    fprintf (stderr, "        #    Blocks         Bytes      micro sec |        #    Blocks         Bytes      micro "
                     "sec |        #      micro sec | Files\n");
    fprintf (stderr, "-------------------------------------------------+-----------------------------------------------"
                     "--+-------------------------+-----------\n");
    for (i = 0; i < file_stats_max; i++)
    {
        file_stats *stats = &all_stats[i];
        if (stats->path[0])
        {
            const char *lastSlash = strrchr (stats->path, '/');
            const char *path = (lastSlash && full_path == 0) ? lastSlash + 1 : stats->path;
            fprintf (stderr, "%9lld%10lld%14lld%15lld |%9lld%10lld%14lld%15lld |%9lld%15lld | %s\n", stats->read_times,
                     stats->read_blocks, stats->read_bytes, stats->read_time, stats->written_times,
                     stats->written_blocks, stats->written_bytes, stats->write_time, stats->fsyncs, stats->fsync_time,
                     path);
            read_times += stats->read_times;
            read_blocks += stats->read_blocks;
            read_bytes += stats->read_bytes;
            read_time += stats->read_time;
            written_times += stats->written_times;
            written_blocks += stats->written_blocks;
            written_bytes += stats->written_bytes;
            write_time += stats->write_time;
            fsyncs += stats->fsyncs;
            fsync_time += stats->fsync_time;
        }
    }
    fprintf (stderr, "%9lld%10lld%14lld%15lld |%9lld%10lld%14lld%15lld |%9lld%15lld | //sum\n", read_times, read_blocks,
             read_bytes, read_time, written_times, written_blocks, written_bytes, write_time, fsyncs, fsync_time);
}

static void setup (void)
{
    if (!initialized)
    {
        parse_options ();
        atexit (unload_me);
        initialized++;
    }
}

struct timespec timer_start ()
{
    struct timespec start_time = {0};
    if (no_time == 0)
    {
        clock_gettime (CLOCK_MONOTONIC, &start_time);
    }
    return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long long timer_end (struct timespec start_time)
{
    struct timespec end_time = {0};
    long long diffInMicro = 0;
    if (no_time == 0)
    {
        clock_gettime (CLOCK_MONOTONIC, &end_time);
    }
    if (end_time.tv_nsec > start_time.tv_nsec)
    {
        diffInMicro = ((long long) (end_time.tv_sec - start_time.tv_sec)) * 1000000LL +
                      ((long long) (end_time.tv_nsec - start_time.tv_nsec)) / 1000LL;
    }
    else
    {
        diffInMicro = ((long long) (end_time.tv_sec - start_time.tv_sec - 1)) * 1000000LL +
                      ((long long) (end_time.tv_nsec + 1000000000LL - start_time.tv_nsec)) / 1000LL;
    }
    return diffInMicro;
}

static file_stats *new_file (const char *path, int fd)
{
    file_stats *stats = NULL;
    if (fd >= 0)
    {
        stats = fd_stats[fd];
        if (stats == NULL)
        {
            char fd_name[20];
            int i = 0;

            if (path == NULL)
            {
                sprintf (fd_name, "//<fd %d>", fd);
                path = fd_name;
            }

            for (;;)
            {
                stats = &all_stats[i];
                if (i == file_stats_max)
                {
                    strcpy (stats->path, path);
                    file_stats_max++;
                }
                if (strcmp (stats->path, path) == 0)
                {
                    fd_stats[fd] = stats;
                    break;
                }
                i++;
            }
        }
    }

    return stats;
}

int open64 (const char *path, int oflag, ...)
{
    int fd;
    va_list args;
    mode_t mode;
    static int (*open_real) (const char *path, int oflag, mode_t mode);

    va_start (args, oflag);

    if (!open_real)
    {
        open_real = (int (*) (const char *path, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "open64");
        setup ();
    }

    mode = va_arg (args, mode_t);

    fd = open_real (path, oflag, mode);

    new_file (path, fd);

    va_end (args);

    return fd;
}

int open (const char *path, int oflag, ...)
{
    int fd;
    va_list args;
    mode_t mode;
    static int (*open_real) (const char *path, int oflag, mode_t mode);

    va_start (args, oflag);

    if (!open_real)
    {
        open_real = (int (*) (const char *path, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "open");
        setup ();
    }

    mode = va_arg (args, mode_t);

    fd = open_real (path, oflag, mode);

    new_file (path, fd);

    va_end (args);

    return fd;
}

int openat64 (int dirfd, const char *path, int oflag, ...)
{
    int fd;
    va_list args;
    mode_t mode;
    static int (*openat_real) (int dirfd, const char *path, int oflag, mode_t mode);

    va_start (args, oflag);

    if (!openat_real)
    {
        openat_real = (int (*) (int dirfd, const char *path, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "openat64");
        setup ();
    }
    fprintf (stderr, "********************* openat called ********************\n");
    mode = va_arg (args, mode_t);

    fd = openat_real (dirfd, path, oflag, mode);

    new_file (path, fd);

    va_end (args);

    return fd;
}

int openat (int dirfd, const char *path, int oflag, ...)
{
    int fd;
    va_list args;
    mode_t mode;
    static int (*openat_real) (int dirfd, const char *path, int oflag, mode_t mode);

    va_start (args, oflag);

    if (!openat_real)
    {
        openat_real = (int (*) (int dirfd, const char *path, int oflag, mode_t mode)) dlsym (RTLD_NEXT, "openat");
        setup ();
    }
    fprintf (stderr, "********************* openat called ********************\n");
    mode = va_arg (args, mode_t);

    fd = openat_real (dirfd, path, oflag, mode);

    new_file (path, fd);

    va_end (args);

    return fd;
}

int close (int fd)
{
    int ret;
    static int (*close_real) (int fd);

    if (!close_real)
    {
        close_real = (int (*) (int fd)) dlsym (RTLD_NEXT, "close");
        setup ();
    }

    ret = close_real (fd);

    if (ret == 0)
    {
        fd_stats[fd] = NULL;
    }

    return ret;
}

int fsync (int fd)
{
    file_stats *stats = new_file (NULL, fd);
    static ssize_t (*fsync_real) (int fd);
    int ret;

    if (!fsync_real)
    {
        fsync_real = (ssize_t (*) (int fd)) dlsym (RTLD_NEXT, "fsync");
        setup ();
    }

    if (no_fsync == 0)
    {
        const struct timespec start_time = timer_start ();
        ret = fsync_real (fd);
        if (stats)
        {
            stats->fsync_time += timer_end (start_time);
        }
    }
    else
    {
        ret = 0;
    }

    if (stats)
    {
        stats->fsyncs++;
    }

    return ret;
}

ssize_t pwrite64 (int fd, const void *buf, size_t count, off64_t offset)
{
    file_stats *stats = new_file (NULL, fd);
    static ssize_t (*pwrite_real) (int fd, const void *buf, size_t count, off64_t offset);
    ssize_t bytes_written;

    if (!pwrite_real)
    {
        pwrite_real =
            (ssize_t (*) (int fd, const void *buf, size_t count, off64_t offset)) dlsym (RTLD_NEXT, "pwrite64");
        setup ();
    }

    {
        const struct timespec start_time = timer_start ();
        bytes_written = pwrite_real (fd, buf, count, offset);
        stats->write_time += timer_end (start_time);
    }

    stats->written_times++;
    if (bytes_written > 0)
    {
        stats->written_blocks += (bytes_written + offset + 0Xfff) / 0X1000 - (offset / 0X1000);
        stats->written_bytes += bytes_written;
    }

    return bytes_written;
}

ssize_t pread64 (int fd, void *buf, size_t count, off64_t offset)
{
    file_stats *stats = new_file (NULL, fd);
    static ssize_t (*pread_real) (int fd, const void *buf, size_t count, off64_t offset);
    ssize_t bytes_read;

    if (!pread_real)
    {
        pread_real = (ssize_t (*) (int fd, const void *buf, size_t count, off64_t offset)) dlsym (RTLD_NEXT, "pread64");
        setup ();
    }

    {
        const struct timespec start_time = timer_start ();
        bytes_read = pread_real (fd, buf, count, offset);
        stats->read_time += timer_end (start_time);
    }

    stats->read_times++;
    if (bytes_read > 0)
    {
        stats->read_blocks += (bytes_read + offset + 0Xfff) / 0X1000 - (offset / 0X1000);
        stats->read_bytes += bytes_read;
    }

    return bytes_read;
}

ssize_t write (int fd, const void *buf, size_t count)
{
    file_stats *stats = new_file (NULL, fd);
    static ssize_t (*write_real) (int fd, const void *buf, size_t count);
    ssize_t bytes_written;

    if (!write_real)
    {
        write_real = (ssize_t (*) (int fd, const void *buf, size_t count)) dlsym (RTLD_NEXT, "write");
        setup ();
    }

    {
        const struct timespec start_time = timer_start ();
        bytes_written = write_real (fd, buf, count);
        stats->write_time += timer_end (start_time);
    }

    stats->written_times++;
    if (bytes_written > 0)
    {
        off_t offset = lseek (fd, 0, SEEK_CUR);
        stats->written_blocks += (bytes_written + offset + 0Xfff) / 0X1000 - (offset / 0X1000);
        stats->written_bytes += bytes_written;
    }

    return bytes_written;
}

ssize_t read (int fd, void *buf, size_t count)
{
    file_stats *stats = new_file (NULL, fd);
    static ssize_t (*read_real) (int fd, const void *buf, size_t count);
    ssize_t bytes_read;

    if (!read_real)
    {
        read_real = (ssize_t (*) (int fd, const void *buf, size_t count)) dlsym (RTLD_NEXT, "read");
        setup ();
    }

    {
        const struct timespec start_time = timer_start ();
        bytes_read = read_real (fd, buf, count);
        stats->read_time += timer_end (start_time);
    }

    stats->read_times++;
    if (bytes_read > 0)
    {
        off_t offset = lseek (fd, 0, SEEK_CUR);
        stats->read_blocks += (bytes_read + offset + 0Xfff) / 0X1000 - (offset / 0X1000);
        stats->read_bytes += bytes_read;
    }

    return bytes_read;
}
#endif
