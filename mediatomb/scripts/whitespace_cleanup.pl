#!/usr/bin/perl

use strict;

foreach (@ARGV)
{
    my $full_path = $_;
    #my @full_fn = split('/', $full_path);
    #my $filename = pop(@full_fn);
    
    print $full_path.": ";
    open (FILE, '<', $full_path);
    my $data = join('', <FILE>);
    close FILE;
    
    my $last_chars = substr($data, -2);
    #print "x-".$last_chars."-x";
    
    my $modified = 0;
    
    if ($last_chars =~ m|^[^\n]\n$|)
    {
        print "-";
    }
    else
    {
        if ($last_chars eq "\n\n")
        {
            print "!!!!!!";
            #$data =~ s|\n+$|\n|s;
            #$modified = 1;
        }
        else
        {
            print "!-!-!-!-!-!";
        }
    }
    
    $last_chars = substr($data, -2);
    #print "y-".$last_chars."-y";
    
    if ($modified)
    {
        open (FILE, '>', $full_path);
        print FILE $data;
#       print $data;
        close FILE;
    }
    print "\n";
}
