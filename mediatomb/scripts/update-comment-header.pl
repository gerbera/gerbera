#!/usr/bin/perl

use strict;

foreach (@ARGV)
{
    my $full_path = $_;
#my $tmpfile = $_."tmp_update-comment-header";
    my @full_fn = split('/', $full_path);
    my $filename = pop(@full_fn);
    
    my $new_header_main = 
'    MediaTomb - http://www.mediatomb.org/
    
    '.$filename.' - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$';
    
    my $new_header_tombupnp = 
'    TombUPnP - a library for developing UPnP applications.
    
    Copyright (C) 2006 Sergey \'Jin\' Bostandzhyan <jin@mediatomb.org>
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA';
    
    my $new_header_c =
'/*MT*
    
'.$new_header_main.'
*/';

    my $new_header_sgml =
'<!--*MT*
    
'.$new_header_main.'
-->';
    
    my @new_header_main_array = split("\n", $new_header_main);
    my $new_header_main_number = join("\n#", @new_header_main_array);
    
    my $new_header_number = 
'#*MT*
#    
#'.$new_header_main_number.'
#
#*MT*';
    
    my $new_header_c_tombupnp =
'/*TU*
    
'.$new_header_tombupnp.'
*/';
    
    print $full_path.": ";
    open (FILE, '<', $full_path);
    my $data = join('', <FILE>);
    close FILE;
    
    my $has_file = ($data =~ m|/// *\\file|);
    my $add_file = ($filename =~ /\.(cc?|h)$/);
    if (! $has_file && $add_file)
    {
        $new_header_c .= "\n\n/// \\file $filename";
    }
    my $nothing = 0;
    if ($data =~ m|/\*MT\*|)
    {
        print "/*MT*...";
        $data =~ s|/\*MT\*(.*?\*)??/|$new_header_c|s;
    }
    elsif ($data =~ /<!--\*MT\*/)
    {
        print "<!--*MT*...";
        $data =~ s/<!--\*MT\*.*?->/$new_header_sgml/s;
    }
    elsif ($data =~ /#\*MT\*/)
    {
        print "#*MT*...";
        $data =~ s/#\*MT\*.*?#\*MT\*/$new_header_number/s;
    }
#    elsif ($data =~ m|/\*.*this file is part of MediaTomb\.|)
#    {
#        print "old header";
#        $data =~ s|/\*.*this file is part of MediaTomb\..*?\*/|$new_header_c|s;
#    }
    elsif ($data =~ m|/\*TU\*|)
    {
        print "/*TU*...";
        $data =~ s|/\*TU\*(.*?\*)??/|$new_header_c_tombupnp|s;
    }
    else
    {
        $nothing = 1;
        print "nothing!";
    }
    
    if (! $nothing)
    {
        if ($has_file)
        {
            $data =~ s|/// *\\file +.*$|/// \\file $filename|m;
            print " and had file";
        }
        open (FILE, '>', $full_path);
        print FILE $data;
#        print $data;
        close FILE;
    }
    print "\n";
}

