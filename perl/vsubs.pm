# Raima Database Manager
#
# Copyright (c) 2010 Raima Inc.,  All rights reserved.
#
# Use of this software, whether in source code format, or in executable,
# binary object code form, is governed by the Raima LICENSE which
# is fully described in the LICENSE.TXT file, included within this
# distribution of files.

# Defines global variable and functions used by other scripts. Loaded by 'require vsubs.pm'

BEGIN { require 5.004 } # 5.004 is required for Win32, DBI 0.90

use Config;
use FileHandle;
use Cwd;
use File::Basename;
use File::Copy;

# ------------------------------------------------------------------------------

if ($Config{archname} =~ /^MSWin32/)
{
    $user     = $ENV{"USERNAME"};
    $lfromode = 0555;
    $lfrwmode = 0777;
    $lfrwxmode= 0777;
    $ldmode   = 0777;
}
else
{
    $user     = $ENV{"LOGNAME"};
    $lfromode = 0444 ^ umask;
    $lfrwmode = 0666 ^ umask;
    $lfrwxmode= 0777 ^ umask;
    $ldmode   = 0777 ^ umask;
}

# ------------------------------------------------------------------------------

die "RDM_HOME must be defined!\n" unless defined($ENV{"RDM_HOME"});
##die "VCS_BRANCH must be defined!\n" unless defined($ENV{"VCS_BRANCH"});

$rdm_home   = $ENV{"RDM_HOME"};
$branch     = $ENV{"VCS_BRANCH"};
$bldcwd     = bld_rel_dir();

$localdir   = "$rdm_home/$bldcwd";

$dir_list   = "@".pp("$rdm_home/dirs");

$keyword    = "Header";

# ------------------------------------------------------------------------------

sub unix2dos ($$)
{
    my ($src, $dest) = @_;
    my $infile  = new FileHandle;
    my $outfile = new FileHandle;

    open($infile, $src)
            || (print("ERROR ($!) - cannot open source file $src.\n"), return(0));
    open($outfile, ">$dest")
            || (print("ERROR ($!) - cannot open destination file $dest.\n"), return(0));

    s/\n$/\r\n/, print($outfile $_) while (<$infile>);  # to DOS

    close($infile);
    close($outfile);

    utime((stat($src))[8..9], $dest);

    return(1);
}

# ------------------------------------------------------------------------------

sub dos2unix ($$)
{
    my ($src, $dest) = @_;
    my $infile  = new FileHandle;
    my $outfile = new FileHandle;

    open($infile, $src)
            || (print("ERROR ($!) - cannot open source file $src.\n"), return(0));
    open($outfile, ">$dest")
            || (print("ERROR ($!) - cannot open destination file $dest.\n"), return(0));

    s/\r\n$/\n/, print($outfile $_) while (<$infile>);  # to Unix

    close($infile);
    close($outfile);

    utime((stat($src))[8..9], $dest);

    return(1);
}

# ------------------------------------------------------------------------------
# retrieves path relative to $rdm_home
# (input path must use '/' as /$rdm_home(.*)/i doesn't work with '\').

sub bld_rel_dir ()
{
    my $cwd = cwd();

    $cwd      =~ s/\\/\//g;
    $rdm_home =~ s/\\/\//g;
    $cwd      =~ /$rdm_home(.*)/i;

    return(substr($1, 1));
}

# ------------------------------------------------------------------------------
# parses path and replace / with \ on Win32 and vice versa on Unix

sub pp ($)
{
    my ($inpath) = @_;

    # remove double dirchars caused by empty path components

    $inpath =~ s#//#/#g;
    $inpath =~ s#\\\\#\\#g;

    return(scalar($inpath =~ s/\//\\/g, $inpath)) if ($Config{archname} =~ /^MSWin32/);
    return(scalar($inpath =~ s/\\/\//g, $inpath));
}

# ------------------------------------------------------------------------------

sub fileeq($$)
{
    my ($file1, $file2) = @_;
    my ($line1, $line2, $ret);

    open(F1, $file1) || die "Can't open $file1: $!\n";
    open(F2, $file2) || die "Can't open $file2: $!\n";

    while (1) {
        $line1 = <F1>;
        $line2 = <F2>;

        $ret = 1, last if (!$line1 && !$line2);
        last           if (!$line1 || !$line2 || ($line1 ne $line2));
    }

    close(F1), close(F2);
    return($ret);
}

# ------------------------------------------------------------------------------

sub get_file_list ($;$$@)
{
    my ($file, $filter, $excl_notags, @tags) = @_;
    my $fh     = new FileHandle;
    my @ret    = ();
    my ($f, $ftags);

    open($fh, $file) || (print("Can't open $file: $!\n"), return(@ret));

    while (<$fh>) {
        s/\s*;.*//g;
        ($f, $ftags) = /(\S+)\s*(.*)/;
#         $plat =~ s/[\s\n]//g;

        next if (!$f);
        next if ($filter && ($f !~ /$filter/));

        push(@ret, $f), next if (!$excl_notags && (!$ftags || !@tags));

        foreach $t (@tags) {
            push(@ret, $f), last if ($f && ($ftags =~ /$t/));
        }
    }

    close($fh);
    return(@ret);
}

# ------------------------------------------------------------------------------

sub get_relver_list ($)
{
    my ($file) = @_;
    my $fh     = new FileHandle;
    my %ret    = ();
    my @fields;

    open($fh, $file) || (print("Can't open $file: $!\n"), return(@ret));

    while (<$fh>) {
        s/;.*//g;
        @fields = split /\s+/;
        next if (scalar @fields < 3);
        $ret{$fields[0]} = [($fields[1], $fields[2], $fields[3])];
    }

    close($fh);
    return(%ret);
}

# ------------------------------------------------------------------------------

sub glob_files ($)
{
    my ($str) = @_;
    return(map { -f $_ ? basename($_) : () } glob(pp($str)));
}

# ------------------------------------------------------------------------------

sub glob_dirs ($)
{
    my ($str) = @_;
    return(map { -d $_ ? basename($_) : () } glob(pp($str)));
}

# ------------------------------------------------------------------------------

sub ask ($$$)
{
    my ($question, $yes, $default) = @_;
    my $answer;

    print "$question [$default] ";

    $answer = <STDIN>;
    chop($answer);
    $answer = $default if (!$answer);

    return($answer =~ /$yes/i);
}

# ------------------------------------------------------------------------------

sub get_treelock
{
    my $fh = new FileHandle;
    my $owner;

    return(undef) if !(-e $treelock);

    open(LOCK, $treelock) || die "ERROR - Cannot open $lockfile: $!\n";
    $owner = <LOCK>;
    close(LOCK);

    return(scalar($owner =~ s/[\n\r]*//g, $owner));
}

# ------------------------------------------------------------------------------

sub update_header ($$)
{
    my ($header, $new_rev) = @_;
    my ($comment_mark, $label, $file, $rev) = split /\s+/, $header;
    my ($comment_end);

    return(scalar($header =~ s/$rev/$new_rev/, $header)) if ($rev);

    $comment_end = ' */' if ($comment_mark eq '/*');

    # octal code for $ is used to avoid VCS trying to put header info here
    return("$comment_mark \044$keyword: $new_rev \$$comment_end\n");
}

# ------------------------------------------------------------------------------
# this will modify the local work file

sub synchronize_header($$$)
{
    my ($file, $comment_prefix, $tiprev) = @_;
    my ($found);
    my $localfile = pp("$localdir/$file");
    my $tmpfile = pp("$localdir/$file.\$\$\$");

    print("ERROR - Cannot perform synchronize operation on a binary file.\n"),
            return(undef) if (-B $localfile);

    open(FILE, $localfile) || die "Can't open $file: $!\n";
    open(TMP, ">$tmpfile") || die "Failed to open temporary file\n";

    while (<FILE>) {
        $_ = update_header($_, $tiprev), $found = 1 if (/\044Revision|\044Header/);
        print TMP $_;
    }

    if (!$found) {
        # octal code for $ is used to avoid VCS trying to put header info here
        $header = sprintf("\044${keyword}%s\$", $tiprev ? ":   $tiprev " : "");
        $header = "$comment_prefix $header\n" if ($comment_prefix);
        $header = "\/* " . $header . " *\/\n" if (!$comment_prefix);
        print TMP "\n$header";
    }

    close(TMP), close(FILE);

    unlink($localfile);
    rename($tmpfile, $localfile);

    print $tiprev ? "$file synchronized to rev $tiprev\n" :
            "Revision stamp inserted into $file\n";
    return(1);
}

# ------------------------------------------------------------------------------

1;

