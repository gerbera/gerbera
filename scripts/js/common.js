// This script will be run once before each other script is loaded. Here you
// can define any functions you want to have available in the other scripts,
// or initialisation code you want to have executed only once for each script.

/*MT_F*
    
    MediaTomb - http://www.mediatomb.cc/
    
    common.js - this file is part of MediaTomb.
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    This file is free software; the copyright owners give unlimited permission
    to copy and/or redistribute it; with or without modifications, as long as
    this notice is preserved.
    
    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    
    $Id$
*/

function escapeSlash(name)
{
    name = name.replace(/\\/g, "\\\\");
    name = name.replace(/\//g, "\\/");
    return name;
}

function createContainerChain(arr)
{
    var path = '';
    for (var i = 0; i < arr.length; i++)
    {
        path = path + '/' + escapeSlash(arr[i]);
    }
    return path;
}

function getYear(date)
{
    var matches = date.match(/^([0-9]{4})-/);
    if (matches)
        return matches[1];
    else
        return date;
}

function getPlaylistType(mimetype)
{
    if (mimetype == 'audio/x-mpegurl')
        return 'm3u';
    if (mimetype == 'audio/x-scpls')
        return 'pls';
    return '';
}

function getLastPath(location)
{
    var path = location.split('/');
    if ((path.length > 1) && (path[path.length - 2]))
        return path[path.length - 2];
    else
        return '';
}

function getRootPath(rootpath, location)
{
    var path = new Array();

    if (rootpath.length != '')
    {
        rootpath = rootpath.substring(0, rootpath.lastIndexOf('/'));

        var dir = location.substring(rootpath.length,location.lastIndexOf('/'));

        if (dir.charAt(0) == '/')
            dir = dir.substring(1);

        path = dir.split('/');
    }
    else
    {
        dir = getLastPath(location);
        if (dir != '')
        {
            dir = escapeSlash(dir);
            path.push(dir);
        }
    }

    return path;
}
