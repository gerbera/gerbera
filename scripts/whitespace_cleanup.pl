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

    if (length($data) == 0)
    {
	print "0\n";
	next;
    }

    my $last_chars = substr($data, -2);
    #print "x-".$last_chars."-x";
    
    my $modified = 0;
    
    if ($last_chars =~ m|^[^\n]\n|s)
    {
        print "-";
    }
    else
    {
        if ($last_chars eq "\n\n")
        {
            print "!!!!!!";
            $data =~ s|\n+$|\n|s;
            $modified = 1;
        }
        else
        {
            print "!-!-!-!-!-!";
        }
    }
    
    if ($modified)
    {
	#$last_chars = substr($data, -2);
	printf("y-%x %x-y",ord(substr($data,-2,1)), ord(substr($data,-1)));
    }
    
    if (0) #tab-cleanup
    {
	my $data_before = $data;
	
	$data =~ s|\t|    |g;
    
	if ($data_before ne $data)
	{
	    print "TAB";
	    $modified = 1;
	}
    }
    
    # off for safety reasons
    $modified = 0;

    if ($modified)
    {
        open (FILE, '>', $full_path);
        print FILE $data;
#       print $data;
        close FILE;
    }
    print "\n";
}
