/*GRB*
  Gerbera - https://gerbera.io/

  metadata.js - this file is part of Gerbera.

  Copyright (C) 2022-2026 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/

function importMetadata(meta, cont, rootPath, autoscanId, containerType) {
  const arr = rootPath.split('.');
  print2("Info", "Processing metafile: " + rootPath + " for " + meta.location + " " + arr[arr.length - 1].toLowerCase());
  var result = [];
  switch (arr[arr.length - 1].toLowerCase()) {
    case "nfo":
      parseNfo(meta, rootPath);
      updateCdsObject(meta);
      result.push(meta.id);
      break;
  }
  return result;
}

function parseNfo(obj, nfo_file_name) {
  var node = readXml(-2);
  var level = 0;
  var isActor = false;
  var isSeries = false;

  while (node || level > 0) {
    if (!node && level > 0) {
      node = readXml(-1); // read parent
      node = readXml(0); // read next
      isActor = false;
      level--;
    } else if (node.NAME === "movie") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_MOVIE;
      level++;
    } else if (node.NAME === "musicvideo") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_MUSICVIDEOCLIP;
      level++;
    } else if (node.NAME === "tvshow") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_BROADCAST;
      level++;
    } else if (node.NAME === "episodedetails") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_BROADCAST;
      isSeries = true;
      level++;
    } else if (node.NAME === "actor") {
      node = readXml(1); // read children
      isActor = true;
      level++;
    } else if (node.NAME == "title") {
      if (isSeries)
        addMeta(obj, "upnp:programTitle", node.VALUE);
      else
        addMeta(obj, M_TITLE, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "showtitle") {
      addMeta(obj, "upnp:seriesTitle", node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "sorttitle") {
      obj.sortKey = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "plot") {
      obj.description = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "name" && isActor) {
      addMeta(obj, M_ACTOR, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "genre") {
      addMeta(obj, M_GENRE, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "premiered") {
      addMeta(obj, M_DATE, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "season") {
      addMeta(obj, M_PARTNUMBER, node.VALUE);
      obj.partNumber = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "episode") {
      addMeta(obj, "upnp:episodeNumber", node.VALUE);
      obj.trackNumber = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "mpaa" && node.VALUE) {
      addMeta(obj, "upnp:rating", node.VALUE);
      addMeta(obj, "upnp:rating@type", "MPAA.ORG");
      addMeta(obj, "upnp:rating@equivalentAge", node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "country") {
      addMeta(obj, M_REGION, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "studio") {
      addMeta(obj, M_PUBLISHER, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "director") {
      addMeta(obj, M_DIRECTOR, node.VALUE);
      node = readXml(0); // read next
    } else if (level === 1 && node.VALUE !== '') {
      addAux(obj, "NFO:" + node.NAME, node.VALUE);
      node = readXml(0); // read next
    } else {
      node = readXml(0); // read next
    }
  }
}
