#!/usr/bin/perl -w

# RDM
#
# Copyright (c) 2010 Raima Inc.,  All rights reserved.
#
# Use of this software, whether in source code format, or in executable,
# binary object code form, is governed by the Raima LICENSE which
# is fully described in the LICENSE.TXT file, included within this
# distribution of files.

use XML::Simple;
use File::Copy;
use Cwd;
use File::Path;

sub error {
    print STDERR "error: $_[0]\n";
    exit 1;
}

sub error_excludes {
    opendir(my $excludes, 'excludes') || error "Can not open the excludes directory";
    print STDERR "error: $_[0] (possible packages follows)\n";
    while (readdir $excludes) {
        if ($_ =~ /^[a-z_-]+$/) {
            print STDERR "info:    prebuild -p $_\n";
        }
    }
    closedir $excludes;
    exit 1;
}

sub usage {
    print STDERR "prebuild.pl [-h|--help] [-v|--verbose] [-f|--force]
          [--nojava] [-y|--yacc] [-N|--makedep] [-d|--depend]
          [--noild] [[-x|--depend-exe] <object>] [--compile-headers[-clean]]
          [--comb-dir <directory>] [--comb-dirs <directory>...]
    Prebuild RDM according to version.xml.
    This script should be run in the top level directory of RDM
    where versioninfo.xml(.tpl) is located.  versioninfo.xml.my
    will be used if present.  Any option accepted by the generate
    scripts are also accepted\n\n";

    system "perl perl/generate0.pl --help";
    print STDERR "\n";
    system "perl perl/generate1.pl --help";
}

# Returns true if $FORCE or first filename is older than second file
# name.
sub newer {
    $out_file= $_[0];
    $in_file= $_[1];
    if ($FORCE or not -e $out_file or
        (lstat ($in_file))[9] >= (lstat ($out_file))[9] ) {
        print "$PATH/$out_file updated\n";
        $_="t";
    }
    else {
        $_="";
    }
}

$YACC="";
$FORCE="";
$PATH=".";
$MAKEDEP="";
$NOJAVA="0";
$NOILD="";
$PACKAGE_FLAGS="";
$RELEASE_FLAGS="";
$GENERATE0_FLAGS="";
$GENERATE1_FLAGS="";
$RELEASE="";
$VERBOSE="";
$DEPEND="";
$COMPILE_HEADERS="";
$COMPILE_HEADERS_CLEAN="";

# Parse options
parse_options: while (@ARGV) {
    $ARG=$ARGV[0];
    if ($ARG eq '-f' || $ARG eq '--force' ) {
        $FORCE='-f';
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS --force";
    }
    elsif ($ARG eq '-y' || $ARG eq '--yacc') {
        $YACC="-y";
    }
    elsif ($ARG eq '-n' || $ARG eq '--nomakedep') {
        $MAKEDEP="";
    }
    elsif ($ARG eq '-N' || $ARG eq '--makedep') {
        $MAKEDEP="t";
    }
    elsif ($ARG eq '-h' || $ARG eq '--help') {
        usage; exit;
    }
    elsif ($ARG eq '-v' || $ARG eq '--verbose') {
        $VERBOSE="-v"
    }
    elsif ($ARG eq '-d' || $ARG eq '--depend') {
        $DEPEND="-d"
    }
    elsif ($ARG eq '--compile-headers') {
        $COMPILE_HEADERS="t"
    }
    elsif ($ARG eq '--compile-headers-clean') {
        $COMPILE_HEADERS_CLEAN="t"
    }
    elsif ($ARG eq '-x' || $ARG eq '--depend-exe') {
        shift @ARGV;
        $DEPEND="-x $ARGV[0]"
    }
    elsif ($ARG eq '--noild') {
        $NOILD="--noild";
    }
    elsif ($ARG eq '--nojava') {
        $NOJAVA="t";
    }
    elsif ("$ARG" eq "--release" || "$ARG" eq "--release-obj" || "$ARG" eq "--release-src" || "$ARG" eq "--release-test") {
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS --release";
        $GENERATE1_FLAGS= "$GENERATE1_FLAGS $ARG";
        $RELEASE_FLAGS= "$RELEASE_FLAGS $ARG";
        $RELEASE= "--release";
    }
    elsif  ("$ARG" eq "--cmake" || "$ARG" eq "--make" || "$ARG" eq "--vsproj" || "$ARG" eq "--vxworks" ||
            "$ARG" eq "--vxworks-rtp" || "$ARG" eq "--qnx" || "$ARG" eq "--integrity" ) {
        $NOJAVA="t";
        $GENERATE1_FLAGS= "$GENERATE1_FLAGS $ARG";
    }
    elsif  ("$ARG" eq "--package" || "$ARG" eq "-p") {
        shift @ARGV;
        if (not @ARGV) {
            error_excludes ('Expected an argument to -p');
        }
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS -p $ARGV[0]";
        $PACKAGE_FLAGS= "$PACKAGE_FLAGS -p $ARGV[0]";
    }
    elsif ("$ARG" eq '--rename-excluded') {
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS $ARGV[0]";
    }
    elsif ("$ARG" eq '--no-symlinks') {
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS $ARGV[0]";
    }
    elsif ($ARG eq '--comb-dirs' ) {
        shift @ARGV;
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS --comb-dirs $ARGV[0]";
    }
    elsif ("$ARG" eq '--comb-dir') {
        shift @ARGV;
        my $COMB_DIR= $ARGV[0];
        rmtree ("source/$COMB_DIR");
        mkdir ("source/$COMB_DIR") || error ('Failed to  mkdir ("source/$COMB_DIR")');
        system "perl perl/generate0.pl --force $PACKAGE_FLAGS $RELEASE" and error "'generate0 --force $PACKAGE_FLAGS $RELEASE' failed";
        system "perl perl/generate1.pl --comb-dir $COMB_DIR $RELEASE_FLAGS" and error "'generate1.pl --comb $COMB_DIR $RELEASE_FLAGS' failed";
        $GENERATE0_FLAGS= "$GENERATE0_FLAGS --force --comb-dirs $COMB_DIR";
        $FORCE= "--force";
    }
    elsif ($ARG eq '--') {
        shift @ARGV;
        last parse_options;
    }
    elsif (substr ($ARG, 0, 1) eq '-') {
        $GENERATE1_FLAGS= "$GENERATE1_FLAGS $ARG";
    }
    else {
        last parse_options;
    }
    shift @ARGV;
}

# Check if there is any arguments
if (@ARGV) {
    error "This command does not take arguments";
}

# Copy versioninfo.xml unless it already exist
if (-e "versioninfo.xml.release") {
    if (newer ("versioninfo.xml", "versioninfo.xml.release")) {
        copy ("versioninfo.xml.release", "versioninfo.xml");
    }
}
elsif (-e "versioninfo.xml.my") {
    if (newer ("versioninfo.xml", "versioninfo.xml.my")) {
        copy ("versioninfo.xml.my", "versioninfo.xml");
    }
}
elsif (-e "versioninfo.xml.tpl") {
    if (newer ("versioninfo.xml", "versioninfo.xml.tpl")) {
        copy ("versioninfo.xml.tpl", "versioninfo.xml");
    }
}
elsif (not -e "versioninfo.xml") {
    error "You need to run this command in the top level source directory where versioninfo.xml(.tpl) is located";
}

# Copy .clang-format files
for $dir ('.', 'include', 'GettingStarted', 'docs-examples')
{
    if (-e "$dir/.clang-format.my") {
        if (newer ("$dir/.clang-format", "$dir/.clang-format.my")) {
            copy ("$dir/.clang-format.my", "$dir/.clang-format");
        }
    }
    elsif (newer ("$dir/.clang-format", "$dir/.clang-format.tpl")) {
        copy ("$dir/.clang-format.tpl", "$dir/.clang-format");
    }
}

# Run productize
system "perl perl/productize.pl -r $FORCE $RELEASE $NOILD mainpage.md.IN include perl GettingStarted source templates CMake installer-components RELEASE-NOTES.txt.IN release-system .vscode CMakeUserPresets.json.in.IN.USER" and error "productize failed";
if (-d "test") {
    system "perl perl/productize.pl -r $FORCE $RELEASE $NOILD test" and error "productize failed";
}
if (-d "docs-examples") {
    system "perl perl/productize.pl -r $FORCE $RELEASE $NOILD docs-examples/learn" and error "productize failed";
}

# Run 

# Generate ryacc output
if ($YACC) {
    chdir "source/util";
    $PATH= "source/util";
    if (newer ("dbgram.c", "dbgram.y") || newer ("dbgram.h", "dbgram.y")) {
        system "rdmyacc -rzy -f dbgramActions  dbgram.y";
    }
    chdir "../..";

    chdir "source/dbimp";
    $PATH= "source/dbimp";
    if (newer ("imp.c", "imp.y") || newer ("imp.h", "imp.y")) {
        system "rdmyacc -rzy -f impActions imp.y";
    }
    chdir "../..";

    chdir "source/ddl";
    $PATH= "source/ddl";
    if (newer ("ddlp.c", "ddlp.y") || newer ("ddlp.h", "ddlp.y")) {
        system "rdmyacc -rzy -f ddlpActions  ddlp.y";
    }
    chdir "../..";

#    chdir "source/internal_dal";
#    $PATH= "source/internal_dal";
#    if (newer ("dal.c", "dal.y") || newer ("dal.h", "dal.y")) {
#        system "rdmyacc -rzy -f dalActions  dal.y";
#    }
#    chdir "../..";

    chdir "source/rsql";
    $PATH= "source/rsql";
    if (newer ("sscsql.c", "sscsql.y") || newer ("sscsql.h", "sscsql.y")) {
        system "rdmyacc -rzy -f sscsqlActions  sscsql.y";
    }
    chdir "../..";

    chdir "source/rsql";
    $PATH= "source/rsql";
    if (newer ("ssctabscan.c", "ssctabscan.y") || newer ("ssctabscan.h", "ssctabscan.y")) {
        system "rdmyacc -rzy -f ssctabscanActions  ssctabscan.y";
    }
    chdir "../..";

}

$ENV{"RDM_HOME"} = cwd;
if (-e "source/base") {
    chdir "source/base";
    if (newer("../../include/rdmretcodetypes.h", "./errordefns.txt") ||
        newer("../base/rettables.h", "./errordefns.txt") ||
        newer("../../include/rdmretcodetypes.h", "../../perl/genErrors.pl") ||
        newer("../base/rettables.h", "../../perl/genErrors.pl")) {
        system "perl ../../perl/genErrors.pl" and error "Errors failed";
    }
    chdir "../../";
}

if (-e "source/ado.net" ) {
    chdir "source/ado.net";
    if (newer("RsqlErrors.cs", "../base/errordefns.txt") ||
        newer("RsqlErrorCode.cs", "../base/errordefns.txt") ||
        newer("RsqlErrors.cs", "../../perl/genErrors_cs.pl") ||
        newer("RsqlErrorCode.cs", "../../perl/genErrors_cs.pl")) {
        system "perl ../../perl/genErrors_cs.pl" and error "genErrors_cs.pl failed";
    }

    if (newer("RsqlEnums.cs", "../base/value_types.h") ||
        newer("RsqlEnums.cs", "../../include/rdmdatetimetypes.h") ||
        newer("RsqlEnums.cs", "../rsql/rdmsqltypes.h") ||
        newer("RsqlEnums.cs", "../../perl/genRsqlEnums_cs.pl")) {
        system "perl ../../perl/genRsqlEnums_cs.pl" and error "genRsqlEnums_cs.pl failed";
    }

    chdir "../..";
}

if (!$NOJAVA && -e "source/jdbc") {
    chdir "source/jdbc";
    if (newer ("./java/RSQLErrors.java", "../base/errordefns.txt") ||
            newer ("./java/RSQLErrorCode.java", "../base/errordefns.txt") ||
            newer ("./java/RSQLErrors.java", "../../perl/genErrors_java.pl") ||
            newer ("./java/RSQLErrorCode.java", "../../perl/genErrors_java.pl")) {
        system "perl ../../perl/genErrors_java.pl" and error "genErrors_java failed";
    }
    if (newer ("./java/RSQLDateFormat.java", "../../include/rdmdatetimetypes.h") ||
	      newer ("./java/RSQLDateFormat.java", "../../perl/genRSQLDateFormat_java.pl")) {
        system "perl ../../perl/genRSQLDateFormat_java.pl" and error "RSQLDateFormat failed";
    }
    if (newer ("./java/RSQLParamType.java", "../rsql/rdmsqltypes.h") ||
	      newer ("./java/RSQLParamType.java", "../../perl/genRSQLParamType_java.pl")) {
        system "perl ../../perl/genRSQLParamType_java.pl" and error "RSQLParamType failed";
    }
    if (newer ("./java/RSQLCursorStatus.java", "../rsql/rdmsqltypes.h") ||
	      newer ("./java/RSQLCursorStatus.java", "../../perl/genRSQLCursorStatus_java.pl")) {
        system "perl ../../perl/genRSQLCursorStatus_java.pl" and error "RSQLCursorStatus failed";
    }
    if (newer("./java/BType.java", "../base/type_types.h") ||
        newer("./java/BType.java", "../../perl/genBType_java.pl")) {
        system "perl ../../perl/genBType_java.pl" and error "BType Failed";
    }
    chdir "../../";
}

if ($COMPILE_HEADERS) {
    chdir "test/compile_headers";
    system ("pwsh internal_maketests.ps1") and error "Running pwsh internal_maketests.ps1 failed";
    chdir "../..";
}

if ($COMPILE_HEADERS_CLEAN) {
    chdir "test/compile_headers";
    system ("pwsh internal_maketests.ps1 -clean") and error "Running pwsh internal_maketests.ps1 failed";
    chdir "../..";
}

system "pwsh perl/genExtraProfiles.ps1";
system "perl perl/generate0.pl $VERBOSE $GENERATE0_FLAGS" and error "generate0 failed";
system "perl perl/generate1.pl $VERBOSE $FORCE $NOILD $DEPEND $GENERATE1_FLAGS" and error "generate1 failed";

if (!$NOJAVA && -e "build.xml") {
    chdir "source/jdbc";
    chdir "../../";

    system ("perl perl/compileJava.pl") and error "Compiling Java failed";
}

if ($MAKEDEP) {
    system "perl perl/makedep.pl" and error "makedep failed";
}

