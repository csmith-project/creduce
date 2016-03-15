## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package creduce_regexes;

use warnings;

use Regexp::Common;
use re 'eval';
use Exporter::Lite;

@EXPORT      = qw($fullvar $binop @delimited_regexes_to_replace
                  @regexes_to_replace $borderorspc $varnumexp);

$varnum = "(\\-?|\\+?)[0-9a-zA-Z\_]+";
$varnumexp = "($varnum)|($RE{balanced}{-parens=>'()'})";
$field = "\\.($varnum)";
$index = "\\\[($varnum)\\\]";
$fullvar = "([\\&\\*]*)($varnumexp)(($field)|($index))*";
$arith = "\\+|\\-|\\%|\\/|\\*";
$comp = "\\<\\=|\\>\\=|\\<|\\>|\\=\\=|\\!\\=|\\=";
$logic = "\\&\\&|\\|\\|";
$bit = "\\||\\&|\\^|\\<\\<|\\>\\>";
$binop = "($arith)|($comp)|($logic)|($bit)|(\\-\\>)";
$border = "[\\*\\{\\(\\[\\:\\,\\}\\)\\]\\;\\,]";
$borderorspc = "(($border)|(\\s))";
$fname = "(?<fname>$varnum)";
$call = "$varnum\\s*$RE{balanced}{-parens=>'()'}";

# these match without additional qualification
@regexes_to_replace = (
    ["$RE{balanced}{-parens=>'<>'}", ""],
    ["$RE{balanced}{-parens=>'()'}", ""],
    ["$RE{balanced}{-parens=>'{}'}", ""],
    ["namespace(.*?)$RE{balanced}{-parens=>'{}'}", ""],
    ["=\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["\\:\\s*[0-9]+\\s*;", ";"],
    ["\\;", ""],
    ["\\^\\=", "="],
    ["\\|\\=", "="],
    ["\\+\\=\\s*1", "++"],
    ["\\-\\=\\s*1", "--"],
    ["\\&\\=", "="],
    ["\"(.*)\"", ""],
    ["checksum = ", ""],
    ["\'(.*)\'", ""],
    ["\\+\\=", "="],
    ["\\-\\=", "="],
    ["\\*\\=", "="],
    ["\\/\\=", "="],
    ["\\%\\=", "="],
    ["\\<\\<\\=", "="],
    ["\\>\\>\\=", "="],
    ["\\+", ""],
    ["\\-", ""],
    [":", ""],
    [",", ""],
    ["::", ""],
    ["\\!", ""],
    ["\\~", ""],
    ["while", "if"],
    ['"(.*?)"', ""],
    ['"(.*?)",', ""],
    ["struct\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["union\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["enum\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["if\\s*$RE{balanced}{-parens=>'()'}", ""],
    );

# these match when preceded and followed by $borderorspc
@delimited_regexes_to_replace = (
    ["($varnumexp)\\s*:", ""],
    ["goto\\s+($varnum);", ""],
    ["class", "struct"],
    ["char", "int"],
    ["short", "int"],
    ["long", "int"],
    ["signed", "int"],
    ["unsigned", "int"],
    ["float", "int"],
    ["double", "int"],
    ["wchar_t", "int"],
    ["bool", "int"],
    ["typeof\\s+($varnum)", "int"],
    ["__typeof__\\s+($varnum)", "int"],
    ["int(\\s+)argc(\\s*),(\\s*)(.*)(\\s*)\\*argv\\[\\]", "void"],
    ["int(\\s+)argc(\\s*),(\\s*)(.*)(\\s*)\\*(\\s*)\\*argv", "void"],
    ["int.*?;", ""],
    ["for", ""],
    ["\"(.*)\"", ""],
    ["\'(.*)\'", ""],
    ["\"(.*?)\"", ""],
    ["\'(.*?)\'", ""],
    ["\"\"", "0"],
    ["$call,", "0"],
    ["$call,", ""],
    ["$call", "0"],
    ["$call", ""],
    );

1;
