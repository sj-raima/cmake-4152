# Set version numbers for build.

require 5.005;

die "FATAL: RDM_HOME environment variable not defined.\n" if (!$ENV{"RDM_HOME"});

use File::Temp qw (tempfile);
use Parse::Lex;

require $ENV{"RDM_HOME"}.'/perl/vsubs.pm';

# Get version information.
$verfile = pp($ENV{"RDM_HOME"} . "/perl/version.pl");
do $verfile || die "Can't read file 'version.pl': $!\n";

$RDM_HOME = $ENV{"RDM_HOME"};
$javacodes_name = 'RsqlErrorCode.cs';
$retjavas_name = 'RsqlErrors.cs';

$errsource = $RDM_HOME.'/source/base/errordefns.txt';
$errdest_jcodes = $RDM_HOME.'/source/ado.net/'.$javacodes_name;
$errdest_javas = $RDM_HOME.'/source/ado.net/'.$retjavas_name;

$year = (localtime(time))[5] + 1900;

# ----------------------------------------------------------------------------

# Generate a header file of export function id's based on a table in code.
my $handle = new IO::File;
$handle->open("< $errsource") || die "Can't open $file: $!\n";

@token = ('COMMA', ',',
          'EOL', '\n',
          'NUMBER', '-?\d+',
          'STRING', '"(\\.|[^"])*"', sub { substr $_[1], 1, -1 },
          'CODE', 'code',
          'SKIP', 'skip.*', sub { substr $_[1], 5 },
          'COMMENT', 'comment.*', sub { substr $_[1], 8 },
          'GROUP', 'group',
          'OTHER', '.*');
$preamble_start =
"/* ----------------------------------------------------------------------------
 * ".$hdr_version."
 *
 * ".$hdr_copyright."
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the RaimaInternal LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 * ----------------------------------------------------------------------------
 */

/* ----------------------------------------------------------------------------
 *  WARNING!  This is a machine-generated file -- do not hand edit.
 *
";
$preamble_end =
" *  Error codes are defined in file source/base/errordefns.txt
 *
 * ----------------------------------------------------------------------------
 */
";

##Parse::Lex->trace;
my $lexer = Parse::Lex->new(@token);
$lexer->from($handle);

my $token;
my $name;
my $desc;
my $incr;
#my $errors;

my @errors;
my $currgroup = -1;
my $ndx;
my $info;
my $len;
my $maxgrouplen = 0;
my $maxcodelen = 0;

TOKEN:while (1)
{
    $token = $lexer->next;
    last TOKEN if $lexer->eoi;
    next TOKEN if $token->type eq 'COMMENT' or $token->type eq 'EOL';

    if ($token->type eq 'GROUP')
    {
        my ($groupname, $startnum) = get_group();
        die "$errsource:".$lexer->line.":0: error: invalid group specification\n" unless defined($groupname);
        
        $errors[++$currgroup]{'name'} = $groupname;
        $errors[$currgroup]{'startnum'} = $startnum;
        $ndx = 0;
        $len = length($groupname);
        $maxgrouplen = $len if $maxgrouplen < $len;
    }
    elsif ($token->type eq 'CODE')
    {
        my ($num, $name, $desc, $sqlstate) = get_code();
        die "$errsource:".$lexer->line.":0: error: invalid code specification\n" unless defined($name);

        $info = \%{$errors[$currgroup]{'codes'}[$ndx++]};
        $info->{'name'} = $name;
        $info->{'desc'} = $desc;
        $info->{'num'} = $num;
        $info->{'sqlstate'} = $sqlstate;

        $errors[$currgroup]{'firstcode'} = $name unless exists $errors[$currgroup]{'firstcode'};
        $errors[$currgroup]{'lastcode'} = $name;
        $errors[$currgroup]{'endnum'} = $num;
        $len = length($info->{'name'});
        $maxcodelen = $len if $maxcodelen < $len;
    }
    elsif ($token->type eq 'SKIP')
    {
        ##Ignore the code */
    }
    else
    {
        die "$errsource:".$lexer->line.":0: error: group/code/skip/comment expected\n";
    }
}

# Open the output file, if any.
($jcodes, $tmpfile_jcodes) = tempfile();
($javas,  $tmpfile_javas) = tempfile();

print $jcodes $preamble_start;
print $jcodes " *  ".$javacodes_name.":  RaimaDB error information\n";
print $jcodes $preamble_end;


print $jcodes "
using System;
using System.Diagnostics.CodeAnalysis;

[module: SuppressMessage(\"Microsoft.Naming\", \"CA1702:CompoundWordsShouldBeCasedCorrectly\", Scope=\"member\", Target=\"Raima.Rdm.RsqlErrorCode.#errREADONLY\", MessageId=\"READONLY\")]
[module: SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", Scope=\"member\", Target=\"Raima.Rdm.RsqlErrorCode.#errDIVBY0\", MessageId=\"DIVBY\")]
namespace Raima.Rdm
{
    public enum RsqlErrorCode
    {
";

print $javas $preamble_start;
print $javas " *  ".$retjavas_name.":  RaimaDB error information\n";
print $javas $preamble_end;

print $javas "
using System;
using System.Collections;
using System.Data;
using System.Diagnostics.CodeAnalysis;

namespace Raima.Rdm.Sql
{
    public static class RsqlErrors
    {
        static private Hashtable errorTable;
        static private RsqlError notConnectedError;
        static private RsqlError cannotConnectError;

        [SuppressMessage(\"Microsoft.Performance\", \"CA1810:InitializeReferenceTypeStaticFieldsInline\")]
        static RsqlErrors()
        {
            errorTable = new Hashtable(254, (float) 1.0);
";

for my $error (@errors)
{
    for $info (@{$error->{'codes'}})
    {
        my $padlen = ($maxcodelen + 1) - length($info->{'name'});

        printf($jcodes "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"err\")]\n");
        printf($jcodes "        [SuppressMessage(\"Microsoft.Naming\", \"CA1709:IdentifiersShouldBeCasedCorrectly\", MessageId=\"%s\")]\n", $info->{'name'});
        printf($jcodes "        %s = %s,\n", $info->{'name'}, $info->{'num'});

        printf $javas "            errorTable.Add(RsqlErrorCode.%s, new RsqlError(", $info->{'name'};
        printf $javas "\"%s\", ", $info->{'name'};

        if (defined($info->{'sqlstate'}))
        {
            printf $javas "\"%s\", ", $info->{'sqlstate'};
        }
        else
        {
            printf $javas "null, ";
        }
        printf $javas "\"%s\"));\n", $info->{'desc'};
    }
}

print $javas "
            notConnectedError = findError(RsqlErrorCode.eDBNOTOPEN);
            cannotConnectError = findError(RsqlErrorCode.eTX_CONNECT);
        }

        static private RsqlError findError(RsqlErrorCode err)
        {
            return (RsqlError) errorTable[err];
        }

        static private void addError(RsqlErrorCode err, RsqlError error)
        {
            errorTable.Add(err, error);
        }

        static public RsqlError NotConnectedError
        {
            get { return notConnectedError; }
        }

        static public RsqlError CannotConnectError
        {
            get { return cannotConnectError; }
        }

        static public RsqlError GetError(RsqlErrorCode err)
        {
            RsqlError error;
 
            if ((error = findError(err)) == null)
                error = findError(RsqlErrorCode.eSQL_RDMERROR);

            return error;
        }

        static public RsqlError GetError(Rsql rsql, RsqlErrorCode err)
        {
            RsqlError error;

            if ((error = findError(err)) == null)
            {
                if ((error = rsql.GetRsqlError(err)) != null)
                    addError(err, error);
                else
                    error = findError(RsqlErrorCode.eSQL_RDMERROR);
            }

            return error;
        }

        static public RsqlError GetError(RsqlStatement rsqlStmt, RsqlErrorCode err)
        {
            RsqlError error;

            if ((error = findError(err)) == null)
            {
                if ((error = rsqlStmt.GetRsqlError(err)) != null)
                    addError(err, error);
                else
                    error = findError(RsqlErrorCode.eSQL_RDMERROR);
            }

            return error;
        }
    }
}
";

print $jcodes "    };
}";

close $jcodes;
close $javas;

chmod($lfrwmode, $errdest_javas) unless !(-e $errdest_javas) || (-w $errdest_javas);
copy($tmpfile_javas, $errdest_javas) || die "Can't copy $errdest_javas: $!\n";
chmod($lfrwmode, $errdest_jcodes) unless !(-e $errdest_jcodes) || (-w $errdest_jcodes);
copy($tmpfile_jcodes, $errdest_jcodes) || die "Can't copy $errdest_jcodes: $!\n";

unlink($tmpfile_javas);
unlink($tmpfile_jcodes);

sub get_group
{
    my $token;
    my $name;
    
    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'COMMA';

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'STRING';

    $name = $token->text;

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'COMMA';

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'NUMBER';

    return ($name, $token->text);
}

sub get_code
{
    my $token;
    my $num;
    my $name;
    my $desc;
    my $sqlstate;

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'COMMA';

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'NUMBER';

    $num = $token->text;

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'COMMA';

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'STRING';

    $name = $token->text;

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'COMMA';

    $token = $lexer->next;
    return $undef unless not $lexer->eoi and $token->type eq 'STRING';

    $desc = $token->text;
    
    $token = $lexer->next;
    if ($token->type eq 'COMMA')
    {
        $token = $lexer->next;
        return $undef unless not $lexer->eoi and $token->type eq 'STRING';

        $sqlstate = $token->text;
    }
    elsif (not $lexer->eoi and $token->type ne 'EOL')
    {
printf "token type = %s\n", $token->type;
        return $undef;
    }
    else
    {
        $sqlstate = $undef;
    }

    return ($num, $name, $desc, $sqlstate);
}

__END__

