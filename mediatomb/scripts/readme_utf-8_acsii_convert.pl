#!/usr/bin/perl

use Text::Iconv;

my $converter = Text::Iconv->new("UTF-8", "ASCII");

while (<>)
{
    s/━/-/g;
    s/●/*/g;
    s/○/o/g;
    s/□/-/g;
    s/“/"/g;
    s/”/"/g;
    s/ / /g;
    
    my $converted = $converter->convert($_);
    if ($converted)
    {
        print $converted;
    }
    else
    {
        print STDERR  "$0: cannot convert (line $.)\n";
        exit 1;
    }
    #print;
}
