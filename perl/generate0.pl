#!/usr/bin/perl -w
# Raima Database Manager
#
# Copyright (c) 2010 Raima Inc.,  All rights reserved.
#
# Use of this software, whether in source code format, or in executable,
# binary object code form, is governed by the Raima LICENSE which
# is fully described in the LICENSE.TXT file, included within this
# distribution of files.

use File::Copy;
use File::Basename;
use Cwd;
use Cwd 'abs_path';

#TBD use Module::Load qw[autoload]

#
# When something fails
#

sub error {
    my ($dirpath, $message)= @_;
    print STDERR "${dirpath}: ERROR: ${message}\n";
    exit 1;
}

#
# Split string up into array elements and remove comments
#

sub splitAndRemoveComments {
    my ($dirpath, $var)= @_;
    my @arr= ();
    my $comment="";
    my $element="";
    if (not defined ($var)) {
        error ($dirpath, "Variable not defined");
    }
    my @elements= split (' ',$var);;

    foreach $element (@elements) {
        no warnings 'uninitialized';
        if ($element eq "#") {
            $comment="t";
        }
        elsif ($element =~ m/\#.*/) {
        }
        elsif ($comment) {
            $comment="";
        }
        elsif ($RELEASE and $element =~ m/internal_/) {
        }
        elsif (grep(/^$element$/, @arr)) {
            #  We have a duplicate, ignore it
        }
        else {
            push (@arr, $element);
        }
    }
    @arr;
}

#
# Is the path excluded
#

sub exclude {
    my ($exclude, $file)= @_;
    my $ret = exists ($EXCLUDES{$exclude});
    my $dir = $exclude;
    $dir =~ s/\/[^\/]*$//;
    if ($ret) {
        printf STDERR "INFO: '$exclude' is excluded\n";
    }
    elsif (exists ($INCLUDES{$dir}) and not exists ($INCLUDES{$exclude})) {
        printf STDERR "INFO: '$exclude' is excluded due to only some files included in this directory\n";
        $EXCLUDES{$exclude} = 't';
        $ret = 't';
    }
    else {
        if ( not -e "$file" and -e "$file.exclude")
        {
            rename "$file.exclude", "$file";
        }
    }
    return $ret;
}

sub rename_excluded_files {
    if ($RENAME_EXCLUDED) {
        foreach $file (keys %EXCLUDES) {
            if ( -e "$file") {
                rename "$file", "$file.exclude";
            }
        }
    }
}

#
# Iterate through subdirectories
#

sub processSubDirectories {
    my ($dirpath, @SUBDIRS)= @_;
    my @INCLUDED_SUBDIRS;
    my $dir;
    foreach $dir (@SUBDIRS) {
        if (!exclude ("$dirpath/$dir", "$dir")) {
            if (-d $dir) {
                chdir ("$dir") || error ($dirpath, "Failed to process $dir");
                if (processDirectory ($dirpath . "/" . $dir)) {
                    push (@INCLUDED_SUBDIRS, $dir);
                }
                chdir ("..");
            }
            else {
                printf "WARNING: $dirpath/$dir does not exist\n";
            }
        }
    }
    return @INCLUDED_SUBDIRS;
}

#
# Process directory
#

sub processDirectory {
    my ($dirpath)= @_;
    my $include_dir= 't';

    #
    # Set variable that may be set by GenDefines.txt
    #

    my $SUBDIRS= "";
    my $LIBRARY= "";
    my $LINKLIBRARIES= "";
    my $PKG_CONFIG= "";
    my $AUTOCONF_PKG_CONFIG= "";
    my $CMAKE_FIND_PACKAGE= "";
    my $TESTS= "";
    my $CHECKS= "";

    my $SYSLIBS= "";
    my $MAINFILE= "";
    my $SRCFILES= "";
    my $RCFILES= "";
    my $HDRFILES= "";
    my $PKGFILES= "";
    my $DESCRIPTION= "";
    my $INCLUDES= "";
    my $DEFINES= "";
    my $STACK_SIZE= "";
    my $EXCEPTIONS= "0";
    my $RTTI= "0";
    my $COMB= "0";
    my $BUILDTOOL= "";
    my $TOOL= "";
    my $EXTRA_DIST= "";
    my $LANGUAGE= "";
    my $PLATFORM_INC= "";
    my $PLATFORM_EXCL= "";
    my $DEPENDENCIES= "";
    my $GENEXCLUDE= "";
    my $INSTALL_HEADERS= "";
    my $PUBLIC_HEADERS= "";
    my $COPY_FILES= "";
    my $COPIED_FILES= "";

    my $RDMYACC_CPP= "";
    my $RDMYACC_CPP_P= "";
    my $EMBED_CPP= "";
    my $DDLP_CPP_LC= "";
    my $DDLP_CPP_LC_C= "";
    my $DDLP_CPP_LC_C_D= "";
    my $DDLP_CPP_LC_C_E= "";
    my $DDLP_CPP_LC_C_D_E= "";
    my $DDLP_CPP_LC_D= "";
    my $DDLP_CPP_LC_D_E= "";
    my $DDLP_CPP_LC_E= "";
    my $DDLP_CPP_API_LC_C= "";
    my $DDLP_CPP_API_LC_C_D= "";
    my $DDLP_CPP_API_LC_C_E= "";
    my $DDLP_CPP_API_LC_E= "";
    my $DDLP_CPP_API_LC_C_D_E= "";
    my $DDLP_CPP_OBJC_C= "";
    my $DDLP_CPP_OBJC_C_E= "";
    my $DDLP_CPP_OBJC_C_D_E= "";
    my $COMPILE_CPP_LC_S= "";
    my $COMPILE_CPP_C= "";
    my $COMPILE_CPP_S_C= "";
    my $COMPILE_CPP_LC_S_C= "";
    my $COMPILE_CPP_E= "";
    my $COMPILE_CPP_LC_S_E= "";
    my $COMPILE_CPP_MC_S_E= "";
    my $COMPILE_CPP_LC_MC_S_E= "";
    my $COMPILE_CPP_LC_S_E_KEY_EX= "";
    my $COMPILE_CPP_API= "";
    my $COMPILE_CPP_API_LC= "";
    my $COMPILE_CPP_API_LC_E= "";
    my $COMPILE_CPP_API_LC_E_U= "";
    my $COMPILE_CPP_S_C_NO_CONSTRUCTOR= "";

    my $RDMYACC= "";
    my $RDMYACC_P= "";
    my $EMBED= "";
    my $DDLP_LC= "";
    my $DDLP_LC_C= "";
    my $DDLP_LC_C_D= "";
    my $DDLP_LC_C_E= "";
    my $DDLP_LC_C_D_E= "";
    my $DDLP_LC_D= "";
    my $DDLP_LC_D_E= "";
    my $DDLP_LC_E= "";
    my $DDLP_OBJC_C= "";
    my $DDLP_OBJC_C_E= "";
    my $DDLP_OBJC_C_D_E= "";
    my $COMPILE_LC_S= "";
    my $COMPILE_C= "";
    my $COMPILE_S_C= "";
    my $COMPILE_LC_S_C= "";
    my $COMPILE_E= "";
    my $COMPILE_LC_S_E= "";
    my $COMPILE_LC_S_E_KEY_EX= "";

    #
    # Evaluate GenDefines.txt
    #

    open (FILE, "<GenDefines.txt") or error ($dirpath, "Could not open GenDefines.txt");
    if ($VERBOSE) {
        print "DEBUG: Eval $dirpath/GenDefines.txt\n";
    }
    eval join ("", <FILE>);
    if ($@ ne "") {
        error ($dirpath, $@);
    }
    close (FILE);

    #
    # Copy files
    #
    if ($COPY_FILES ne "") {
        my $file;
        my @COPY_FILES= splitAndRemoveComments ($dirpath, $COPY_FILES);
        foreach $file (@COPY_FILES) {
            my($basename, $ignore1, $ignore2) = fileparse($file);
            if (exclude ("$dirpath/$basename", "$basename")) {
                printf STDERR "INFO: $file not copied due to $dirpath/$basename excluded\n"
            }
            else {
                my $windows=($^O=~/Win/)?1:0;# Are we running on windows?
                if ($RELEASE or $NO_SYMLINKS) {
                    if ($FORCE or not -e $basename or (lstat ($file))[9] >= (lstat ($basename))[9]) {
                        if (-e $basename)
                        {
                            chmod((stat($basename))[2] | 0200, $basename);
                        }
                        open (FILE, "<", $file ) or error ($dirpath, "Failed to copy $file");
                        open (FILE_COPY, ">", $basename) or error ($dirpath, "Failed to open $basename");
                        if ($basename =~ m/\.cpp$/ or $basename =~ m/\.c$/ or $basename =~ m/\.h$/)
                        {
                            print FILE_COPY "#line 1 \"" . abs_path($file) . "\"\n";
                        }
                        while (my $line = <FILE>) {
                            print FILE_COPY $line;
                        }
                        close (FILE_COPY);
                        close (FILE);
                        if (not $RELEASE) {
                            chmod((stat($basename))[2] & ~0222, $basename);
                        }
                    }
                }
                else {
                    if ($FORCE or not -e $basename or (lstat ($file))[9] >= (lstat ($basename))[9]) {
                        if (-e $basename)
                        {
                            chmod((stat($basename))[2] | 0200, $basename);
                        }
                        if (-f $basename)
                        {
                            unlink ($basename);
                        }
                        if ($windows) {
                            link($file => $basename);
                        }
                        else
                        {
                            symlink($file, $basename);
                        }
                    }
                }
                $COPIED_FILES .= " $basename";
            }
        }
    }

    #
    # Compute the list of main and source files
    #

    my @all_files = sort <*>;
    my @files = ();
    my $file;
    foreach $file (@all_files) {
        if ($RELEASE and $file =~ m/internal_/) {
        }
        elsif ($file =~ m/(.+)_main\.c(|s|pp)/) {
            $MAINFILE.= "\n    $file";
        }
        elsif ($file =~ m/(.+)_main\.java/) {
            $MAINFILE.= "\n    $file";
        }
        elsif (($file =~ m/_dbd\.c(pp)?$/) ||
               ($file =~ m/_cat\.c(pp)?$/) ||
               ($file =~ m/_ssp\.c(pp)?$/) ||
               ($file =~ m/\.ddl\.c(pp)?$/) ||
               ($file =~ m/_ddl\.c(pp)?$/) ||
               ($file =~ m/embed_.*\.c(pp)?$/) ||
               ($file =~ m/_gen\.cpp$/) ||
               ($file =~ m/_objc\.m$/) ||
               ($file =~ m/.sdl\.c(pp)?$/) ||
               ($file =~ m/.png\.c(pp)?$/) ||
               ($file =~ m/.js\.c(pp)?$/) ||
               ($file =~ m/.html\.c(pp)?$/) ||
               ($file =~ m/.ico\.c(pp)?$/) ||
               ($file =~ m/.css\.c(pp)?$/) ||
               ($file =~ m/_embed\.c(pp)?$/)) {
        }
        elsif (($file =~ m/all_sourcex.cpp/) ||
               ($file =~ m/all_source.cpp/) ||
               ($file =~ m/all_source.m/) ||
               ($file =~ m/all_source.c/)) {
        }
        elsif (($file =~ m/\.cpp$/) ||
               ($file =~ m/\.cc$/) ||
               ($file =~ m/\.java$/) ||
               ($file =~ m/\.cs$/) ||
               ($file =~ m/\.c$/)) {
            if (! exclude ("$dirpath/$file", "$file")) {
                $SRCFILES.= "\n    $file";
            }
        }
    }

    #
    # Compute the list of header files
    #

    foreach $file (@all_files) {
        if (($file =~ m/_dbd\.h$/) ||
            ($file =~ m/_cat\.h$/) ||
            ($file =~ m/_ssp\.h$/) ||
            ($file =~ m/\.ddl\.h$/) ||
            ($file =~ m/_ddl\.h$/) ||
            ($file =~ m/embed_.*\.h$/) ||
            ($file =~ m/_gen_api\.h$/) ||
            ($file =~ m/.sdl\.h$/) ||
            ($file =~ m/_struct\.h$/) ||
            ($file =~ m/_embed\.h$/)) {
        }
        elsif ($file =~ m/\.h$/ && ! exclude ("$dirpath/$file", "$file")) {
            $HDRFILES.="\n    $file";
        }
    }

    #
    # Set up RC-files
    #

    foreach $file (@all_files) {
        if ($file =~ m/^(.+)\.rc$/) {
            if ($RCFILES ne '')
            {
                error ($dirpath, "Only one RC file is allowed");
            }
            $RCFILES = $file;
        }
    }

    #
    # Set up Check scripts
    #

    foreach $file (@all_files) {
        if ($file =~ m/Check.sh$/) {
            $CHECKS.= "\n    $file";
        }
        elsif ($file =~ m/Check.ps1$/) {
            $CHECKS.= "\n    $file";
        }
        elsif ($file =~ m/Check.bat$/) {
            $CHECKS.= "\n    $file";
        }
        elsif ($file =~ m/Check.pl$/) {
            $CHECKS.= "\n    $file";
        }
    }    
    
    #
    # Set up Tests
    #

    foreach $file (@all_files) {
        if ($file =~ m/Test.sh$/) {
            if (exclude ("$dirpath/$file", "$file")) {
                # Exclude this test
            }
            else {
                $TESTS.= "\n    $file";
            }
        }
        elsif ($file =~ m/Test.ps1$/) {
            if (exclude ("$dirpath/$file", "$file")) {
                # Exclude this test
            }
            else {
                $TESTS.= "\n    $file";
            }
        }
        elsif ($file =~ m/Test.bat$/) {
            if (exclude ("$dirpath/$file", "$file")) {
                # Exclude this test
            }
            else {
                $TESTS.= "\n    $file";
            }
        }
        elsif ($file =~ m/Test.pl$/) {
            if (exclude ("$dirpath/$file", "$file")) {
                # Exclude this test
            }
            else {
                $TESTS.= "\n    $file";
            }
        }
    }
    
    #
    # Special case for the Include directory
    #
    if ($INSTALL_HEADERS ne "") {
        $HDRFILES= $ALL_PUBLIC_HEADERS;
    }

    #
    # Do we depend on a library that are excluded
    #

    if ($LINKLIBRARIES ne "") {
        my $lib;
        my @LINKLIBRARIES= splitAndRemoveComments ($dirpath, $LINKLIBRARIES);
        foreach $lib (@LINKLIBRARIES) {
            if (! exists ($EXISTLIBRARY{$lib})) {
                $include_dir= '';
            }
            if (exists $EXCEPTIONSLIBRARY{$lib}) {
                $EXCEPTIONS = '1';
            }
            if (exists $RTTILIBRARY{$lib}) {
                $RTTI = '1';
            }
        }
    }

    #
    # Add rdm-convert to dependencies when a DDLP macro is used
    #

    if ($DDLP_LC || $DDLP_LC_C || $DDLP_LC_C_D || $DDLP_LC_C_E || $DDLP_LC_C_D_E || $DDLP_LC_D || $DDLP_LC_D_E || $DDLP_LC_E ||
        $DDLP_CPP_LC || $DDLP_CPP_LC_C || $DDLP_CPP_LC_C_D || $DDLP_CPP_LC_C_E || $DDLP_CPP_LC_C_D_E || $DDLP_CPP_LC_D || $DDLP_CPP_LC_D_E || $DDLP_CPP_LC_E || 
        $DDLP_CPP_API_LC_C || $DDLP_CPP_API_LC_C_D || $DDLP_CPP_API_LC_C_E || $DDLP_CPP_API_LC_E || $DDLP_CPP_API_LC_C_D_E ||
        $DDLP_OBJC_C || $DDLP_OBJC_C_E || $DDLP_OBJC_C_D_E ||
        $DDLP_CPP_OBJC_C || $DDLP_CPP_OBJC_C_E || $DDLP_CPP_OBJC_C_D_E) {
        $DEPENDENCIES.= " rdm-convert";
    }

    #
    # Do we depend on a tool that are excluded
    #

    if ($DEPENDENCIES ne "") {
        my $dep;
        my @DEPENDENCIES = splitAndRemoveComments ($dirpath, $DEPENDENCIES );
        foreach $dep (@DEPENDENCIES ) {
            if (! exists ($EXISTTOOL{$dep})) {
                $include_dir= '';
            }
        }
    }

    if ($include_dir) {
        $ALL_PUBLIC_HEADERS.= $PUBLIC_HEADERS;

        my @variables = (
            "LINKLIBRARIES",
            "PKG_CONFIG",
            "AUTOCONF_PKG_CONFIG",
            "CMAKE_FIND_PACKAGE",
            "TESTS",
            "CHECKS",
            "SYSLIBS",
            "MAINFILE",
            "SRCFILES",
            "RCFILES",
            "HDRFILES",
            "PKGFILES",
            "INCLUDES",
            "DEFINES",
            "COPIED_FILES",
            "LANGUAGE",
            "PLATFORM_INC",
            "PLATFORM_EXCL",
            "DEPENDENCIES",
            "GENEXCLUDE",
            "PUBLIC_HEADERS",
            );
        my @gen_macros = (
            "EMBED_CPP",
            "DDLP_CPP_LC",
            "DDLP_CPP_LC_C",
            "DDLP_CPP_LC_C_D",
            "DDLP_CPP_LC_C_E",
            "DDLP_CPP_LC_C_D_E",
            "DDLP_CPP_LC_D",
            "DDLP_CPP_LC_D_E",
            "DDLP_CPP_LC_E",
            "DDLP_CPP_API_LC_C",
            "DDLP_CPP_API_LC_C_D",
            "DDLP_CPP_API_LC_C_E",
            "DDLP_CPP_API_LC_E",
            "DDLP_CPP_API_LC_C_D_E",
            "DDLP_CPP_OBJC_C",
            "DDLP_CPP_OBJC_C_E",
            "DDLP_CPP_OBJC_C_D_E",
            "COMPILE_CPP_LC_S",
            "COMPILE_CPP_C",
            "COMPILE_CPP_LC_S_C",
            "COMPILE_CPP_S_C",
            "COMPILE_CPP_E",
            "COMPILE_CPP_LC_S_E",
            "COMPILE_CPP_MC_S_E",
            "COMPILE_CPP_LC_MC_S_E",
            "COMPILE_CPP_LC_S_E_KEY_EX",
            "COMPILE_CPP_API",
            "COMPILE_CPP_API_LC",
            "COMPILE_CPP_API_LC_E",
            "COMPILE_CPP_API_LC_E_U",
            "COMPILE_CPP_S_C_NO_CONSTRUCTOR",

            "EMBED",
            "DDLP_LC",
            "DDLP_LC_C",
            "DDLP_LC_C_D",
            "DDLP_LC_C_E",
            "DDLP_LC_C_D_E",
            "DDLP_LC_D",
            "DDLP_LC_D_E",
            "DDLP_LC_E",
            "DDLP_OBJC_C",
            "DDLP_OBJC_C_E",
            "DDLP_OBJC_C_D_E",
            "COMPILE_LC_S",
            "COMPILE_C",
            "COMPILE_S_C",
            "COMPILE_LC_S_C",
            "COMPILE_E",
            "COMPILE_LC_S_E",
            "COMPILE_LC_S_E_KEY_EX",
            );
        if (! $RELEASE) {
            push (@gen_macros, ("RDMYACC_CPP", "RDMYACC_CPP_P", "RDMYACC", "RDMYACC_P"));
        }
        
        #
        # If we are a library mark it as not being excluded
        #

        if ($LIBRARY ne "") {
            $EXISTLIBRARY{$LIBRARY}='t';
        }

        #
        # If we are a tool mark it as not being excluded
        #

        if ($TOOL ne "") {
            $EXISTTOOL{$TOOL}='t';
        }

        #
        # Process sub directories
        #

        my @SUBDIRS= processSubDirectories ($dirpath, splitAndRemoveComments ($dirpath, $SUBDIRS));

        #
        # Evaluate PostDefines.txt
        #

        if (open (FILE, "<PostDefines.txt")) {
            if ($VERBOSE) {
                print "DEBUG: Eval $dirpath/PostDefines.txt\n";
            }
            eval join ("", <FILE>);
            if ($@ ne "") {
                error ($dirpath, $@);
            }
            close (FILE);
        }

        #
        # Compute the output
        #

        my $output= "";
        if ($#SUBDIRS >= 0) {
            $output.= "\$SUBDIRS= \"\n  ";
            $output.= join ("\n  ", @SUBDIRS);
            $output.= "\n\";\n\n";
        }

        if ($DESCRIPTION ne "") {
            $output.= "\$DESCRIPTION= \"$DESCRIPTION\";\n\n";
        }
        if ($LIBRARY ne "") {
            $output.= "\$LIBRARY= \"$LIBRARY\";\n\n";
        }
        if ($STACK_SIZE ne "") {
            $output.= "\$STACK_SIZE= \"$STACK_SIZE\";\n\n";
        }

        if ($MAINFILE ne "" or $LIBRARY ne "")
        {
            $output.= "\$EXCEPTIONS= \"$EXCEPTIONS\";\n\n";
            if ($LIBRARY && $EXCEPTIONS) {
                $EXCEPTIONSLIBRARY{$LIBRARY}='t';
            }
            $output.= "\$RTTI= \"$RTTI\";\n\n";
            if ($LIBRARY && $RTTI) {
                $RTTILIBRARY{$LIBRARY}='t';
            }
            $output.= "\$COMB=\"$COMB\";\n\n";
            if ($COMB) {
                $COMBLIBRARY{$LIBRARY}='t';
            }
        }

        if ($BUILDTOOL ne "") {
            $output.= "\$BUILDTOOL= \"$BUILDTOOL\";\n\n";
        }

        if ($INSTALL_HEADERS ne "") {
            $output.= "\$INSTALL_HEADERS= \"$INSTALL_HEADERS\";\n\n";
        }

        #
        # Compute the output
        #

        my @EXTRA_DIST= splitAndRemoveComments ($dirpath, $EXTRA_DIST);

        if ($#EXTRA_DIST >= 0) {
            $output.= "\$EXTRA_DIST= \"\n";
            foreach $file (@EXTRA_DIST) {
                if (exclude ("$dirpath/$file", "$file")) {
                    # Exclude this EXTRA_DIST
                }
                else {
                    $output.= "    $file\n";
                }
            }
            $output.= "\";\n\n";
        }

        my $variable;

        foreach $variable ((@variables, @gen_macros)) {
            my $content= eval "\${${variable}}";
            my @content= splitAndRemoveComments ($dirpath, $content);
            if ($#content >= 0) {
                $output.= "\$${variable}= \"\n  ";
                $output.= join ("\n  ", @content);
                $output.= "\n\";\n\n";
            }
        }

        my $changes = 't';
        if (open (my $fh, "<RelGenDefines.txt")) {
            my $existing_output = '';
            while ($line= <$fh> ) {
                $existing_output.= $line;
            }
            if ($existing_output eq $output) {
                $changes = '';
            }
            close ($fh);
        }
        
        #
        # Generate the output
        #

        if ($output ne "" and ($#SUBDIRS ge 0 or $MAINFILE ne "" or $LIBRARY ne "" or $TESTS ne "" or
                               $INSTALL_HEADERS ne "" or $CHECKS ne "" or $BUILDTOOL ne "" or $DEPENDENCIES ne "" or $EXTRA_DIST ne "")) {
            if ($changes) {
                if (-e "RelGenDefines.txt") {
                    chmod((stat("RelGenDefines.txt"))[2] | 0200, "RelGenDefines.txt");
                }
                open (my $fh, ">RelGenDefines.txt") or error ($dirpath, "Could not open RelGenDefines.txt");
                print $fh $output;
                close ($fh);

                if (not $RELEASE) {
                    chmod((stat("RelGenDefines.txt"))[2] & ~0222, "RelGenDefines.txt");
                }
            }
        }
        else {
            $include_dir= '';
        }
    }

    #
    # Return wether we included this directory or no
    #

    $include_dir;
}

sub usage {
    print "generate0.pl [<option>]...

    This command will process the GenDefine.txt and produce a
    RelGenDefines.txt.  If there are no changes needed its
    timestamp will remain the same.

    A package option is required when used from the release-system.
    For development use the package option to set up project and
    make files similar to a release package or use a pseudo package
    name with the purpose of excluding certain libraries.  We use
    'inmem', 'embed', and 'client' for our tests to link with the
    respective tfs_* library.
Options:
    [-h|--help]             Print this help text
    [-v|--verbose]          Print extra debug information
    [-r|--release]          Release mode
    [-f|--force]            Force
    [-p|--package] PACKAGE  Specify a package. PACKAGE can be file names
                            separated by '--'. Each file name must exist in
                            the excludes directory
    [--rename-excluded]     Rename excluded files and directories
    [--no-symlinks]         Do not use symlinks instead of copying files
    [--comb-dirs DIR...]    Add DIR to the directories to process under
                              source.  This option is usually used in
                              combination with --comb-dir DIR passed to
                              generate1.pl.
";
    exit 1;
}

sub parse_command_line {
    # Parse the options
    $RELEASE='';
    $VERBOSE='';
    $PACKAGE='';
    $FORCE='';
    $RENAME_EXCLUDED='';
    $NO_SYMLINKS='';

    parse_options: while (@ARGV) {
        $ARG=$ARGV[0];
        if ($ARG eq '--') {
            shift @ARGV;
            last parse_options;
        }
        elsif ($ARG eq '-h' || $ARG eq '--help') {
            usage;
        }
        elsif ($ARG eq '-v' || $ARG eq '--verbose') {
            $VERBOSE='t';
        }
        elsif ($ARG eq '-r' || $ARG eq '--release' ) {
            $RELEASE='t';
        }
        elsif ($ARG eq '-f' || $ARG eq '--force' ) {
            $FORCE='t';
        }
        elsif ($ARG eq '-p' || $ARG eq '--package' ) {
            shift @ARGV;
            $PACKAGE=$ARGV[0];
        }
        elsif ($ARG eq '--comb-dirs' ) {
            shift @ARGV;
            $COMB_DIRS=$ARGV[0];
        }
        elsif ($ARG eq '--rename-excluded' ) {
            $RENAME_EXCLUDED='t';
        }
        elsif ($ARG eq '--no-symlinks' ) {
            $NO_SYMLINKS='t';
        }
        shift @ARGV;
    }
}

#
#  Set up the excludes for a given package or user
#
sub get_excludes_from_file {
    my ($exclude_file_name)= @_;

    if (open (my $fh, '<excludes/' . $exclude_file_name)) {
        while (my $exclude = <$fh>) {
            $exclude =~ s/^([^ \t\n#]*)[ \t]*([#].*)?[\n]*$/$1/;
            if ($exclude =~ m/[+]\w/) {
                $exclude =~ s/^[+]//;
                if (not -e $exclude) {
                    printf ("WARNING: File $exclude for $PACKAGE does not exist\n");
                }
                else {
                    if ($VERBOSE) {
                        printf ('DEBUG: ' . '<includes/' . $PACKAGE . ': ' . $exclude . "\n");
                    }
                    $INCLUDES{"./$exclude"}='t';
                    my $dir = $exclude;
                    $dir =~ s/\/[^\/]*$//;
                    $INCLUDES{"./$dir"}='t';
                }
            }
            else {
                if ($exclude =~ m/[-]/) {
                    $exclude =~ s/^[-]//;
                }
                if ($exclude =~ m/\w/) {
                    if (not -e $exclude)
                    {
                        printf ("WARNING: File $exclude for $PACKAGE does not exist\n");
                        $EXCLUDES{"./$exclude"}='t';
                    }
                    else {
                        if ($VERBOSE) {
                            printf ('DEBUG: ' . '<excludes/' . $PACKAGE . ': ' . $exclude . "\n");
                        }
                        $EXCLUDES{"./$exclude"}='t';
                    }
                }
            }
        }
        close $fh;
    }
}

#
#  Set up all the excludes
#
sub get_excludes {
    my $username = $ENV{USER} || $ENV{USERNAME} || 'build';

    get_excludes_from_file ($username);

    foreach my $package (split ('--', $PACKAGE)) {
        get_excludes_from_file ($package)
            or error ('.', "excludes for $PACKAGE does not exist (excludes/$package)");
    }
}

%EXISTLIBRARY= ();
%EXISTTOOL= ();
%EXCLUDES= ();
%INCLUDES= ();
%EXCEPTIONSLIBRARY= ();
%RTTILIBRARY= ();
%COMBLIBRARY= ();
$ALL_PUBLIC_HEADERS= "";
$COMB_DIRS="";
parse_command_line();
get_excludes();
processDirectory(".");
rename_excluded_files();
