#!/usr/bin/perl

#
# bin2hex.pl by Chami.com
# http://www.chami.com/tips/
#

# number of characters per line
$chars_per_line = 15;

# -------------------------------------

# language id
#
# 0 = Perl (default)
# 1 = C / C++
# 2 = Pascal / Delphi
#
$lang  = $ARGV[1];

$rem_begin  = "begin binary data:";
$rem_end    = "end binary data.";

# initialize for Perl strings
# by default
$_var       = "# $rem_begin\n".
              "$bin_data = # %d}n";
$_begin     = "\"";
$_end       = "\";}\n";
$_break     = "\".\n\"";
$_format    = "\\x%02X";
$_separator = "";
$_comment   = "# $rem_end ".
              "size = %d bytes";


# C / C++
if(1 == $lang)
{
  $_var       = "/* $rem_begin */\n".
                "char bin_data[] = ".
                "/* %d */\n";
  $_begin     = "{";
  $_end       = "};\n";
  $_break     = "\n";
  $_format    = "0x%02X";
  $_separator = ",";
  $_comment   = "/* $rem_end ".
                "size = %d bytes */";
}
elsif(2 == $lang)
{
  $_var       = "{ $rem_begin }\n".
                "const bin_data : ".
                "array [1..%d] of ".
                "byte = \n";
  $_begin     = "(";
  $_end       = ");\n";
  $_break     = "\n";
  $_format    = "$%02X";
  $_separator = ",";
  $_comment   = "{ $rem_end ".
                "size = %d bytes }";
}

if(open(F, "<".$ARGV[0]))
{
  binmode(F);

  $s = '';
  $i = 0;
  $count = 0;
  $first = 1;
  $s .= $_begin;
  while(!eof(F))
  {
    if($i >= $chars_per_line)
    {
      $s .= $_break;
      $i = 0;
    }
    if(!$first)
    {
      $s .= $_separator;
    }
    $s .= sprintf(
            $_format, ord(getc(F)));
    ++$i;
    ++$count;
    $first = 0;
  }
  $s .= $_end;
  $s .= sprintf $_comment, $count;
  $s .= "\n\n";

  $s = "\n".sprintf($_var, $count).$s;

  print $s;

  close( F );
}
else
{
  print
    "bin2hex.pl by Chami.com\n".
    "\n".
    "usage:\n".
    "  perl bin2hex.pl <binary file>".
    " <language id>\n".
    "\n".
    "  <binary file> : path to the ".
    "binary file\n".
    "  <language id> : 0 = Perl, ".
    "1 = C/C++/Java, ".
    "2 = Pascal/Delphi\n".
    "\n";
}

