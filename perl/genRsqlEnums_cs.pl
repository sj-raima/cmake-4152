# Set version numbers for build.

die "FATAL: RDM_HOME environment variable not defined.\n" if (!$ENV{"RDM_HOME"});

use Convert::Binary::C;
use Hash::Util qw/lock_hash/;

use File::Temp qw (tempfile);

use warnings;

require $ENV{"RDM_HOME"}.'/perl/vsubs.pm';

# ----------------------------------------------------------------------------
# Initialization
my $lfrwmode = 0666;

$rsqltypes  = $ENV{"RDM_HOME"}.'/source/rsql/rdmsqltypes.h';
$basevtypes = $ENV{"RDM_HOME"}.'/source/base/type_types.h';
$basedtypes = $ENV{"RDM_HOME"}.'/include/rdmdatetimetypes.h';
$rdmtypes   = $ENV{"RDM_HOME"}.'/include/rdmtypes.h';
$year = (localtime(time))[5] + 1900;

# ----------------------------------------------------------------------------

# Get the output file name.
$ofile = "RsqlEnums.cs";

# Open the output file, if any.
($out, $tmpfile) = tempfile();

# Put out standard descriptive information.
print $out
"/* ----------------------------------------------------------------------
 *   WARNING!  This is a machine-generated file -- do not hand edit.
 * 
 *   To make this file, run the command: 
 *        perl \$RDM_HOME\\perl\\genRsqlEnums_cs.pl
 *
 *   $ofile: Raima Database Manager SQL enumerations
 *
 *   Copyright (c) $year Raima Inc.  All Rights Reserved.
 * ---------------------------------------------------------------------- */

using System;
using System.Diagnostics.CodeAnalysis;

namespace Raima.Rdm.Sql
{
    public enum SqlType
    {
";

# Generate a header file of export function id's based on a table in code.
open(FILE, $basevtypes) || die "Can't open $file: $!\n";

# ----------------------------------------------------------------------------
# This code parses the specified C header file and extracts the enum defines
my %C_ENUMS;
{
    my $c = Convert::Binary::C->new;
    $c->configure(Define => [qw( RDM_PERL_GENTYPES )]);

    $c->parse_file($basevtypes);
    %C_ENUMS = map %{ $_->{enumerators} || {} }, $c->enum;
    lock_hash( %C_ENUMS );
}

# Skip until the GEN_SQL_T_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_TYPE_BEGIN/;
}

# Process until GEN_SQL_T_END.
while (<FILE>) {
    if (/GEN_TYPE_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/\s+(\w+)\s*=\s*(-*\w+)/ && $1 ne "B_TYPE_STRUCTURE")
    {
        my ($type_pref, $sql_type) = split('B_TYPE_', $1);

        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"t\")]\n");
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $sql_type);
        printf($out "        t%s = %s,\n", $sql_type, $C_ENUMS{$1});
    }
}

close(FILE);

print $out
"    };

    public enum TransactionType
    {
";

open(FILE, $rsqltypes) || die "Can't open $file: $!\n";

# Skip until the GEN_TRANS_TYPE_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_TRANS_TYPE_BEGIN/;
}

# Process until GEN_TRANS_TYPE_END.
while (<FILE>) {
    if (/GEN_TRANS_TYPE_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/\s+(\w+)\s*=\s*(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        %s = %s,\n", $1, $2);
    }
    elsif (/\s+trans(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        %s,\n", $1);
    }
}

close(FILE);

print $out
"    };
}

namespace Raima.Rdm
{
    [SuppressMessage(\"Microsoft.Design\", \"CA1008:EnumsShouldHaveZeroValue\")]
    public enum RdmDateFormat
    {
";

open(FILE, $basedtypes) || die "Can't open $file: $!\n";

# Skip until the GEN_RDM_DATE_FORMAT_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_RDM_DATE_FORMAT_BEGIN/;
}

# Process until GEN_RDM_DATE_FORMAT_END.
while (<FILE>) {
    if (/GEN_RDM_DATE_FORMAT_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/\s+(\w+)\s*=\s*(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        %s = %s,\n", $1, $2);
    }
    elsif (/\s+(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        %s,\n", $1);
    }
}

close(FILE);

print $out
"    };
}

namespace Raima.Rdm
{
    [SuppressMessage(\"Microsoft.Design\", \"CA1008:EnumsShouldHaveZeroValue\")]
    public enum RdmTimeFormat
    {
";

open(FILE, $basedtypes) || die "Can't open $file: $!\n";

# Skip until the GEN_RDM_DATE_FORMAT_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_RDM_TIME_FORMAT_BEGIN/;
}

# Process until GEN_RDM_DATE_FORMAT_END.
while (<FILE>) {
    if (/GEN_RDM_TIME_FORMAT_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/\s+(\w+)\s*=\s*(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        %s = %s,\n", $1, $2);
    }
    elsif (/\s+(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        %s,\n", $1);
    }
}

close(FILE);

print $out
"    };
}

namespace Raima.Rdm.Sql
{
    public enum StmtType
    {
";

open(FILE, $rsqltypes) || die "Can't open $file: $!\n";

# Skip until the GEN_STMT_TYPE_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_STMT_TYPE_BEGIN/;
}

# Process until GEN_STMT_TYPE_END.
while (<FILE>) {
    if (/GEN_STMT_TYPE_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/^\s+sql(\w+)\s*=\s*(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"sql\")]\n");
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        sql%s = %s,\n", $1, $2);
    }
    elsif (/^\s+sql(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"sql\")]\n");
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        sql%s,\n", $1);
    }
}

close(FILE);

print $out
"    };

    [SuppressMessage(\"Microsoft.Naming\", \"CA1704:IdentifiersShouldBeSpelledCorrectly\", MessageId=\"Val\")]
    public enum ValStatus
    {
";

open(FILE, $basevtypes) || die "Can't open $file: $!\n";

# Skip until the GEN_VAL_STATUS_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_VAL_STATUS_BEGIN/;
}

# Process until GEN_VAL_STATUS_END.
while (<FILE>) {
    if (/GEN_VAL_STATUS_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/^\s+vs(\w+)\s*=\s*(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"vs\")]\n");
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        vs%s = %s,\n", $1, $2);
    }
    elsif (/^\s+vs(\w+)/) {
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"vs\")]\n");
        printf($out "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $1);
        printf($out "        vs%s,\n", $1);
    }
}

close(FILE);

print $out
"    };
}

namespace Raima.Rdm
{
    [SuppressMessage(\"Microsoft.Naming\", \"CA1704:IdentifiersShouldBeSpelledCorrectly\", MessageId=\"Rdm\")]
    public enum RdmOpenMode
    {
";

open(FILE, $rdmtypes) || die "Can't open $file: $!\n";

my %O_ENUMS;
{
    my $c = Convert::Binary::C->new;
    $c->configure(Define => [qw( RDM_PERL_GENTYPES )]);

    $c->parse_file($rdmtypes);
    %O_ENUMS = map %{ $_->{enumerators} || {} }, $c->enum;
    lock_hash( %O_ENUMS );
}

# Skip until the GEN_SQL_T_BEGIN line is seen.
while (<FILE>) {
    last if /GEN_OPEN_MODE_BEGIN/;
}

# Process until GEN_SQL_T_END.
while (<FILE>) {
    if (/GEN_OPEN_MODE_END/) 
    {
        last;
    }

# Possible future:  if preprocessor line, pass it through.
# For now:  ignore it.

    next if (/^\s*#/);

# This should be a definition-generating line.  Do the conversion.

    if (/\s+(\w+)\s*=\s*(-*\w+)/)
    {
        my ($type_pref, $open_mode) = split('M_', $1);

        printf($out "        %s = %s,\n", $open_mode, $O_ENUMS{$1});
    }
}

close(FILE);

print $out
"    };
}
";

close($out);

chmod($lfrwmode, $ofile) unless !(-e $ofile) || (-w $ofile);
copy($tmpfile, $ofile) || die "Can't copy $ofile: $!\n";
printf(" updated.\n");

unlink($tmpfile);

