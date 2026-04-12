/*GRB*
  Gerbera - https://gerbera.io/

  cuesheet.js - this file is part of Gerbera.

  Copyright (C) 2026 Gerbera Contributors

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

function importCuesheet(cue, cont, rootPath, autoscanId, containerType) {
  const arr = cue.location.split('.');
  print2("Info", "Processing cuesheet: " + cue.location + " " + arr[arr.length - 1].toLowerCase());
  var result = [];
  switch (arr[arr.length - 1].toLowerCase()) {
    case "cue":
      parseCue(cue, cont, rootPath);
      break;
  }
  return result;
}

const reCuePerformer = new RegExp("^performer\\s+\"(.+)\"", 'i');
const reCueDiscTitle = new RegExp("^title\\s+\"(.+)\"", 'i');
const reCueFile = new RegExp("^file\\s+\"(.+)\"\\s+(\\S.+)", 'i');
const reCueTrack = new RegExp("^\\s+track\\s+(\\d+)\\s+(\\S+)", 'i');
const reCueTrackTitle = new RegExp("^\\s+title \"(.+)\"", 'i');
const reCueTrackIndex = new RegExp("^\\s+index\\s+(\\d+)\\s+(\\S.+)", 'i');

function parseCue(cue, parent, rootPath) {
  var line = readln();
  const result = [];
  var entry = {
    performer: null,
    discTitle: null,
    fileName: null,
    format: null,
    track: null,
    type: null,
    trackTitle: null,
    count: null,
    offset: null,
  };
  var parentId = -1;
  var container = JSON.parse(JSON.stringify(parent)); // create a copy
  container.entryType = "ExtraDirectory";
  container.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ALBUM;
  container.id = -1;
  container.refID = cue.id;
  container.searchable = false;
  do {
    if (reCuePerformer.test(line)) {
      const matches = reCuePerformer.exec(line);
      entry.performer = matches[1];
      container.metaData[M_ALBUMARTIST] = entry.performer;
      parentId = -1;
    } else if (reCueDiscTitle.test(line)) {
      const matches = reCueDiscTitle.exec(line);
      entry.discTitle = matches[1];
      container.title = entry.discTitle;
      parentId = -1;
    } else if (reCueFile.test(line)) {
      const matches = reCueFile.exec(line);
      if (entry.fileName && entry.track) {
          if (parentId === -1)
            parentId = addContainerTree([container], parent.id);
          result.push(parentId);
          result.push(storeCueEntry(entry, container, parentId, cue, rootPath));
          entry.track = null;
          entry.type = null;
      }
      entry.fileName = matches[1];
      entry.format = matches[2];
    } else if (reCueTrack.test(line)) {
      const matches = reCueTrack.exec(line);
      if (entry.fileName && entry.track) {
          if (parentId === -1)
            parentId = addContainerTree([container], parent.id);
          result.push(parentId);
          result.push(storeCueEntry(entry, container, parentId, cue, rootPath));
      }
      entry.track = matches[1];
      entry.type = matches[2];
    } else if (reCueTrackTitle.test(line)) {
      const matches = reCueTrackTitle.exec(line);
      entry.trackTitle = matches[1];
    } else if (reCueTrackIndex.test(line)) {
      const matches = reCueTrackIndex.exec(line);
      entry.count = matches[1];
      entry.offset = matches[2];
    }

    line = readln();
  } while (line !== undefined);
  
  if (entry.fileName && entry.track) {
      if (parentId === -1)
        parentId = addContainerTree([container], parent.id);
      result.push(parentId);
      result.push(storeCueEntry(entry, container, parentId, cue, rootPath));
  }
  return result;
}

function storeCueEntry(entry, parent, parentId, cue, rootPath) {
    entry.location = entry.fileName.startsWith('/') ? entry.fileName : parent.location + "/" + entry.fileName;
    var cds = getCdsObject(entry.location);
    if (!cds)
      cds = {};
    cds.id = -1;
    cds.refID = cue.id;
    cds.sortKey = '';
    if (!cds.mimetype)
      cds.mimetype = entry.format;
    cds.location = entry.location;
    cds.title = entry.trackTitle ? entry.trackTitle : entry.fileName;
    cds.trackNumber = entry.track;
    cds.description = entry.description ? entry.description : ("Entry from " + cue.location);
    if (entry.performer) {
      cds.metaData[M_ARTIST] = [entry.performer];
      cds.metaData[M_ALBUMARTIST] = [entry.performer];
    }
    if (entry.discTitle)
      cds.metaData[M_ALBUM] = [entry.discTitle];

    print2("Info", "cuesheet: " + cue.location + "\nparent=" + JSON.stringify(parent, null, " ") + "\nentry= " + JSON.stringify(entry, null, " ")+ "\ncds= " + JSON.stringify(cds, null, " "));
    return addCueObject(cds, entry, parentId, rootPath);
}
