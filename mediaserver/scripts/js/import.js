function copyObject(obj)
{
    var key;
	var copy = new Object();
	for (key in obj)
	{
		copy[key] = obj[key];
	}
    var meta = obj['meta'];
    if (meta)
    {
        meta_copy = new Object();
	    for (key in meta)
	    {
		    meta_copy[key] = meta[key];
	    }
        copy.meta = meta_copy;
    }
	return obj;
}
function splitPath(path)
{
	var start = 0;
	var end = path.length;
	if (path[0] == '/')
		start++;
	if (path[end - 1] == '/')
		end--;
	var str = path.substring(start, end);
	return str.split('/');
}
function getFileName(path)
{
	if (path[path.length - 1] == '/')
		return null;
	var i = path.lastIndexOf('/');
	return path.substring(i+1);
}
function getFilePath(path)
{
	if (path[path.length - 1] == '/')
		return path.substring(0, path.length - 1);
	var i = path.lastIndexOf('/');
	return path.substring(0, i);
}

function concatArrays(arr1, arr2)
{
	var arr = new Array();
	var i;
	for(i = 0; i < arr1.length; i++)
		arr[arr.length] = arr1[i];
	for(i = 0; i < arr2.length; i++)
		arr[arr.length] = arr2[i];
	return arr;
}

function addContainerChain(parentID, arr)
{
	for (var i = 0; i < arr.length; i++)
	{
		var title = arr[i];
		var cont = new Object();
		cont.objectType = OBJECT_TYPE_CONTAINER;
		cont.parentID = parentID;
		cont.title = title;
		cont.upnp_class = 'object.container';
		parentID = addCdsObject(cont);
	}
	return parentID;
}

var arr = orig.mimetype.split("/");
var mime = arr[0];

if (mime == 'audio')
{
	var obj = copyObject(orig);

	var chain = new Array();
	chain[chain.length] = 'Audio';

    var artist = obj.meta[M_ARTIST];
    if (artist)
        chain[chain.length] = artist;

    var album = obj.meta[M_ALBUM];
    if (album)
        chain[chain.length] = album;

    var title = obj.meta[M_TITLE];
    if (title)
        obj.title = title;

	var parentID = addContainerChain("0", chain);
	obj.parentID = parentID;
	var id = addCdsObject(obj);


	chain = new Array('Audio', 'All audio');
    if (artist)
        obj.title = artist + " - " + obj.title;

	var parentID = addContainerChain("0", chain);
	obj.parentID = parentID;
	var id = addCdsObject(obj);

    print('object created, ID: '+ id);
}


