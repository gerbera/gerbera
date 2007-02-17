#!/usr/bin/perl

while (<>)
{
    s/━/-/g;
    s/●/*/g;
    s/○/o/g;
    s/□/-/g;
    s/“/"/g;
    s/”/"/g;
    s/ / /g;
    
    print;
}
