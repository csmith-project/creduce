## -*- mode: Perl -*-

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
    ["\#(.*?)\n", ""],
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
    ["char", "int"],
    ["short", "int"],
    ["long", "int"],
    ["signed", "int"],
    ["unsigned", "int"],
    ["int(\\s+)argc(\\s*),(\\s*)(.*)(\\s*)\\*argv\\[\\]", "void"],
    ["int(\\s+)argc(\\s*),(\\s*)(.*)(\\s*)\\*(\\s*)\\*argv", "void"],
    ["int.*?;", ""],
    ["for", ""],
    ["\"(.*)\"", ""],
    ["\'(.*)\'", ""],
    ["\"(.*?)\"", ""],
    ["\'(.*?)\'", ""],
    ["$call,", "0"],
    ["$call,", ""],
    ["$call", "0"],
    ["$call", ""],
    );

1;
