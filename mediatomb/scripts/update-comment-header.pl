#!/usr/bin/perl

use strict;

foreach (@ARGV)
{
    my $full_path = $_;
#my $tmpfile = $_."tmp_update-comment-header";
    my @full_fn = split('/', $full_path);
    my $filename = pop(@full_fn);
    
    my $new_header = 
'/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    '.$filename.' - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/';
#    print $full_path;
    open (FILE, '<', $full_path);
    my $data = join('', <FILE>);
    close FILE;
    open (FILE, '>', $full_path);
    
    $data =~ s/\/\*.*this file is part of MediaTomb\..*?\*\//$new_header/s;
    #$data =~ s/MediaTomb/test/;
    
    print FILE $data;
    close FILE;
}
