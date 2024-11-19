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
$javacodes_name = 'RSQLErrorCode.java';
$retjavas_name = 'RSQLErrors.java';

$errsource = $RDM_HOME.'/source/base/errordefns.txt';
$errdest_jcodes = $RDM_HOME.'/source/jdbc/java/'.$javacodes_name;
$errdest_javas = $RDM_HOME.'/source/jdbc/java/'.$retjavas_name;

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

print $jcodes "package com.raima.rdm.jdbc;

public class RSQLErrorCode
{
";

print $javas $preamble_start;
print $javas " *  ".$retjavas_name.":  RaimaDB error information\n";
print $javas $preamble_end;

print $javas "package com.raima.rdm.sql;

import java.util.Hashtable;
import com.raima.rdm.util.RDMException;
import com.raima.rdm.jdbc.RSQLErrorCode;

public class RSQLErrors
{
    static private Hashtable<Integer, RSQLError> errorTable;

    static
    {
        errorTable = new Hashtable<Integer, RSQLError>(254, (float) 1.0);

";

for my $error (@errors)
{
    for $info (@{$error->{'codes'}})
    {
        my $padlen = ($maxcodelen + 1) - length($info->{'name'});

        printf $jcodes "    public static final int %s = %6d;\n", $info->{'name'}, $info->{'num'};

        printf $javas "        errorTable.put(RSQLErrorCode.%s, new RSQLError(", $info->{'name'};
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

print $javas
"    }

    static private synchronized RSQLError findError(int err)
    {
        return errorTable.get(err);
    }

    static private synchronized void addError(int err, RSQLError error)
    {
        errorTable.put(err, error);
    }
    static protected RSQLError getNotConnectedError()
    {
        return findError(RSQLErrorCode.eDBNOTOPEN);
    }

    static public RSQLError getCannotConnectError()
    {
        return findError(RSQLErrorCode.eTX_CONNECT);
    }

    static protected RSQLError getError(int err)
    {
        RSQLError error = findError(err);
        if (error == null)
        {
            error = findError(RSQLErrorCode.eSQL_RDMERROR);
        }

        return error;
    }

    static protected RSQLError getError(RSQL rsql, int err) throws RDMException
    {
        RSQLError error = findError(err);

        if (error == null)
        {
            error = rsql.getErrorMsg(err);
            if (error != null)
                addError(err, error);
            else
                error = findError(RSQLErrorCode.eSQL_RDMERROR);
        }

        return error;
    }

    static protected RSQLError getError(RSQLStatement rsqlStmt, int err) throws RDMException
    {
        RSQLError error;

        error = findError(err);
        if (error == null)
        {
            error = rsqlStmt.getErrorMsg(err);
            if (error != null)
                addError(err, error);
            else
                error = findError(RSQLErrorCode.eSQL_RDMERROR);
        }

        return error;
    }
}
";

print $jcodes "}";

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

