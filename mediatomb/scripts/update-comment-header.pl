#!/usr/bin/perl

use strict;

foreach (@ARGV)
{
    my $full_path = $_;
#my $tmpfile = $_."tmp_update-comment-header";
    my @full_fn = split('/', $full_path);
    my $filename = pop(@full_fn);
    my $id_keyword = '$'.'Id'.'$';
    
    $filename =~ s/\.sql\.tmpl\.h/_create_sql\.h/;
    my $current = '2010';
    
    my $new_header_main = 
'    MediaTomb - http://www.mediatomb.cc/
    
    '.$filename.' - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey \'Jin\' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-'.$current.' Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey \'Jin\' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    '.$id_keyword;
    
    my $new_header_tombupnp = 
'    TombUPnP - a library for developing UPnP applications.
    
    Copyright (C) 2006-'.$current.' Sergey \'Jin\' Bostandzhyan <jin@mediatomb.cc>
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    '.$id_keyword;
    
    my $new_header_free = 
'    MediaTomb - http://www.mediatomb.cc/
    
    '.$filename.' - this file is part of MediaTomb.
    
    Copyright (C) 2006-'.$current.' Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey \'Jin\' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    This file is free software; the copyright owners give unlimited permission
    to copy and/or redistribute it; with or without modifications, as long as
    this notice is preserved.
    
    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    
    '.$id_keyword;

    my $new_header_jan =
'    MediaTomb - http://www.mediatomb.cc/
    
    '.$filename.' - this file is part of MediaTomb.
    
    Copyright (C) 2007-2008 Jan Habermann <jan.habermann@gmail.com>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    '.$id_keyword;

    my $new_header_c =
'/*MT*
    
'.$new_header_main.'
*/';
    
    my $new_header_free_c =
'/*MT_F*
    
'.$new_header_free.'
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
    

    my $new_header_jan_c = 
'/*MT_J*
    
'.$new_header_jan.'
*/';

my $new_header_jan_sgml =
'<!--*MT_J*
    
'.$new_header_jan.'
-->';


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
    elsif ($data =~ m|/\*MT_F\*|)
    {
        print "/*MT_F*...";
        $data =~ s|/\*MT_F\*(.*?\*)??/|$new_header_free_c|s;
    }
    elsif ($data =~ m|/\*MT_J\*|)
    {
        print "/*MT_J*...";
        $data =~ s|/\*MT_J\*(.*?\*)??/|$new_header_jan_c|s;
    }
    elsif ($data =~ /<!--\*MT_J\*/)
    {
        print "<!--*MT_J*...";
        $data =~ s/<!--\*MT_J\*.*?->/$new_header_jan_sgml/s;
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
