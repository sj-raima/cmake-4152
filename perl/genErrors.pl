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
$retcodes_name = 'rdmretcodetypes.h';
$rettables_name = 'rettables.h';
$errsource = $RDM_HOME.'/source/base/errordefns.txt';

$errdest_grep_retcode = $RDM_HOME.'/perl/grep-retcode';
$errdest_return_code_err_h = $RDM_HOME.'/source/psp/return_code_err.h';
$errdest_return_code_err_sed = $RDM_HOME.'/perl/return_code_err.sed';
$errdest_codes = $RDM_HOME.'/include/'.$retcodes_name;
$errdest_tables = $RDM_HOME.'/source/base/'.$rettables_name;

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
my $numGen;
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
        
        if ($currgroup != -1)
        {
            die "$errsource:".$lexer->line.":0: error: invalid group initial code: must be less than the last code\n" unless $startnum < $errors[$currgroup]->{'endnum'};
            die "$errsource:".$lexer->line.":0: error: invalid group initial code: overlaps with last group code assignment\n" unless $startnum <= $numGen;
        }

        $incr = 1;
        $incr = -1 if $startnum < 0;

        $errors[++$currgroup]{'name'} = $groupname;
        $errors[$currgroup]{'startnum'} = $startnum;
        $numGen = $startnum;
        $ndx = 0;
        $len = length($groupname);
        $maxgrouplen = $len if $maxgrouplen < $len;
    }
    elsif ($token->type eq 'CODE')
    {
        my ($num, $name, $desc, $sqlstate) = get_code();
        die "$errsource:".$lexer->line.":0: error: invalid code specification\n" unless defined($name);
        die "$errsource:".$lexer->line.":0: error: invalid number, a SKIP may be needed\n" unless $num == $numGen || $name eq 'eNOTIMPLEMENTED_MIN';

        $info = \%{$errors[$currgroup]{'codes'}[$ndx++]};
        $info->{'name'} = $name;
        $info->{'desc'} = $desc;
        $info->{'num'} = $num;
        $info->{'sqlstate'} = $sqlstate;

        $errors[$currgroup]{'firstcode'} = $name unless exists $errors[$currgroup]{'firstcode'};
        $errors[$currgroup]{'lastcode'} = $name;
        $errors[$currgroup]{'endnum'} = $num;
        $numGen += $incr;
        $len = length($info->{'name'});
        $maxcodelen = $len if $maxcodelen < $len;
    }
    elsif ($token->type eq 'SKIP')
    {
        ##Ignore the code but increment past the defunct code */
        $numGen += $incr;
    }
    else
    {
        die "$errsource:".$lexer->line.":0: error: group/code/skip/comment expected\n";
    }
}

# Open the output file, if any.
($grep_retcode,  $tmpfile_grep_retcode) = tempfile();
($return_code_err_h,  $tmpfile_return_code_err_h) = tempfile();
($return_code_err_sed,  $tmpfile_return_code_err_sed) = tempfile();
($tables, $tmpfile_tables) = tempfile();
($codes,  $tmpfile_codes) = tempfile();

print $grep_retcode
'#!/bin/bash
#
# Use this script to find RDM_RETCODE enum codes

git grep -E --line-number \'';

my $first='t';
for my $error (@errors)
{
    for $info (@{$error->{'codes'}})
    {
        if ($first)
        {
            $first ='';
        }
        else
        {
            printf $grep_retcode "|";
        }
        printf $grep_retcode '\\b%s\\b', $info->{'name'};
    }
}

print $grep_retcode '\'
git grep -E --line-number \'\\bRDM_RETCODE\\b\'
';

for my $error (@errors)
{
    for $info (@{$error->{'codes'}})
    {
        printf $grep_retcode "git grep -E --line-number \'\\b%s\\b\'\n", $info->{'name'};
    }
}

close $grep_retcode;

print $return_code_err_h
'/*
 * Raima Database Manager
 *
 * Copyright (C) 2023 Raima Inc.,  All rights reserved.
 *
 * Use of this software, whether in source code format, or in executable,
 * binary object code form, is governed by the Raima LICENSE which
 * is fully described in the LICENSE.TXT file, included within this
 * distribution of files.
 */

/** \file return_code.cpp
 *  \brief
 */

#include "return_code.h"
#if !defined(RETURN_CODE_ERR_H)
#define RETURN_CODE_ERR_H
';
for my $error (@errors)
{
    printf $return_code_err_h "\n/* %s */\n", $error->{'name'};
    for $info (@{$error->{'codes'}})
    {
        printf $return_code_err_h "\#define i%s RDM::PSP::RETURN_CODE (%s, __FILE__, __LINE__, __FUNCTION__)\n", $info->{'name'}, $info->{'name'};
    }
}

print $return_code_err_h "\n#endif /* RETURN_CODE_ERR_H */\n";

close $return_code_err_h;

print $return_code_err_sed
'# Use this sed script to rename RDM_RETCODE enums to RDM_RETURN_CODE defines,
# Process a source file as follows:
#
#   sed -i -f /home/sj/turin/perl/return_code.sed sourcefile
#
#
';

for my $error (@errors)
{
    printf $return_code_err_sed "\n# %s\n", $error->{'name'};
    for $info (@{$error->{'codes'}})
    {
        printf $return_code_err_sed "s/\\b%s\\b/i%s/g\n", $info->{'name'}, $info->{'name'};
    }
}

print $return_code_err_sed '
# RDM_RETCODE
s/\bRDM_RETCODE\b/RDM::PSP::RETURN_CODE/g
#s/static[ ]RDM_RETCODE\b/static RDM::PSP::RETURN_CODE/g
#s/\([ ][ ][ ][ ]\+\)RDM_RETCODE\b/\1RDM::PSP::RETURN_CODE/g
s/\b\(RDM[:][:]PSP[:][:]RETURN_CODE\b[ ]\+[a-z_A-Z0-9]\+[ ]\+\)i/\1= i/g
s/\([#]include[ ]\+\)["]rdmretcodetypes[.]h["]/\1"..\/psp\/return_code_err.h"/g
';

close $return_code_err_sed;

print $tables $preamble_start;
print $tables " *  $rettables_name:  RaimaDB error information\n";
print $tables $preamble_end;
print $tables
"#if !defined(RETTABLES_H_INCLUDED_)
#define RETTABLES_H_INCLUDED_

#include \"rdmretcodetypes.h\"
#include \"psptypes.h\"

typedef struct
{
    const char       *name;
    const RDM_RETCODE firstcode;
    const RDM_RETCODE lastcode;
} RDM_ERRGROUP;

typedef struct
{
    const char       *name;
    const RDM_RETCODE code;
    const char       *sqlstate;
    const char       *descr;
} RDM_ERRDEFN;

static const RDM_ERRGROUP rdm_errgroups[] = {
";

for my $error (@errors)
{
    my $padlen = ($maxgrouplen + 1) - length($error->{'name'});
    my $padlen2 = ($maxcodelen + 1) - length($error->{'firstcode'});
    printf $tables "    {\"%s\",%*s%s,%*s%s},\n", $error->{'name'}, $padlen,
            ' ', $error->{'firstcode'}, $padlen2, ' ', $error->{'lastcode'};
}

print $tables
"};

static const uint32_t no_rdm_errgroups = (uint32_t) (sizeof(rdm_errgroups)/sizeof(RDM_ERRGROUP));

static const RDM_ERRDEFN rdm_errors[] = {";

print $codes $preamble_start;
print $codes " *  ".$retcodes_name.":  RaimaDB error information\n";
print $codes $preamble_end;
print $codes
"
/** \\file rdmretcodetypes.h
 *
 *  \\brief RaimaDB Status and Error Return Codes.
 *
 *  \\defgroup retcodes RaimaDB Return Codes
 *
 *  The RaimaDB Return codes.
 *
 *  \\ingroup rdm
 *
 *  \\addtogroup retcodes
 *  \@{
 */

#ifndef RDMRETCODETYPES_H_INCLUDED_
#define RDMRETCODETYPES_H_INCLUDED_

/**
 *  \\brief RaimaDB status and error return codes.
 */
typedef enum RDM_RETCODE_E
{";

for my $error (@errors)
{
    printf $tables "\n    /* ".$error->{'name'}." codes: */\n";
    printf $codes  "\n    /* ".$error->{'name'}." codes: */\n";

    for $info (@{$error->{'codes'}})
    {
        my $padlen = ($maxcodelen + 1) - length($info->{'name'});
        printf $tables "    {\"%s\",%*s%s %*s/*%7d*/, ", $info->{'name'},
                $padlen, ' ', $info->{'name'}, $padlen, ' ', $info->{'num'};
        if (defined($info->{'sqlstate'}))
        {
            printf $tables "\"%s\", ", $info->{'sqlstate'};
        }
        else
        {
            printf $tables "NULL,    ";
        }
        printf $tables "\"%s\"},\n", $info->{'desc'};
        printf $codes  "    %-*s  = %7d%s /**< %s. */\n", $maxcodelen,
                $info->{'name'}, $info->{'num'}, 
                ($error == $errors[-1] && $info == ${$error->{'codes'}}[-1]) ? "" : ",",
                $info->{'desc'};
    }
}

print $tables
"};

static const uint32_t no_rdm_errors = (uint32_t) (sizeof(rdm_errors)/sizeof(RDM_ERRDEFN));

#endif /* RETTABLES_H_INCLUDED_ */
";

close $tables;

print $codes
"} RDM_RETCODE;

#define RDM_IS_ERROR(c) ((int32_t) (c) < 0 ? 1 : 0) /**< Is the return code an error code */
#define RDM_IS_INFO(c) ((int32_t) (c) >= 0 ? 1 : 0) /**< Is the return code an info code */
#define RDM_IS_NOTIMPLEMENTED(c) (c > eNOTIMPLEMENTED_MIN && c < eNOTIMPLEMENTED_MAX)
/**
 *  \@}
 */

#endif  /* RDMRETCODETYPES_H_INCLUDED_ */
";

close $codes;

chmod($lfrwxmode, $errdest_grep_retcode) unless !(-e $errdest_grep_retcode) || (-x $errdest_grep_retcode);
copy($tmpfile_grep_retcode, $errdest_grep_retcode) || die "Can't copy $errdest_grep_retcode: $!\n";

chmod($lfrwmode, $errdest_return_code_err_h) unless !(-e $errdest_return_code_err_h) || (-w $errdest_return_code_err_h);
copy($tmpfile_return_code_err_h, $errdest_return_code_err_h) || die "Can't copy $errdest_return_code_err_h: $!\n";

chmod($lfrwmode, $errdest_return_code_err_sed) unless !(-e $errdest_return_code_err_sed) || (-w $errdest_return_code_err_sed);
copy($tmpfile_return_code_err_sed, $errdest_return_code_err_sed) || die "Can't copy $errdest_return_code_err_sed: $!\n";

chmod($lfrwmode, $errdest_codes) unless !(-e $errdest_codes) || (-w $errdest_codes);
copy($tmpfile_codes, $errdest_codes) || die "Can't copy $errdest_codes: $!\n";

chmod($lfrwmode, $errdest_tables) unless !(-e $errdest_tables) || (-w $errdest_tables);
copy($tmpfile_tables, $errdest_tables) || die "Can't copy $errdest_tables: $!\n";

unlink($tmpfile_grep_retcode);
unlink($tmpfile_return_code_err_h);
unlink($tmpfile_return_code_err_sed);
unlink($tmpfile_codes);
unlink($tmpfile_tables);

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

