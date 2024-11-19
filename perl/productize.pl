# Raima Database Manager
#
# Copyright (c) 2010 Raima Inc.,  All rights reserved.
#
# Use of this software, whether in source code format, or in executable,
# binary object code form, is governed by the Raima LICENSE which
# is fully described in the LICENSE.TXT file, included within this
# distribution of files.

# Productize

$FORCE="";
$RECURSIVE="";
$PATH="";
$RELEASE="";
$NOILD="";
$USER = $ENV{'USER'} || $ENV{'USERNAME'} || 'build';

use XML::Simple;
use Cwd;
use Cwd 'abs_path';

# Set up the command and corresponding xml file
$INFO_XML="versioninfo.xml";
# Error
sub error {
    print "ERROR: $_[0]\n";
    exit 1;
}

sub warning {
    print "WARNING: $_[0]\n";
}

# Usage information
sub usage {
    print "productize [-r|--recursive] [-f|--force] [--] FILE...
    This command will process any file found with extension .IN and
    write the output to files without this extension.
    Any occurences of \@\@VARIABLE\@\@ will be replaced with the content
    of the corresponding environment variable if it is set.  Use -r
    in combination with at least one directory to recursively search
    for files with extension .IN
Options:
    [-r|--recursive] Process directories recursively
    [-|--force] Force updating destination files even though they are
                up-to-date.
    [-x|--xml] The XML file to take values from instead of ${INFO_XML}.
    [-h|--help] Print this help text.
    [-v|--version] Print out version information.
    [--noild] No Include Line Directives in generated source files
";

    exit 0;
}

sub print_file {
    my ($file_name, $content)= @_;
    if (-e $file_name) {
        chmod((stat($file_name))[2] | 0200, $file_name);
    }

    open (OUT_FILE, ">$file_name") or error "Could not open $file_name";
    print OUT_FILE $content;
    close (OUT_FILE);

    if (not $RELEASE) {
        chmod((stat($file_name))[2] & ~0222, $file_name);
    }
    print "./$PATH$file_name updated\n";
}

# Process a single file
sub process_file {
    my $in_file= $_[0];
    my $out_file= $in_file;
    if (substr($out_file, - length($USER) - 1) eq ".$USER") {
        $out_file = substr($out_file, 0, - length($USER) - 1);
    }
    elsif (substr($out_file, - 8) eq ".windows") {
        $out_file = substr($out_file, 0, - 8);
    }
    $out_file=~ s/^(.*)\.IN/$1/;
    if ($FORCE or (not -e $out_file) or
        (lstat ($in_file))[9] >= (lstat ($out_file))[9] or
        $mtime_INFO_XML > (lstat ($out_file))[9]) {
        print "./$PATH$out_file updated\n";
        open (IN_FILE, "<$in_file") or die "Could not open $in_file";
        if (-e $out_file) {
            chmod((stat($out_file))[2] | 0200, $out_file);
        }
        open (OUT_FILE, ">$out_file") or die "Could not open $out_file";
        if (not $RELEASE and not $NOILD) {
            if ($out_file =~ /.[ch]$/ || $out_file =~ /.cpp$/) {
                my $abs_path = abs_path($in_file);
                $abs_path =~ tr{\\}{/};
                print OUT_FILE "#line 1 \"" . $abs_path . "\"\n";
            }
        }
        while(<IN_FILE>) {
            $_=~ s/\@\@([A-Za-z0-9_]+)\@\@/$variables{$1}/g;
            print OUT_FILE;
        }
        close (OUT_FILE);
        if (not $RELEASE) {
            chmod((stat($in_file))[2] & ~0222, $out_file);
	}
        close (IN_FILE);
    }
}

# Recurse through the directories
sub process_directory {
    opendir (DIR, $_[0]) or die "Can not open directory $_[0]";
    my @all_files= grep { $_ ne '.' and $_ ne '..'} readdir DIR;
    closedir DIR;
    my $MY_PATH= $PATH;
    $PATH= $PATH . $_[0] . "/";
    chdir $_[0];
    my @all_in_files;
    my $generate_seen= "";
    my $windows=($^O=~/Win/)?1:0;# Are we running on windows?
    while (@all_files) {
        my $file= shift @all_files;
        if (substr($file, - length ($USER) - 4) eq ".IN.$USER" && -f $file) {
            process_file $file;
            push (@all_in_files, $file);
        }
        elsif ($windows && substr($file, - 11) eq ".IN.windows" && -f $file) {
            process_file $file;
            push (@all_in_files, $file);
        }
        elsif (substr($file, -3) eq '.IN' && -f $file) {
            process_file $file;
            push (@all_in_files, $file);
        }
        elsif ($file eq "RelGenDefines.txt") {
            $generate_seen="t";
        }
        elsif (-l $file) {
        }
        elsif (-d $file) {
            process_directory ($file);
        }
    }
    if (!$generate_seen && (not -e "RelGenDefines.txt") && $#all_in_files >= 0 && ($FORCE || (not -e ".gitignore") || (not -e ".acignore") || (lstat ("."))[9] >= (lstat (".gitignore"))[9])) {
	my $content="";
	foreach $file (@all_in_files) {
	    $file =~ s/.$USER$//;
	    $file =~ s/.IN$//;
	    $content.= "$file\n";
	}
	print_file(".gitignore", $content);
	print_file(".acignore", $content);
    }
    chdir "..";
    $PATH= $MY_PATH;
}

# Parse the options
parse_options: while (@ARGV) {
    $ARG=$ARGV[0];
    if ($ARG eq '--') {
        shift @ARGV;
        last parse_options;
    }
    elsif ($ARG eq '-h' || $ARG eq '--help') {
        usage;
    }
    elsif ($ARG eq '-v' || $ARG eq '--version') {
        print "Build @Productize_VERSION_MAJOR@.@Productize_VERSION_MINOR@\n";
    }
    elsif ($ARG eq '-x' || $ARG eq '--xml') {
        shift @ARGV;
        $ARG=$ARGV[0];
        $INFO_XML=$ARGV[0];
    }
    elsif ($ARG eq '-f' || $ARG eq '--force' ) {
        $FORCE='-f';
    }
    elsif ($ARG eq '--release' ) {
        $RELEASE='t';
    }
    elsif ($ARG eq '--noild' ) {
        $NOILD='t';
    }
    elsif ($ARG eq '-r' || $ARG eq '--recursive' ) {
        $RECURSIVE='t';
    }
    elsif (substr ($ARG, 0, 1) eq '-') {
        error "Illegal option: $ARG";
    }
    else {
        last parse_options;
    }
    shift @ARGV;
}

# Open versioninfo.xml
$xml_versioninfo =  new XML::Simple (suppressempty => '');
$versioninfo = $xml_versioninfo->XMLin("versioninfo.xml");

$var_x= "";
sub traverse {
   my $myValue= $value;
   my %myTrav= %{$myValue};
   my $myPrefix= $prefix;
   my $traversed= 0;
   while (($var, $value)= each %myTrav) {
       $traversed= 1;
       $var=~ tr/[a-z]/[A-Z]/;
       $prefix="${myPrefix}_$var";
       &traverse;
   }
   if (not $traversed) {
       $var_x= $var_x . $myPrefix . " => \"" . $myValue . "\",\n";
   }
}

$prefix= "PRODUCT";
$value=$versioninfo;
&traverse;
chop ($var_x); chop ($var_x);

%variables= (eval $var_x);

# Iterate through the arguments
$mtime_INFO_XML= (lstat($INFO_XML))[9];
while (@ARGV) {
    $ARG=$ARGV[0];
    if (-d $ARG) {
        if ($RECURSIVE) {
            process_directory ($ARG);
        }
        else {
            error "$ARG is a directory and -r is not specified";
        }
    }
    elsif (substr($ARG, -8) eq '.IN.USER') {
        $ARG = substr ($ARG, 0, -5);
        if (-f "$ARG.$USER") {
            process_file ("$ARG.$USER");
        }
    }
    elsif (substr($ARG, -3) eq '.IN') {
        if (-f "$ARG.$USER") {
            process_file ("$ARG.$USER");
        }
        elsif (-f "$ARG") {
            process_file ("$ARG");
        }
        else {
            error "productize: $ARG or $ARG.$USER does not exist";
        }
    }
    elsif (-e $ARG) {
        error "$ARG does not end in .IN"
    }
    else
    {
        warning "$ARG does not exist"
    }
    shift @ARGV;
}
