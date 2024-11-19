#!/usr/bin/perl -w
# Raima Database Manager
#
# Copyright (c) 2010 Raima Inc.,  All rights reserved.
#
# Use of this software, whether in source code format, or in executable,
# binary object code form, is governed by the Raima LICENSE which
# is fully described in the LICENSE.TXT file, included within this
# distribution of files.

# Generate

use Cwd;
use Cwd 'abs_path';
use File::Copy;
use File::Path;
use File::Basename;
use Digest::MD5 qw(md5_hex);

# Error
sub error {
    my ($message_string)= @_;
    print STDERR "ERROR: $REL_DIR: $message_string\n";
    exit 1;
}

sub warning {
    my ($message_string)= @_;
    my $message= "Warning: $REL_DIR: $message_string\n";
    print STDERR $message;
    $warnings.= $message;
}

sub initialize_filelists {
    %FILELISTS= ();
}

sub add_path_to_filelist {
    my ($path_name, $filelist)= @_;

    if ($filelist eq "") {
        $filelist= "$TPLTYPE";
    }

    if (! (($RELEASE_TEST || $RELEASE_OBJ) && $DIRPATH =~ m/^source/)) {
        $FILELISTS{"$filelist"}.= "$path_name\n";
    }
}

sub add_file_to_filelist {
    my ($file_name, $filelist)= @_;
    my $path_name= normalize_path ("$REL_DIR/$file_name");
    add_path_to_filelist ($path_name, $filelist);
}

sub add_to_filelist {
    my $file;
    add_file_to_filelist ("RelGenDefines.txt", "RelGenDefines.txt");

    foreach $file (@HDRFILES) {
        add_file_to_filelist ($file, "source");
    }
    foreach $file (@SRCFILES) {
        add_file_to_filelist ($file, "source");
    }
    if ($EXECUTABLE ne "" && $LANGUAGE eq "c/cpp") {
        add_file_to_filelist ("${EXECUTABLE}_main.cpp", "source");
    }
    elsif ($EXECUTABLE ne "") {
        add_file_to_filelist ("${EXECUTABLE}_main.$LANGUAGE", "source");
    }
    if ($LIBRARY ne "" && ($LANGUAGE eq "cpp" || $LANGUAGE eq "c/cpp")) {
        add_file_to_filelist ("all_sourcex.$LANGUAGE", "source");
    }
    if ($LIBRARY ne "" && ($LANGUAGE eq "c" || $LANGUAGE eq "c/cpp")) {
        add_file_to_filelist ("all_source.$LANGUAGE", "source");
    }
    foreach $file (@RCFILES) {
        add_file_to_filelist ("$file", "source");
    }

    my $gen_macro;
    foreach $gen_macro (keys (%GEN_MACRO_RULES)) {
        my $macro;
        foreach $macro (@{$gen_macro}) {
            my $argument;
            foreach $argument (@{$GEN_MACRO_RULES{$gen_macro}{input}}) {
                my ${arg}= "${argument}";
                ${arg} =~ s/\$\{base\}/${macro}/;
                add_file_to_filelist ("${arg}", "source");
            }
        }
    }

    foreach $file (@CHECKS) {
        add_file_to_filelist ($file, "check");
    }

    foreach $file (@TESTS) {
        if ($file =~ m/Test.sh$/ || $file =~ m/Test.ps1$/ || $file =~ m/Test.bat$/ || $file =~ m/Test.pl$/) {
            add_file_to_filelist ($file, "test");
        }
    }

    foreach $file (@EXTRA_DIST) {
        add_file_to_filelist ($file, "extra-dist");
    }
}

sub print_filelists {
    mkdir "FILELISTS";

    $TPLTYPE="filelists";
    my $type;

    foreach $type (keys %FILELISTS) {
        print_file("FILELISTS/filelist-$type", $FILELISTS{"$type"});
    }
}

sub normalize_path {
    my ($path_name)= @_;

    while ($path_name =~ s|/[^/]*/\.\./|/|) {}
    while ($path_name =~ s|/\./|/|) {}

    $path_name =~ s|^[^/]*/\.\./||;
    $path_name =~ s|^\./||;
    $path_name;
}

#Used by some template scripts
sub print_file {
    my ($file_name, $content)= @_;
    my $path_name= normalize_path ("$REL_DIR/$file_name");

    add_path_to_filelist ($path_name, "$TPLTYPE");

    if ($GEN_OUTPUT) {
        if (-e $file_name) {
            chmod((stat($file_name))[2] | 0200, $file_name);
        }

        open (OUT_FILE, ">$file_name") or error "Could not open $file_name";
        print OUT_FILE $content;
        close (OUT_FILE);

        if (not $RELEASE) {
            chmod((stat($file_name))[2] & ~0222, $file_name);
        }

        print "$path_name updated\n";
    }
}

sub print_executable_file {
    my ($file_name, $content)= @_;
    my $path_name= normalize_path ("$REL_DIR/$file_name");

    add_path_to_filelist ($path_name, "$TPLTYPE");

    if ($GEN_OUTPUT) {
        if (-e $file_name) {
            chmod((stat($file_name))[2] | 0200, $file_name);
        }

        open (OUT_FILE, ">$file_name") or error "Could not open $file_name";
        print OUT_FILE $content;
        close (OUT_FILE);

        if (not $RELEASE) {
            chmod(((stat($file_name))[2] & ~0222) | 0111, $file_name);
        }

        print "$path_name updated\n";
    }
}

sub print_writeable_file {
    my ($file_name, $content)= @_;
    my $path_name= normalize_path ("$REL_DIR/$file_name");

    add_path_to_filelist ($path_name, "$TPLTYPE");

    if ($GEN_OUTPUT) {
        if (-e $file_name) {
            chmod((stat($file_name))[2] | 0200, $file_name);
        }

        open (OUT_FILE, ">$file_name") or error "Could not open $file_name";
        print OUT_FILE $content;
        close (OUT_FILE);

        print "$path_name updated\n";
    }
}

sub source {
    my ($file)= @_;
    open (FILE, "<$file") or error "Could not open $file";
    if ($VERBOSE) {
        print "DEBUG: Eval $file\n";
    }
    eval join ("", <FILE>);
    if ($@ ne "") {
        error ($@);
    }
    close (FILE);
}

sub splitAndRemoveComments {
    my ($var, $prefix)= @_;
    my @arr= ();
    my $comment="";
    my $element="";
    my @elements= split (' ',$var);

    foreach $element (@elements) {
        no warnings 'uninitialized';
        if ($element eq "#") {
            $comment="t";
        }
        elsif ($element =~ m/\$\#/) {
        }
        elsif ($comment) {
            $comment="";
        }
        elsif ($RELEASE and $element =~ m/internal_/) {
        }
        else {
            push (@arr, $prefix . $element);
        }
    }
    @arr;
}

sub generate {
    my ($script_file, $out_file)= @_;

    if ($GEN_OUTPUT and -e $out_file) {
        chmod((stat($out_file))[2] | 0200, $out_file);
    }

    if (-e "$out_file.OR") {
        my $path_name= normalize_path ("$REL_DIR/$out_file");
        add_path_to_filelist ($path_name, "");
        if ($GEN_OUTPUT) {
            copy ("$out_file.OR", "$out_file") or error ("Could not copy $out_file.OR to $out_file");
            if (not $RELEASE) {
                chmod((stat($out_file))[2] & ~0222, $out_file);
            }
            print "$path_name updated\n";
        }
        push (@{$PROJECT_SUBDIRS{$out_file}}, $DIR);
        $PROJECT_INCLUDE{$out_file}= 't';
    }
    elsif (-e "$script_file") {
        $output= "";
        $output_executable='';
        $output_writeable='';

        @SUBDIRS=@{$THIS_PROJECT_SUBDIRS{$out_file}};

        source ("$script_file");

        if ($output ne "" and $GEN_OUTPUT) {
            my $path_name= normalize_path ("$REL_DIR/$out_file");
            if ($output eq "track") {
                $output = "# Please ignore this file, this is just for dependency tracking\n";
            }
            else {
                add_path_to_filelist ($path_name, "");
            }
            if ($GEN_OUTPUT) {
                open (OUT_FILE, ">$out_file") or error "Could not open $out_file";
                print OUT_FILE $output;
                close (OUT_FILE);
                if (not $RELEASE and not $output_writeable) {
                    chmod((stat($out_file))[2] & ~0222, $out_file);
                }
                if ($output_executable) {
                    chmod((stat($out_file))[2] | 0111, $out_file);
                }
                print "$path_name updated\n";
            }

            if ($RELEASE_SRC || ! $RELEASE || $DIRPATH ne "source") {
                push (@{$PROJECT_SUBDIRS{$out_file}}, $DIR);
            }
            $PROJECT_INCLUDE{$out_file}= 't';
        }
        else {
            if ($GEN_OUTPUT) {
                unlink $out_file;
            }
        }
    }
}

sub gd_newest {
    
}

sub newer {
    my $type= $_[0];
    my $lstat_type = 0;
    if (-e $type) {
        $lstat_type = (lstat ($type))[9];
    }

    if (($FORCE or
        ($GD_NEWEST >= $lstat_type) or
        (-e "$TMPL_DIR/${type}.TP" and (lstat ("$TMPL_DIR/" . $type . ".TP"))[9] >= $lstat_type) or
        (-e "$TMPL_DIR/${type}.INIT" and (lstat ("$TMPL_DIR/" . $type . ".INIT"))[9] >= $lstat_type) or
        (-e "$type.OR" and (lstat ("$type.OR"))[9] >= $lstat_type))
        ) {
        $GEN_OUTPUT = 't';
        $_="t";
    }
    else {
        if (-e "$TMPL_DIR/${type}.TERM") {
            $GEN_OUTPUT = '';
            $_="t";
        }
        else {
            $_="";
        }
    }
}

sub addlibrary {
    my ($library)= @_;
    no warnings 'uninitialized';
    my @linklibraries= @{$LIBRARY_DEPENDENCIES {$library}};

    $includelibrary {$library}= '';

    addlibraries (@linklibraries);
}

sub addlibraries {
    my (@linklibraries)= @_;

    my $addlibrary;
    foreach $addlibrary (@linklibraries) {
        if (! exists ($includelibrary {$addlibrary})) {
            if (!exists ($EXISTLIBRARY {$addlibrary})) {
                error "Don't know how to make library $addlibrary\n";
            }
            addlibrary ($addlibrary);
        }
    }
}

sub alllinklibraries {

    if ($LIBRARY ne "") {
        push (@ALL_LIBRARIES, $LIBRARY);
        $LIBRARY_DEPENDENCIES {$LIBRARY}= [@LINKLIBRARIES];

        if ($#PLATFORM_INC >= 0) {
            my @myPLATFORM_INC = @PLATFORM_INC;
            $EXISTLIBRARY{$LIBRARY}="PLATFORM_INC";
            $PLATFORMLIBRARY{$LIBRARY} = \@myPLATFORM_INC;
        }
        elsif ($#PLATFORM_EXCL >= 0) {
            my @myPLATFORM_EXCL = @PLATFORM_EXCL;
            $EXISTLIBRARY{$LIBRARY}="PLATFORM_EXCL";
            $PLATFORMLIBRARY{$LIBRARY} = \@myPLATFORM_EXCL;
        }
        else {
            $EXISTLIBRARY{$LIBRARY}="";
        }
    }
    
    %includelibrary= ();

    if ($LIBRARY ne "") {
        addlibrary ($LIBRARY);
    }
    else {
        addlibraries (@LINKLIBRARIES);
    }

    my @alllinklibraries= ();

    my $libi;

    foreach $libi (reverse (@ALL_LIBRARIES)) {
        if (exists ($includelibrary{$libi})) {
            push (@alllinklibraries, $libi);
        }
    }

    @alllinklibraries;
}

sub existingSubdirs {
    my ($excludeNonReleaseDirs, @subdirs)= @_;
    my @existingSubdirs= ();
    my $dir;

    foreach $dir (@subdirs) {
        if ($excludeNonReleaseDirs && ($RELEASE_OBJ || $RELEASE_TEST) && $dir eq "source") {
        }
        elsif (($RELEASE && ! $RELEASE_TEST) && $dir eq "test") {
        }
        elsif ($RELEASE_TEST && ($dir eq "GettingStarted" || $dir eq "include")) {
        }
        elsif (-d $dir) {
            push (@existingSubdirs, $dir);
        }
        elsif ($excludeNonReleaseDirs) {
            if (! -d "$dir.no") {
                warning "Directory $dir does not exist";
            }
        }
    }
    @existingSubdirs;
}

sub generatedOutputFile {
    my ($file)= @_;
    my $generated= '';
    my $outputType;
    if ($file =~ m/\.h$/) {
        $outputType= "outputH";
    }
    else {
        $outputType= "outputC";
    }
    foreach $gen_macro (keys (%GEN_MACRO_RULES)) {
        foreach $macro (@{$gen_macro}) {
            foreach $argument ((@{$GEN_MACRO_RULES{$gen_macro}{$outputType}})) {
                my ${arg}= "${argument}";
                ${arg} =~ s/\$\{base\}/${macro}/;
                if (${arg} eq $file) {
                    $generated='t';
                    warning "Generated file $file exist in the source directory";
                }
            }
	}
    }
    $generated;
}

sub scanCwd {
    $EXECUTABLE="";

    $INCLUDES=join (" ", splitAndRemoveComments ($INCLUDES, ""));
    $ALL_INCLUDES= concatWWS ($INCLUDES, $ALL_INCLUDES);
    $DEFINES=join (" ", splitAndRemoveComments ($DEFINES, ""));
    $ALL_DEFINES= concatWWS ($ALL_DEFINES, $DEFINES);

    @PUBLIC_HEADERS= splitAndRemoveComments ($PUBLIC_HEADERS, "");

    {
        my $header;
        foreach $header (@PUBLIC_HEADERS) {
            if (defined $DIRHEADER_DECLARED{$header}) {
                error "${header} also listed in $DIRHEADER_DECLARED{$header}/GenDefines.txt";
            }
            else {
                $DIRHEADER_DECLARED{$header}=$REL_DIR;
                push (@ALL_PUBLIC_HEADERS, $header);
            }
        }
    }

    @LINKLIBRARIES= splitAndRemoveComments ($LINKLIBRARIES, "rdm");
    @PKGFILES= splitAndRemoveComments ($PKGFILES, "");
    @INCLUDES= splitAndRemoveComments ($INCLUDES, "");
    @DEFINES= splitAndRemoveComments ($DEFINES, "");

    foreach $gen_macro (keys (%GEN_MACRO_RULES)) {
        @{${gen_macro}}= splitAndRemoveComments (${${gen_macro}}, "");
    }

    @SUBDIRS= existingSubdirs (1, splitAndRemoveComments ($SUBDIRS, ""));
    @SYSLIBS=splitAndRemoveComments ($SYSLIBS, "");
    @PKG_CONFIG=splitAndRemoveComments ($PKG_CONFIG, "");
    @AUTOCONF_PKG_CONFIG=splitAndRemoveComments ($AUTOCONF_PKG_CONFIG, "");
    @CMAKE_FIND_PACKAGE=splitAndRemoveComments ($CMAKE_FIND_PACKAGE, "");
    @PLATFORM_INC=splitAndRemoveComments ($PLATFORM_INC, "");
    @PLATFORM_EXCL=splitAndRemoveComments ($PLATFORM_EXCL, "");
    @ALL_INCLUDES=splitAndRemoveComments ($ALL_INCLUDES, "");
    @ALL_DEFINES=splitAndRemoveComments ($ALL_DEFINES, "");
    @DEPENDENCIES=splitAndRemoveComments ($DEPENDENCIES, "");
    @GENEXCLUDE= splitAndRemoveComments ($GENEXCLUDE, "");
    @EXTRA_DIST=splitAndRemoveComments ($EXTRA_DIST, "");
    @COPIED_FILES=splitAndRemoveComments ($COPIED_FILES, "");

    if ($#PLATFORM_INC >= 0 && $#PLATFORM_EXCL >= 0) {
        warning "Cannot use both PLATFORM_INC and PLATFORM_EXCL, defaulting to using PLATFORM_INC only";
        $#PLATFORM_EXCL="";
    }

    @MAINFILE= splitAndRemoveComments ($MAINFILE, "");
    foreach $file (@MAINFILE) {
        if ($file =~ m/(.+)_main\.c$/) {
            if ($EXECUTABLE ne "") {
                error "Direcory contain more than one executable ($EXECUTABLE, $1)";
            }
            $EXECUTABLE=$1;
            $LANGUAGE{"$1"} = "c";
            if ($LANGUAGE ne "" and $LANGUAGE ne "c") {
                error "Language $LANGUAGE mixed with C source files";
            }
            $LANGUAGE="c";
        }
        elsif ($file =~ m/(.+)_main\.cpp$/) {
            if ($EXECUTABLE ne "") {
                error "Direcory contain more than one executable ($EXECUTABLE, $1)";
            }
            $EXECUTABLE=$1;
            $LANGUAGE{"$1"} = "cpp";
            if ($LANGUAGE ne "" and $LANGUAGE ne "cpp" and $LANGUAGE ne "c" and $LANGUAGE ne "c/cpp") {
                error "Language $LANGUAGE mixed with C++ source files";
            }
            if ($LANGUAGE eq "c")
            {
                $LANGUAGE= "c/cpp";
            }
            elsif ($LANGUAGE eq "")
            {
                $LANGUAGE= "cpp";
            }
        }
        elsif ($file =~ m/(.+)_main\.cs$/) {
            if ($EXECUTABLE ne "") {
                error "Direcory contain more than one executable ($EXECUTABLE, $1)";
            }
            $EXECUTABLE=$1;
            $LANGUAGE{"$1"} = "cs";
            if ($LANGUAGE ne "" and $LANGUAGE ne "cs") {
                error "Language $LANGUAGE mixed with C# source files";
            }
            $LANGUAGE="cs";
        }
        elsif ($file =~ m/(.+)_main\.java$/) {
            if ($EXECUTABLE ne "") {
                error "Direcory contain more than one executable ($EXECUTABLE, $1)";
            }
            $EXECUTABLE=$1;
            $LANGUAGE{"$1"} = "java";
            if ($LANGUAGE ne "" and $LANGUAGE ne "java") {
                error "Language $LANGUAGE mixed with Java source files";
            }
            $LANGUAGE="java";
        }
    }

    @SRCFILES= splitAndRemoveComments ($SRCFILES, "");
    foreach $file (@SRCFILES) {
        if ($file =~ m/\.cpp$/ or $file =~ m/\.cc$/) {
            if ($LANGUAGE ne "" and $LANGUAGE ne "cpp" and $LANGUAGE ne "c" and $LANGUAGE ne "c/cpp") {
                error "Language $LANGUAGE mixed with C++ source files";
            }
            if ($LANGUAGE eq "c") {
                $LANGUAGE="c/cpp";
            }
            elsif ($LANGUAGE eq "") {
                $LANGUAGE="cpp";
            }
        }
        elsif ($file =~ m/\.cs$/) {
            if ($LANGUAGE ne "" and $LANGUAGE ne "cs") {
                error "Language $LANGUAGE mixed with C# source files";
            }
            $LANGUAGE="cs";
        }
        elsif ($file =~ m/\.java$/) {
            if ($LANGUAGE ne "" and $LANGUAGE ne "java") {
                error "Language $LANGUAGE mixed with C# source files";
            }
            $LANGUAGE="java";
        }
        elsif ($file =~ m/\.c$/) {
            if ($LANGUAGE ne "" and $LANGUAGE ne "c" and $LANGUAGE ne "cpp" and $LANGUAGE ne "c/cpp") {
                error "Language $LANGUAGE mixed with C source files";
            }
            if ($LANGUAGE eq "cpp") {
                $LANGUAGE="c/cpp";
            }
            elsif ($LANGUAGE eq "") {
                $LANGUAGE="c";
            }
        }
    }
    if ("$LIBRARY" ne "") {
        $LANGUAGELIBRARY{$LIBRARY}= $LANGUAGE;
    }

    if ($BUILDTOOL) {
        $BUILDTOOL= $EXECUTABLE;
    }

    if (($EXECUTABLE ne "" && $LIBRARY ne "")) {
        error "Direcory contain more than one artifact ($EXECUTABLE, $LIBRARY)";
    }

    @TESTS= splitAndRemoveComments ($TESTS, "");

    if ($EXECUTABLE ne "") {
        if ($EXECUTABLE =~ m/Test/) {
            if ($LANGUAGE{$EXECUTABLE} eq "java") {
                push(@TESTS, $EXECUTABLE . ".jar");
            }
            else {        
                push(@TESTS, $EXECUTABLE);
            }
        }
    }

    @CHECKS= splitAndRemoveComments ($CHECKS, "");
    @RCFILES= splitAndRemoveComments ($RCFILES, "");
    @HDRFILES= splitAndRemoveComments ($HDRFILES, "");

    if ($INSTALL_HEADERS eq "yes") {
        my @publish_public_headers= ();
        my $public_header;
        public_files: foreach $public_header (@ALL_PUBLIC_HEADERS) {
            my $header;
            foreach $header (@HDRFILES) {
                if ($header eq $public_header) {
                    push(@publish_public_headers, $header);
                    next public_files;
                }
            }
            if (!$RELEASE_OBJ) {
                error "$public_header listed in $DIRHEADER_DECLARED{$public_header}/GenDefines.txt does not exist";
            }
        }

        my $header;
        header_files: foreach $header (@HDRFILES) {
            my $public_header;
            foreach $public_header (@ALL_PUBLIC_HEADERS) {
                if ($header eq $public_header) {
                    next header_files;
                }
            }
            warning "$header is not listed in \$PUBLIC_HEADERS by any GenDefines.txt";
        }
        @HDRFILES= @publish_public_headers;
    }

    add_to_filelist ();

    @ALL_LINKLIBRARIES= (); # Just to avoid a warning
    @ALL_LINKLIBRARIES= alllinklibraries ();

    if ($EXECUTABLE ne "") {
        $DIRPATH{$EXECUTABLE}=$DIRPATH;
        $ARTIFACT{$DIRPATH}=$EXECUTABLE;
    }
    if ($LIBRARY ne "") {
        $DIRPATH{$LIBRARY}=$DIRPATH;
        $ARTIFACT{$DIRPATH}=$LIBRARY;
    }
}

sub generate_all {
    my $type;
    tpltype: foreach $TPLTYPE (@TPLTYPES) {
        foreach $no (@GENEXCLUDE) {
            if ($no eq $TPLTYPE) {
                next tpltype;
            }
        }

        $tplfile = "$TMPL_DIR/$TPLTYPE.TP";
 
        if (newer ($TPLTYPE)) {
            generate ($tplfile, $TPLTYPE);
        }
        else {
            push (@{$PROJECT_SUBDIRS{$TPLTYPE}}, $DIR);
            $PROJECT_INCLUDE{$TPLTYPE}= 't';
        }
    }
    $TPLTYPE= "";
}

sub concatWWS {
    my ($s1, $s2)= @_;

    if ($s1 eq "" or $s2 eq "") {
        "$s1$s2";
    }
    else {
        "$s1 $s2";
    }
}

sub setVariables {
    # Set variable that may be set by GenDefines.txt
    $SUBDIRS="";

    %PROJECT_SUBDIRS=();
    %PROJECT_INCLUDE=();
    my $type;
    foreach $type (@TPLTYPES) {
        @{$PROJECT_SUBDIRS{$type}}=();
        $PROJECT_INCLUDE{$type}="";
    }

    $SYSLIBS="";
    $PKG_CONFIG="";
    $AUTOCONF_PKG_CONFIG="";
    $CMAKE_FIND_PACKAGE="";
    $LIBRARY="";
    $MAINFILE="";
    $SRCFILES="";
    $RCFILES="";
    $HDRFILES="";
    $EXECUTABLE="";
    $LINKLIBRARIES="";
    $PKGFILES="";
    $DESCRIPTION="";
    $INCLUDES="";
    $DEFINES="";
    $BUILDTOOL="";
    $EXTRA_DIST="";
    $COPIED_FILES="";
    $LANGUAGE="";

    foreach $gen_macro (keys (%GEN_MACRO_RULES)) {
        ${${gen_macro}}= "";
    }
    $PLATFORM_INC="";
    $PLATFORM_EXCL="";
    $DEPENDENCIES="";
    $TESTS="";
    $CHECKS="";
    $GENEXCLUDE="";
    $INSTALL_HEADERS="";
    $PUBLIC_HEADERS="";

    source ("RelGenDefines.txt");

    if ($LIBRARY ne "") {
        $LIBRARY = "rdm" . $LIBRARY;
    }

    # We always subtract one belove since generated files can end up
    # having the exact same time-stamp as the RelGenDefines.txt
    my $gd_age = (lstat ("RelGenDefines.txt"))[9] - 1;
    if ($gd_age > $GD_NEWEST) {
        $GD_NEWEST = $gd_age;
    }
}

sub processSubDir {
    my ($dir)= @_;

    my $myALL_INCLUDES= $ALL_INCLUDES;
    my $myALL_DEFINES= $ALL_DEFINES;
    my $myDIR= $DIR;
    my $myREL_DIR=$REL_DIR;
    my $myDIRPATH= $DIRPATH;
    my $mySTACK_SIZE= $STACK_SIZE;
    my $myEXCEPTIONS= $EXCEPTIONS;
    my $myRTTI= $RTTI;
    my $myGD_NEWEST = $GD_NEWEST;

    if ($ALL_INCLUDES ne "") {
        $ALL_INCLUDES= "../" . join (" ../", splitAndRemoveComments ($ALL_INCLUDES, ""));
    }

    $DIR= $dir;
    if ($DIRPATH eq ".") {
        $DIRPATH= "$dir";
    }
    else {
        $DIRPATH= "$DIRPATH/$dir";
    }
    $REL_DIR= $myREL_DIR . "/$dir";

    if ($VERBOSE) {
        print "DEBUG: Chdir $dir\n"; 
    }
    chdir ($dir) || error "Failed to process $dir";
    processCwd ();
    chdir "..";
    if ($VERBOSE) {
        print "DEBUG: Chdir ..\n"; 
    }

    $ALL_INCLUDES= $myALL_INCLUDES;
    $ALL_DEFINES= $myALL_DEFINES;
    $DIR= $myDIR;
    $REL_DIR= $myREL_DIR;
    $DIRPATH= $myDIRPATH;
    $STACK_SIZE= $mySTACK_SIZE;
    $EXCEPTIONS= $myEXCEPTIONS;
    $RTTI= $myRTTI;
    $GD_NEWEST = $myGD_NEWEST;
}

# Process GenDefines.txt in the current directory
sub processCwd {
    my %MY_PROJECT_SUBDIRS= %PROJECT_SUBDIRS;

    if (! -e "RelGenDefines.txt" ) {
        error ("The RelGenDefines.txt does not exist");
    }

    setVariables ();
    scanCwd ();

    # Save the variables needed for generate_all
    my $MY_EXECUTABLE= $EXECUTABLE;
    my @MY_LINKLIBRARIES= @LINKLIBRARIES;
    my @MY_PKGFILES= @PKGFILES;
    my $MY_DESCRIPTION= $DESCRIPTION;
    my @MY_INCLUDES= @INCLUDES;
    my @MY_DEFINES= @DEFINES;
    my $MY_STACK_SIZE= $STACK_SIZE;
    my $MY_EXCEPTIONS= $EXCEPTIONS;
    my $MY_RTTI= $RTTI;
    my @MY_SUBDIRS= @SUBDIRS;
    my %MY_PROJECT_INCLUDE= %PROJECT_INCLUDE;
    my @MY_SYSLIBS= @SYSLIBS;
    my @MY_PKG_CONFIG= @PKG_CONFIG;
    my @MY_AUTOCONF_PKG_CONFIG= @AUTOCONF_PKG_CONFIG;
    my @MY_CMAKE_FIND_PACKAGE= @CMAKE_FIND_PACKAGE;
    my @MY_PLATFORM_INC= @PLATFORM_INC;
    my @MY_PLATFORM_EXCL= @PLATFORM_EXCL;
    my @MY_ALL_INCLUDES= @ALL_INCLUDES;
    my @MY_ALL_DEFINES= @ALL_DEFINES;
    my @MY_DEPENDENCIES= @DEPENDENCIES;
    my @MY_GENEXCLUDE= @GENEXCLUDE;
    my @MY_MAINFILE= @MAINFILE;
    my @MY_SRCFILES= @SRCFILES;
    my $MY_BUILDTOOL= $BUILDTOOL;
    my @MY_TESTS= @TESTS;
    my @MY_CHECKS= @CHECKS;
    my @MY_RCFILES= @RCFILES;
    my @MY_HDRFILES= @HDRFILES;
    my $MY_INSTALL_HEADERS= $INSTALL_HEADERS;
    my @MY_PUBLIC_HEADERS=@PUBLIC_HEADERS;
    my @MY_ALL_LINKLIBRARIES= @ALL_LINKLIBRARIES;
    my @MY_EXTRA_DIST= @EXTRA_DIST;
    my @MY_COPIED_FILES= @COPIED_FILES;
    my $MY_LIBRARY= $LIBRARY;
    my $MY_DIR= $DIR;
    my $MY_DIRPATH= $DIRPATH;
    my %MY_LANGUAGE= %LANGUAGE;
    my $MY_LANGUAGE=$LANGUAGE;

    # Save all the gen macros needed for generate_all
    my @MY_EMBED_CPP= @EMBED_CPP;
    my @MY_RDMYACC_CPP= @RDMYACC_CPP;
    my @MY_RDMYACC_CPP_P= @RDMYACC_CPP_P;
    my @MY_DDLP_CPP_DBD= @DDLP_CPP_DBD;
    my @MY_DDLP_CPP_IMP= @DDLP_CPP_IMP;
    my @MY_DDLP_CPP_IMPX= @DDLP_CPP_IMPX;
    my @MY_DDLP_CPP_D_DBD= @DDLP_CPP_D_DBD;
    my @MY_DDLP_CPP_D_IMP= @DDLP_CPP_D_IMP;
    my @MY_DDLP_CPP_D_IMPX= @DDLP_CPP_D_IMPX;
    my @MY_NDDLP_CPP= @NDDLP_CPP;
    my @MY_DDLP_CPP_LC= @DDLP_CPP_LC;
    my @MY_DDLP_CPP_LC_C= @DDLP_CPP_LC_C;
    my @MY_DDLP_CPP_LC_C_D= @DDLP_CPP_LC_C_D;
    my @MY_DDLP_CPP_LC_C_E= @DDLP_CPP_LC_C_E;
    my @MY_DDLP_CPP_LC_D= @DDLP_CPP_LC_D;
    my @MY_DDLP_CPP_LC_D_E= @DDLP_CPP_LC_D_E;
    my @MY_DDLP_CPP_LC_E= @DDLP_CPP_LC_E;
    my @MY_DDLP_CPP_API_LC_C= @DDLP_CPP_API_LC_C;
    my @MY_DDLP_CPP_API_LC_C_D= @DDLP_CPP_API_LC_C_D;
    my @MY_DDLP_CPP_API_LC_C_E= @DDLP_CPP_API_LC_C_E;
    my @MY_DDLP_CPP_API_LC_C_D_E= @DDLP_CPP_API_LC_C_D_E;
    my @MY_DDLP_CPP_OBJC_C= @DDLP_CPP_OBJC_C;
    my @MY_DDLP_CPP_OBJC_C_E= @DDLP_CPP_OBJC_C_E;
    my @MY_DDLP_CPP_OBJC_C_D_E= @DDLP_CPP_OBJC_C_D_E;
    my @MY_RSQL_CPP= @RSQL_CPP;
    my @MY_RSQL_CPP_SSP= @RSQL_CPP_SSP;
    my @MY_COMPILE_CPP_S_C= @COMPILE_CPP_S_C;
    my @MY_COMPILE_CPP_C= @COMPILE_CPP_C;
    my @MY_COMPILE_CPP_C_E= @COMPILE_CPP_C_E;
    my @MY_COMPILE_CPP_API= @COMPILE_CPP_API;
    my @MY_COMPILE_CPP_API_LC= @COMPILE_CPP_API_LC;
    my @MY_COMPILE_CPP_API_LC_E= @COMPILE_CPP_API_LC_E;
    my @MY_COMPILE_CPP_API_LC_E_U= @COMPILE_CPP_API_LC_E_U;
    my @MY_COMPILE_CPP_LC_S= @COMPILE_CPP_LC_S;
    my @MY_COMPILE_CPP_LC_S_E= @COMPILE_CPP_LC_S_E;
    my @MY_COMPILE_CPP_LC_S_E_KEY_EX= @COMPILE_CPP_LC_S_E_KEY_EX;
    my @MY_COMPILE_CPP_S_C_NO_CONSTRUCTOR= @COMPILE_CPP_S_C_NO_CONSTRUCTOR;

    my @MY_EMBED= @EMBED;
    my @MY_RDMYACC= @RDMYACC;
    my @MY_RDMYACC_P= @RDMYACC_P;
    my @MY_DDLP_DBD= @DDLP_DBD;
    my @MY_DDLP_IMP= @DDLP_IMP;
    my @MY_DDLP_IMPX= @DDLP_IMPX;
    my @MY_DDLP_D_DBD= @DDLP_D_DBD;
    my @MY_DDLP_D_IMP= @DDLP_D_IMP;
    my @MY_DDLP_D_IMPX= @DDLP_D_IMPX;
    my @MY_NDDLP= @NDDLP;
    my @MY_DDLP_LC= @DDLP_LC;
    my @MY_DDLP_LC_C= @DDLP_LC_C;
    my @MY_DDLP_LC_C_D= @DDLP_LC_C_D;
    my @MY_DDLP_LC_C_E= @DDLP_LC_C_E;
    my @MY_DDLP_LC_D= @DDLP_LC_D;
    my @MY_DDLP_LC_D_E= @DDLP_LC_D_E;
    my @MY_DDLP_LC_E= @DDLP_LC_E;
    my @MY_DDLP_OBJC_C= @DDLP_OBJC_C;
    my @MY_DDLP_OBJC_C_E= @DDLP_OBJC_C_E;
    my @MY_DDLP_OBJC_C_D_E= @DDLP_OBJC_C_D_E;
    my @MY_RSQL= @RSQL;
    my @MY_RSQL_SSP= @RSQL_SSP;
    my @MY_COMPILE_C= @COMPILE_C;
    my @MY_COMPILE_S_C= @COMPILE_S_C;
    my @MY_COMPILE_C_E= @COMPILE_C_E;
    my @MY_COMPILE_LC_S_E= @COMPILE_LC_S_E;
    my @MY_COMPILE_LC_S_E_KEY_EX= @COMPILE_LC_S_E_KEY_EX;

    my $scannedCwd="";
    my $dir;

    my $myTOPDIR= $TOPDIR;
    if ($TOPDIR eq ".") {
        $TOPDIR= "..";
    }
    else {
        $TOPDIR= "$TOPDIR/..";
    }

    foreach $dir (existingSubdirs (0, splitAndRemoveComments ($SUBDIRS, ""))) {
        processSubDir ($dir);
    }
    $TOPDIR= $myTOPDIR;

    %THIS_PROJECT_SUBDIRS= %PROJECT_SUBDIRS;
    %PROJECT_SUBDIRS= %MY_PROJECT_SUBDIRS;

    # Restore the variables needed for generate_all
    $EXECUTABLE= $MY_EXECUTABLE;
    @LINKLIBRARIES= @MY_LINKLIBRARIES;
    @PKGFILES= @MY_PKGFILES;
    $DESCRIPTION= $MY_DESCRIPTION;
    @INCLUDES= @MY_INCLUDES;
    @DEFINES= @MY_DEFINES;
    $STACK_SIZE= $MY_STACK_SIZE;
    $EXCEPTIONS= $MY_EXCEPTIONS;
    $RTTI= $MY_RTTI;
    @SUBDIRS= @MY_SUBDIRS;
    %PROJECT_INCLUDE= %MY_PROJECT_INCLUDE;
    @SYSLIBS= @MY_SYSLIBS;
    @PKG_CONFIG= @MY_PKG_CONFIG;
    @AUTOCONF_PKG_CONFIG= @MY_AUTOCONF_PKG_CONFIG;
    @CMAKE_FIND_PACKAGE= @MY_CMAKE_FIND_PACKAGE;
    @PLATFORM_INC= @MY_PLATFORM_INC;
    @PLATFORM_EXCL= @MY_PLATFORM_EXCL;
    @ALL_INCLUDES= @MY_ALL_INCLUDES;
    @ALL_DEFINES= @MY_ALL_DEFINES;
    @DEPENDENCIES= @MY_DEPENDENCIES;
    @GENEXCLUDE= @MY_GENEXCLUDE;
    @MAINFILE= @MY_MAINFILE;
    @SRCFILES= @MY_SRCFILES;
    $BUILDTOOL= $MY_BUILDTOOL;
    @TESTS= @MY_TESTS;
    @CHECKS= @MY_CHECKS;
    @RCFILES= @MY_RCFILES;
    @HDRFILES= @MY_HDRFILES;
    $INSTALL_HEADERS= $MY_INSTALL_HEADERS;
    @PUBLIC_HEADERS=@MY_PUBLIC_HEADERS;
    @ALL_LINKLIBRARIES= @MY_ALL_LINKLIBRARIES;
    @EXTRA_DIST= @MY_EXTRA_DIST;
    @COPIED_FILES= @MY_COPIED_FILES;
    $LIBRARY= $MY_LIBRARY;
    $DIR= $MY_DIR;
    $DIRPATH= $MY_DIRPATH;
    %LANGUAGE= %MY_LANGUAGE;
    $LANGUAGE=$MY_LANGUAGE;

    # Restore all the gen macros needed for generate_all
    @EMBED_CPP= @MY_EMBED_CPP;
    @RDMYACC_CPP= @MY_RDMYACC_CPP;
    @RDMYACC_CPP_P= @MY_RDMYACC_CPP_P;
    @NDDLP_CPP= @MY_NDDLP_CPP;
    @DDLP_CPP_DBD= @MY_DDLP_CPP_DBD;
    @DDLP_CPP_IMP= @MY_DDLP_CPP_IMP;
    @DDLP_CPP_IMPX= @MY_DDLP_CPP_IMPX;
    @DDLP_CPP_D_DBD= @MY_DDLP_CPP_D_DBD;
    @DDLP_CPP_D_IMP= @MY_DDLP_CPP_D_IMP;
    @DDLP_CPP_D_IMPX= @MY_DDLP_CPP_D_IMPX;
    @DDLP_CPP_LC= @MY_DDLP_CPP_LC;
    @DDLP_CPP_LC_C= @MY_DDLP_CPP_LC_C;
    @DDLP_CPP_LC_C_D= @MY_DDLP_CPP_LC_C_D;
    @DDLP_CPP_LC_C_E= @MY_DDLP_CPP_LC_C_E;
    @DDLP_CPP_LC_D= @MY_DDLP_CPP_LC_D;
    @DDLP_CPP_LC_D_E= @MY_DDLP_CPP_LC_D_E;
    @DDLP_CPP_LC_E= @MY_DDLP_CPP_LC_E;
    @DDLP_CPP_API_LC_C= @MY_DDLP_CPP_API_LC_C;
    @DDLP_CPP_API_LC_C_D= @MY_DDLP_CPP_API_LC_C_D;
    @DDLP_CPP_API_LC_C_E= @MY_DDLP_CPP_API_LC_C_E;
    @DDLP_CPP_API_LC_C_D_E= @MY_DDLP_CPP_API_LC_C_D_E;
    @DDLP_CPP_OBJC_C= @MY_DDLP_CPP_OBJC_C;
    @DDLP_CPP_OBJC_C_E= @MY_DDLP_CPP_OBJC_C_E;
    @DDLP_CPP_OBJC_C_D_E= @MY_DDLP_CPP_OBJC_C_D_E;
    @RSQL_CPP= @MY_RSQL_CPP;
    @RSQL_CPP_SSP= @MY_RSQL_CPP_SSP;
    @COMPILE_CPP_S_C= @MY_COMPILE_CPP_S_C;
    @COMPILE_CPP_C= @MY_COMPILE_CPP_C;
    @COMPILE_CPP_C_E= @MY_COMPILE_CPP_C_E;
    @COMPILE_CPP_API= @MY_COMPILE_CPP_API;
    @COMPILE_CPP_API_LC= @MY_COMPILE_CPP_API_LC;
    @COMPILE_CPP_API_LC_E= @MY_COMPILE_CPP_API_LC_E;
    @COMPILE_CPP_API_LC_E_U= @MY_COMPILE_CPP_API_LC_E_U;
    @COMPILE_CPP_LC_S= @MY_COMPILE_CPP_LC_S;
    @COMPILE_CPP_LC_S_E= @MY_COMPILE_CPP_LC_S_E;
    @COMPILE_CPP_LC_S_E_KEY_EX= @MY_COMPILE_CPP_LC_S_E_KEY_EX;
    @COMPILE_CPP_S_C_NO_CONSTRUCTOR= @MY_COMPILE_CPP_S_C_NO_CONSTRUCTOR;

    @EMBED= @MY_EMBED;
    @RDMYACC= @MY_RDMYACC;
    @RDMYACC_P= @MY_RDMYACC_P;
    @NDDLP= @MY_NDDLP;
    @DDLP_DBD= @MY_DDLP_DBD;
    @DDLP_IMP= @MY_DDLP_IMP;
    @DDLP_IMPX= @MY_DDLP_IMPX;
    @DDLP_D_DBD= @MY_DDLP_D_DBD;
    @DDLP_D_IMP= @MY_DDLP_D_IMP;
    @DDLP_D_IMPX= @MY_DDLP_D_IMPX;
    @DDLP_LC= @MY_DDLP_LC;
    @DDLP_LC_C= @MY_DDLP_LC_C;
    @DDLP_LC_C_D= @MY_DDLP_LC_C_D;
    @DDLP_LC_C_E= @MY_DDLP_LC_C_E;
    @DDLP_LC_D= @MY_DDLP_LC_D;
    @DDLP_LC_D_E= @MY_DDLP_LC_D_E;
    @DDLP_LC_E= @MY_DDLP_LC_E;
    @DDLP_OBJC_C= @MY_DDLP_OBJC_C;
    @DDLP_OBJC_C_E= @MY_DDLP_OBJC_C_E;
    @DDLP_OBJC_C_D_E= @MY_DDLP_OBJC_C_D_E;
    @RSQL= @MY_RSQL;
    @RSQL_SSP= @MY_RSQL_SSP;
    @COMPILE_C= @MY_COMPILE_C;
    @COMPILE_S_C= @MY_COMPILE_S_C;
    @COMPILE_C_E= @MY_COMPILE_C_E;
    @COMPILE_LC_S_E= @MY_COMPILE_LC_S_E;
    @COMPILE_LC_S_E_KEY_EX= @MY_COMPILE_LC_S_E_KEY_EX;

    if ($DEPEND && ($DEPEND eq $EXECUTABLE || $DEPEND eq $LIBRARY)) {
        print_lib_dependencies (@LINKLIBRARIES);
    }
    else {
        generate_all ();
    }
}

# Usage information
sub usage {
    print "generate1.pl [<option>]... [--] [<template file type>]...
    This command will process the RelGenDefines.txt and produce files based
    on template files.
Options:
    [-f|--force]     Force updating destination files even though they are
                     up-to-date.
    [--noild]        No Include Line Directives in generated source files
    [-r|--release]   Release mode, but still keep source and test
    [--release-src]  Release mode for source package (exclude test)
    [--release-obj]  Release mode for object package (exclude test and source)
    [--release-test] Release mode for test package (exclude GettingStarted,
                     include, and source)
    [-h|--help]      Print this help text\n
    [-v|--verbose]   Print extra debug information\n
    [-d|--depend]    Print library dependencies
    [[-x|--depend-exe]
          <object>]  Print object dependencies for one object
    [--comb-dir DIR] Genetate a combined version of libraries into DIR
    [--cmake]        Generate files for cmake
    [--make]         Generate files for configure/make
    [--rdm]          Generate files for RDM makefiles
    [--vs2019]       Generate files for Visual Studio 2019 project files
";

    exit 1;
}

# This function is called from generators to figure out wheter the
# particular library should be included for a given platform.  For
# generators that handle multiple platforms this function need to be
# called multiple times.
#
# Targets are the once allowed in $PLATFORM_INC and $PLATFORM_EXCL,
# which currently are as follows:
#
#   WINDOWS
#   LINUX
#   DARWIN
#   QNX
#   UNIX
#   INTEGRITY
#   VXWORKS
#   VXWORKS_RTP
#
# Return values:
#
#   1 - The library is included for the particular platform
#       but may not be for other platforms
#   0 - The library is included for any target
#  -1 - The library is not included for this platform

sub libraryInclude {
    my ($platform, $linklibrary) = @_;
    my $PLATFORM = $PLATFORMLIBRARY{$linklibrary};

    my $found = -1;
    my $plat;
    foreach $plat (@$PLATFORM) {
        if ($plat eq $platform) {
            $found= 1;
        }
    }
    
    if ($EXISTLIBRARY{$linklibrary} eq "PLATFORM_INC") {
    }
    elsif ($EXISTLIBRARY{$linklibrary} eq "PLATFORM_EXCL") {
        $found = -$found;
    }
    else {
        $found = 0;
    }

    $found;
}
    

# This function is called from generators to figure out wheter the
# particular directory should be included

sub projectInclude {
    @platforms=@_;
    my $ProjectInclude;
    if ($#PLATFORM_INC >= 0) {
        $ProjectInclude= "";
        my $plat_inc;
        foreach $plat_inc (@PLATFORM_INC) {
            my $plat;
            foreach $plat (@platforms) {
                if ($plat eq $plat_inc) {
                    $ProjectInclude= "t";
                }
            }
        }
    }
    else {
        $ProjectInclude= "";
        my $plat;
        NEXT: foreach $plat (@platforms) {
            my $plat_excl;
            foreach $plat_excl (@PLATFORM_EXCL) {
                if ($plat eq $plat_excl) {
                    next NEXT;
                }
            }
            $ProjectInclude= "t";
        }
    }

    $_= $ProjectInclude;
}

# Find the template directory
sub findDir {
    my $iterations= 0;
    my $dir= ".";
    while (! -d "${dir}/templates") {
        $dir= $dir . "/..";
        $myiterations++;
        if ($myiterations > 10) {
            error "Can not find RDM_HOME"
        }
    }

    $RDM_HOME=getcwd () . "/$dir";
    $REL_DIR= $dir;
    $TMPL_DIR=$RDM_HOME . "/templates";
}

sub lib_included {
    my ($library, @libraries)= @_;

    my $lib;
    foreach $lib (@libraries) {
        if ($lib eq $library) {
            return 1;
        }
        if (lib_included ($library, @{$LIBRARY_DEPENDENCIES {$lib}})) {
            return 1;
        }
    }
    return 0;
}

sub lib_included_other {
    my ($library, @libraries)= @_;

    my $lib;
    foreach $lib (@libraries) {
        if ($lib ne $library) {
            if (lib_included ($library, @{$LIBRARY_DEPENDENCIES {$lib}})) {
                return 1;
            }
        }
    }
    return 0;
}

sub print_lib_dep {
    my ($library, $indentation)= @_;
    my $i= $indentation;
    my $myIndentation= $indentation;
    if ($indentation and $indentation =~ m/ $/) {
        chop $myIndentation;
        $myIndentation= $myIndentation . "`";
    }
    if ($LIBRARY_PRINTED->{$library}) {
        print "$myIndentation-- (($library))\n";
    }
    else {
        $LIBRARY_PRINTED->{ $library } = 't';
        print "$myIndentation-- $library\n";
        my @libraries= @{$LIBRARY_DEPENDENCIES {$library}};

        my $lib;
        my @print_libraries;
        foreach $lib (@libraries) {
            if (not lib_included_other ($lib, @libraries)) {
                push (@print_libraries, $lib);
            }
        }
        foreach $lib (@print_libraries) {
            if ($lib eq $print_libraries[$#print_libraries]) {
                print_lib_dep ($lib, $indentation . "    ");
            }
            else {
                print_lib_dep ($lib, $indentation . "   |");
            }
        }
    }
}

sub print_lib_dependencies {
    my (@LIBRARIES)= @_;
    my $lib;
    foreach $lib (@LIBRARIES) {
        if (not lib_included_other ($lib, @LIBRARIES)) { 
            print_lib_dep ($lib, "");
        }
    }
    exit 1;
}

sub print_dependencies {
    if ($DEPEND) {
        if ($DEPEND ne 't') {
            error ("Could not find object $DEPEND");
        }
        else {
            print_lib_dependencies (@ALL_LIBRARIES);
        }
    }
}

sub parse_command_line {
    # Parse the options
    $FORCE="";
    $RELEASE='';
    $ILD='l';
    $RELEASE_SRC='';
    $RELEASE_OBJ='';
    $DEPEND='';
    $VERBOSE='';
    $COMB_DIR="";

    @TPLTYPES=();

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
        elsif ($ARG eq '-f' || $ARG eq '--force' ) {
            $FORCE='t';
        }
        elsif ($ARG eq '--noild' ) {
            $ILD='';
        }
        elsif ($ARG eq '-r' || $ARG eq '--release' ) {
            $RELEASE='t';
        }
        elsif ($ARG eq '--release-src' ) {
            $RELEASE_SRC='t';
            $RELEASE='t';
        }
        elsif ($ARG eq '--release-obj' ) {
            $RELEASE_OBJ='t';
            $RELEASE='t';
        }
        elsif ($ARG eq '--release-test' ) {
            $RELEASE_TEST='t';
            $RELEASE='t';
        }
        elsif ($ARG eq '-d' || $ARG eq '--depend' ) {
            $DEPEND='t';
        }
        elsif ($ARG eq '-x' || $ARG eq '--depend-exe' ) {
            shift @ARGV;
            $DEPEND=$ARGV[0];
        }
        elsif ($ARG eq '--cmake' ) {
            push (@TPLTYPES, "CMakeLists.txt.install");
            push (@TPLTYPES, "CMakeLists.txt");
            push (@TPLTYPES, "INSTALLConfig.cmake");
            push (@TPLTYPES, "GenMacros.txt");
            push (@TPLTYPES, "Doxyfile");
            push (@TPLTYPES, "pkgconfig.in");
        }
        elsif ($ARG eq '--vs2019' ) {
            push (@TPLTYPES, "visualstudio.vcxproj.install");
            push (@TPLTYPES, "visualstudio.sln.install");
        }
        elsif ($ARG eq '--rdm' ) {
            push (@TPLTYPES, "Makefile.rdm");
        }
        elsif ($ARG eq '--comb-dir' ) {
            shift @ARGV;
            $COMB_DIR="$ARGV[0]";
            push (@TPLTYPES, "comb");
        }
        elsif (substr ($ARG, 0, 1) eq '-') {
            error "Illegal option: $ARG";
        }
        else {
            last parse_options;
        }
        shift @ARGV;
    }

    # Iterate through the arguments
    while (@ARGV) {
        $ARG=$ARGV[0];
        if (substr ($ARG, 0, 1) ne '-') {
            push (@TPLTYPES, $ARG);
        }
        shift @ARGV;}
}

sub setup_tpltypes {
    if ($DEPEND) {
        @TPLTYPES= ();
    }
    elsif ($#TPLTYPES < 0) {
        @TPLTYPES= ("CMakeLists.txt",
                    "CMakeLists.txt.install",
                    "INSTALLConfig.cmake",
                    "GenMacros.txt",
                    "Doxyfile",
    
                    "dirs",

                    "build.xml",

                    "visualstudio.vcxproj.install",
                    "visualstudio.sln.install",
                    "check.ps1",
    
                    "all_sourcex.cpp",
                    "all_source.m",
                    "all_source.c",
                    
                    "TAGS",
                    "tarTests.sh",
    
                    "Makefile.rdm",
                    "pkgconfig.in",

                    "lint.pl",
                    "indent.sh",
                    "format.sh",
                    "format",
                    "README.md",

                    ".gitignore"
            );
    }
    else {
        push (@TPLTYPES, "all_sourcex.cpp");
        push (@TPLTYPES, "all_source.m");
        push (@TPLTYPES, "all_source.c");
        push (@TPLTYPES, "TAGS");
        push (@TPLTYPES, ".gitignore");
        push (@TPLTYPES, "lint.pl");
        push (@TPLTYPES, "check.ps1");
        push (@TPLTYPES, "indent.sh");
        push (@TPLTYPES, "format.sh");
        push (@TPLTYPES, "format");
        push (@TPLTYPES, "README.md");
    }
}

sub setup_gen_macro_rules {
    my $convertOptions = "-q --no-warnings --sed";
    my $convertOptions_d = "-q --no-warnings --sed-d";

    %INTERNAL_GEN_MACRO_RULES= 
        (
         RDMYACC_CPP =>
         {
             input => ['${base}.y'],
             command => ['source/internal_rdmyacc/rdmyacc -Cbiry' . $ILD . ' -f ${base}Actions ${input_base}.y'],
             description => 'Generate yacc parser for ${base}.y',
             tool => ['rdmyacc'],
             output => ['${base}.i'],
             outputH => ['${base}.h'],
             outputC => ['${base}.cpp']
         },
         RDMYACC_CPP_P =>
         {
             input => ['${base}.y'],
             command => ['source/internal_rdmyacc/rdmyacc -Cbiryp' . $ILD . ' -f ${base}Actions ${input_base}.y'],
             description => 'Generate yacc parser for ${base}.y',
             tool => ['rdmyacc'],
             output => ['${base}.i'],
             outputH => ['${base}.h'],
             outputC => ['${base}.cpp']
         },
         RDMYACC =>
         {
             input => ['${base}.y'],
             command => ['source/internal_rdmyacc/rdmyacc -biry' . $ILD . ' -f ${base}Actions ${input_base}.y'],
             description => 'Generate yacc parser for ${base}.y',
             tool => ['rdmyacc'],
             output => ['${base}.i'],
             outputH => ['${base}.h'],
             outputC => ['${base}.c']
         },
         RDMYACC_P =>
         {
             input => ['${base}.y'],
             command => ['source/internal_rdmyacc/rdmyacc -biryp' . $ILD . ' -f ${base}Actions ${input_base}.y'],
             description => 'Generate yacc parser for ${base}.y',
             tool => ['rdmyacc'],
             output => ['${base}.i'],
             outputH => ['${base}.h'],
             outputC => ['${base}.c']
         }
        );
    %GEN_MACRO_RULES= 
        (
         EMBED_CPP => 
         {
             input => ['${base}'],
             command => ['test/qa-embed/qa-embed --cpp ${input_base}'],
             tool => ['qa-embed'],
             description => 'Embed ${base}',
             output => [],
             outputH => ['${base}.h'],
             outputC => ['${base}.cpp']
         },
         DDLP_CPP_LC =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl',
             output => ['${base}.sdl'],
             outputH => ['${base}-structs.h'],
             outputC => []
         },
         DDLP_CPP_LC_C =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --catalog --cpp ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl (C-array)',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.cpp']
         },
         DDLP_CPP_LC_C_D =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --catalog --cpp ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl (C-array) where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.cpp']
         },
         DDLP_CPP_LC_C_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --cpp ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl (C-array)',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_embed.cpp']
         },
         DDLP_CPP_LC_C_D_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --cpp ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl (C-array) where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_embed.cpp']
         },
         DDLP_CPP_LC_D =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h'],
             outputC => []
         },
         DDLP_CPP_LC_D_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --cpp ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_embed.cpp']
         },
         DDLP_CPP_LC_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --cpp ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_embed.cpp']
         },
         DDLP_CPP_API_LC_C =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl and generate interfaces for CPP++',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_gen.cpp']
         },
         DDLP_CPP_API_LC_C_D =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl and generate interfaces for C++ where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_gen.cpp']
         },
         DDLP_CPP_API_LC_C_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl and generate interfaces for C++',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_gen.cpp',
                         '${base}_embed.cpp']
         },
         DDLP_CPP_API_LC_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl and generate interfaces for C++',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h',
                         '${base}_embed.h'],
             outputC => ['${base}_gen.cpp',
                         '${base}_cat.cpp',
                         '${base}_embed.cpp']
         },
         DDLP_CPP_API_LC_C_D_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl and generate interfaces for C++ where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_gen.cpp',
                         '${base}_embed.cpp']
         },
         COMPILE_CPP_LC_S =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h'],
             outputC => []
         },
         COMPILE_CPP_C =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --catalog --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl',
             output => [],
             outputH => ['${base}_cat.h'],
             outputC => ['${base}_cat.cpp']
         },
         COMPILE_CPP_LC_S_C =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --catalog --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.cpp']
         },
         COMPILE_CPP_S_C_NO_CONSTRUCTOR =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --catalog --cpp --no-cpp-constructors ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.cpp']
         },
         COMPILE_CPP_S_C =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --catalog --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.cpp']
         },
         COMPILE_CPP_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --embed --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl with embed interface',
             output => [],
             outputH => ['${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp']
         },
         COMPILE_CPP_LC_S_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C API with embed interface',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp']
         },
         COMPILE_CPP_MC_S_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --mc-enums --embed --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl with embed interface',
             output => [],
             outputH => ['${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp']
         },
         COMPILE_CPP_LC_MC_S_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --mc-enums --embed --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C API with embed interface',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp']
         },
         COMPILE_CPP_LC_S_E_KEY_EX =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --key-ex --cpp ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C API with embed interface',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp']
         },
         COMPILE_CPP_API =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --cpp-api ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile schema for ${base}.sdl for C++ API',
             output => ['${base}_Doxygen'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_gen.cpp']
         },
         COMPILE_CPP_API_LC =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile schema for ${base}.sdl for C++ API',
             output => ['${base}_Doxygen'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h'],
             outputC => ['${base}_cat.cpp',
                         '${base}_gen.cpp']
         },
         COMPILE_CPP_API_LC_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --cpp-api --lc-cpp-methods --lc-struct-members --embed ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C++ API with embed interface',
             output => ['${base}_Doxygen'],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp',
                         '${base}_gen.cpp']
         },
         COMPILE_CPP_API_LC_E_U =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --cpp-api --uc-struct-names --embed ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C++ API with embed interface',
             output => ['${base}_Doxygen'],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h',
                         '${base}_gen_api.h'],
             outputC => ['${base}_embed.cpp',
                         '${base}_cat.cpp',
                         '${base}_gen.cpp']
         },
         EMBED => 
         {
             input => ['${base}'],
             command => ['test/qa-embed/qa-embed ${input_base}'],
             tool => ['qa-embed'],
             description => 'Embed ${base}',
             output => [],
             outputH => ['${base}.h'],
             outputC => ['${base}.c']
         },
         DDLP_LC =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl',
             output => ['${base}.sdl'],
             outputH => ['${base}-structs.h'],
             outputC => []
         },
         DDLP_LC_C =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --catalog ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl (C-array)',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.c']
         },
         DDLP_LC_C_D =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --catalog ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl (C-array) where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.c']
         },
         DDLP_LC_C_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl (C-array)',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.c',
                         '${base}_embed.c']
         },
         DDLP_LC_C_D_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl (C-array) where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.c',
                         '${base}_embed.c']
         },
         DDLP_LC_D =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert and compile schema for ${base}.ddl where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h'],
             outputC => []
         },
         DDLP_LC_D_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions_d} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl where we handle duplicate names',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.c',
                         '${base}_embed.c']
         },
         DDLP_LC_E =>
         {
             input => ['${base}.ddl'],
             command => ['source/rdm-convert/rdm-convert -q ' . ${convertOptions} . ' ${input_base}.ddl',
                         'source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed ${output_base}.sdl'],
             tool => ['rdm-convert',
                      'rdm-compile'],
             description => 'Convert, embed, and compile schema for ${base}.ddl',
             output => ['${base}.sdl'],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h',
                         '${base}_embed.h'],
             outputC => ['${base}_cat.c',
                         '${base}_embed.c']
         },
         COMPILE_LC_S =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h'],
             outputC => []
         },
         COMPILE_C =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --catalog ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl',
             output => [],
             outputH => ['${base}_cat.h'],
             outputC => ['${base}_cat.c']
         },
         COMPILE_S_C =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --catalog ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.c']
         },
         COMPILE_LC_S_C =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --catalog ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C struct definition API',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_cat.h'],
             outputC => ['${base}_cat.c']
         },
         COMPILE_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --embed ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl with embed interface',
             output => [],
             outputH => ['${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.c',
                         '${base}_cat.c']
         },
         COMPILE_LC_S_E =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C API with embed interface',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.c',
                         '${base}_cat.c']
         },
         COMPILE_LC_S_E_KEY_EX =>
         {
             input => ['${base}.sdl'],
             command => ['source/rdm-compile/rdm-compile -q --c-structs --lc-struct-members --embed --key-ex ${input_base}.sdl'],
             tool => ['rdm-compile'],
             description => 'Compile and embed schema for ${base}.sdl for C API with embed interface',
             output => [],
             outputH => ['${base}_structs.h',
                         '${base}_embed.h',
                         '${base}_cat.h'],
             outputC => ['${base}_embed.c',
                         '${base}_cat.c']
         }
        );

    if (! $RELEASE) {
        @GEN_MACRO_RULES{keys %INTERNAL_GEN_MACRO_RULES}= values %INTERNAL_GEN_MACRO_RULES;
    }
}

sub setup_variables {

    findDir ();

    @ALL_LIBRARIES= ();
    %LIBRARY_DEPENDENCIES= ();
    %EXISTLIBRARY= ();
    %PLATFORMLIBRARY= ();
    %LANGUAGELIBRARY= ();
    $ALL_INCLUDES= "";
    $ALL_DEFINES= "";
    $STACK_SIZE= "";
    $EXCEPTIONS= "0";
    $RTTI= "0";
    $DIR="...";
    $TOPDIR=".";
    $DIRPATH=".";
    %DIRPATH=();
    %LANGUAGE=();
    %DIRHEADER_DECLARED=();
    @ALL_PUBLIC_HEADERS= ();
    $TPLTYPE="";
    %FILELISTS= ();
    $GD_NEWEST= (lstat ("$TOPDIR/perl/generate1.pl"))[9];
    source ("perl/package.pl");
}

sub processing {
    foreach $TPLTYPE (@TPLTYPES) {
        $inifile= "$TMPL_DIR/$TPLTYPE.INIT";
        if (-e $inifile) {
            source ($inifile);
        }
    }
    $TPLTYPE="";

    processCwd ();

    %LIBRARY_PRINTED= ();


    foreach $TPLTYPE (@TPLTYPES) {
        $inifile= "$TMPL_DIR/$TPLTYPE.TERM";
        if (-e $inifile) {
            source ($inifile);
        }
    }
}

parse_command_line ();

setup_variables ();
setup_tpltypes ();

setup_gen_macro_rules ();

if ($REL_DIR ne "") {
    chdir ($REL_DIR);
}

$warnings="";

initialize_filelists ();
processing ();
print_filelists ();

print_dependencies ();

print $warnings;

