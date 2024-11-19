# Generate a Java class file for the RDM data types

die "FATAL: RDM_HOME environment variable not defined.\n" if (!$ENV{"RDM_HOME"});

use Convert::Binary::C;
use Hash::Util qw/lock_hash/;

use File::Temp qw (tempfile);
use Parse::Lex;

use warnings;

require $ENV{"RDM_HOME"}.'/perl/vsubs.pm';

# ----------------------------------------------------------------------------
# Initialization
my $lfrwmode = 0666;

# ----------------------------------------------------------------------------
# Get version information.
my $verfile = pp($ENV{"RDM_HOME"} . "/perl/version.pl");
do $verfile || die "Can't read file 'version.pl': $!\n";

$base_file = $ENV{"RDM_HOME"}.'/source/base/type_types.h';
$year = (localtime(time))[5] + 1900;

# Generate a header file of export function id's based on a table in code.
open(FILE, $base_file) || die "Can't open $base_file: $!\n";

# ----------------------------------------------------------------------------
# This code parses the specified C header file and extracts the enum defines
my %C_ENUMS;
{
    my $c = Convert::Binary::C->new;
    $c->configure(Define => [qw( RDM_PERL_GENTYPES )]);

    $c->parse_file($base_file);
    %C_ENUMS = map %{ $_->{enumerators} || {} }, $c->enum;
    lock_hash( %C_ENUMS );
}

# ----------------------------------------------------------------------------
# Get the output file name.
$oname = "BType.java";
$ofile = $ENV{"RDM_HOME"}."/source/jdbc/java/$oname";

# Open the output file, if any.
($out, $tmpfile) = tempfile();

# print "Generating class file '$oname'...";

# Put out standard descriptive information.
print $out
"/* ----------------------------------------------------------------------
 *   WARNING!  This is a machine-generated file -- do not hand edit.
 *
 *   To make this file, run the command:
 *        perl \$RDM_HOME\\perl\\genBType_java.pl
 *
 *   $oname: Raima Database Manager RDM type enumerations
 *
 *   Copyright (c) $year Raima Inc.  All Rights Reserved.
 * ---------------------------------------------------------------------- */

package com.raima.rdm.util;

public class BType
{
";

# Skip until the GEN_TYPE_BEGIN line is seen.
while (<FILE>)
{
    last if /GEN_TYPE_BEGIN/;
}

while (<FILE>)
{
    if (/GEN_TYPE_END/) 
    {
        last;
    }

    # Possible future:  if preprocessor line, pass it through.
    # For now:  ignore it.
    next if (/^\s*#/);

    if (/\s+(\w+)\s*=\s*(-*\w+)/)
    {
        # Skip B_TYPE_STRUCTURE for now
        if ($1 ne "B_TYPE_STRUCTURE")
        {
            my ($type_pref, $sql_type) = split('B_TYPE_', $1);
            printf($out "    public static final int t%s = (int) %d;\n", $sql_type, $C_ENUMS{$1});
        }
    }
}

close(FILE);

print $out
"}
";

close($out);

chmod($lfrwmode, $ofile) unless !(-e $ofile) || (-w $ofile);
copy($tmpfile, $ofile) || die "Can't copy $ofile: $!\n";
# printf(" updated.\n");

unlink($tmpfile);

