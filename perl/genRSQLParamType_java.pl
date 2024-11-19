# Set version numbers for build.

die "FATAL: RDM_HOME environment variable not defined.\n" if (!$ENV{"RDM_HOME"});

use File::Temp qw (tempfile);
require $ENV{"RDM_HOME"}.'/perl/vsubs.pm';


$base_h = $ENV{"RDM_HOME"}.'/source/rsql/rdmsqltypes.h';
$year = (localtime(time))[5] + 1900;

# ----------------------------------------------------------------------------

# Generate a header file of export function id's based on a table in code.
open(FILE, $base_h) || die "Can't open $file: $!\n";

# Get the output file name.
$oname = "RSQLParamType.java";
$ofile = "java/$oname";

# Open the output file, if any.
($out, $tmpfile) = tempfile();
# print "Generating class file '$ofile'...";

# Put out standard descriptive information.
print $out
"/* ----------------------------------------------------------------------
 *   WARNING!  This is a machine-generated file -- do not hand edit.
 * 
 *   To make this file, run the command: 
 *        perl \$RDM_HOME\\perl\\genRSQLParamType.pl
 *
 *   $oname: RDM SQL enumerations
 *
 *   Copyright (c) $year Raima Inc.  All Rights Reserved.
 * ---------------------------------------------------------------------- */

package com.raima.rdm.util;

public class RSQLParamType
{
";

# Skip until the GEN_RSQL_DATE_FORMAT_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_RSQL_PARAMTYPE_BEGIN/;
}

# Process until GEN_RDM_DATE_FORMAT_END.
my($inc) = 0;
while (<FILE>) {
    if (/GEN_RSQL_PARAMTYPE_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/\s+(\w+)\s*=\s*(\w+)/) {
        printf($out "    public static final short %s = (short) %s;\n", $1, $2);
        $inc = int($2);
    }
    elsif (/\s+(\w+)/) {
        $inc++;
        printf($out "    public static final short %s = (short) %d;\n", $1, $inc);
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

