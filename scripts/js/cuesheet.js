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
  print2("Info", "Processing cuesheet: " + cue.location + " as " + arr[arr.length - 1].toLowerCase());
  var result = [];
  switch (arr[arr.length - 1].toLowerCase()) {
    case "cue":
      result = parseCue(cue, cont, rootPath);
      break;
  }
  return result;
}

const reCuePerformer = new RegExp("^performer\\s+\"(.+)\"", 'i');
const reCueDiscTitle = new RegExp("^title\\s+\"(.+)\"", 'i');
const reCueFile = new RegExp("^file\\s+\"(.+)\"\\s+(\\S.+)", 'i');
const reCueTrack = new RegExp("^\\s+track\\s+(\\d+)\\s+(\\S+)", 'i');
const reCueTrackTitle = new RegExp("^\\s+title \"(.+)\"", 'i');
const reCueTrackIndex = new RegExp("^\\s+index\\s+(\\d+)\\s+(\\d+:\\d+):(\\d+)", 'i');
const reCueTrackPerformer = new RegExp("^\\s+performer\\s+\"(.+)\"", 'i');

function parseCue(cue, parent, rootPath) {
  var line = readln();
  const result = [];
  var entry = {
    performer: null,
    discTitle: null,
    discArtist: null,
    fileName: null,
    format: null,
    track: 0,
    type: null,
    trackTitle: null,
    frames: 0,
    offset: null,
  };
  var parentId = -1;
  var container = JSON.parse(JSON.stringify(parent)); // create a copy
  container.entryType = ET_ExtraDirectory;
  container.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ALBUM;
  container.id = -1;
  container.refID = cue.id;
  container.searchable = false;
  if (!container.metaData)
    container.metaData = {};
  do {
    if (reCuePerformer.test(line)) { // PERFORMER
      const matches = reCuePerformer.exec(line);
      entry.performer = matches[1];
      entry.discArtist = matches[1];
      container.metaData[M_ALBUMARTIST] = [entry.performer];
      parentId = -1;
    } else if (reCueDiscTitle.test(line)) { // TITLE
      const matches = reCueDiscTitle.exec(line);
      entry.discTitle = matches[1];
      container.title = entry.discTitle;
      parentId = -1;
    } else if (reCueFile.test(line)) { // FILE
      const matches = reCueFile.exec(line);
      if (entry.fileName && entry.offset) {
        if (parentId === -1) {
          parentId = addContainerTree([container], parent.id);
          result.push(parentId);
        }
        result.push(storeCueEntry(entry, container, parentId, cue, rootPath));
        entry.track = 0;
        entry.type = null;
        entry.offset = null;
        entry.frames = 0;
        entry.performer = entry.discArtist;
      }
      entry.fileName = matches[1];
      entry.format = matches[2];
    } else if (reCueTrack.test(line)) { // TRACK
      const matches = reCueTrack.exec(line);
      if (entry.fileName && entry.offset) {
        if (parentId === -1) {
          parentId = addContainerTree([container], parent.id);
          result.push(parentId);
        }
        result.push(storeCueEntry(entry, container, parentId, cue, rootPath));
      }
      entry.performer = entry.discArtist;
      entry.track = Number.parseInt(matches[1]);
      entry.type = matches[2];
      entry.offset = null;
      entry.frames = 0;
    } else if (reCueTrackPerformer.test(line)) { // TRACK -> PERFORMER
      const matches = reCueTrackPerformer.exec(line);
      entry.performer = matches[1];
    } else if (reCueTrackTitle.test(line)) { // TRACK -> TITLE
      const matches = reCueTrackTitle.exec(line);
      entry.trackTitle = matches[1];
    } else if (reCueTrackIndex.test(line)) { // INDEX
      const matches = reCueTrackIndex.exec(line);
      if (matches[1] === '01') {
        entry.offset = matches[2];
        entry.frames = Number.parseInt(matches[3]);
      }
    }

    line = readln();
  } while (line !== undefined);
  
  if (entry.fileName && entry.offset) {
      if (parentId === -1) {
        parentId = addContainerTree([container], parent.id);
        result.push(parentId);
      }
      result.push(storeCueEntry(entry, container, parentId, cue, rootPath));
  }

  return result;
}

function storeCueEntry(entry, parent, parentId, cue, rootPath) {
    entry.location = entry.fileName.startsWith('/') ? entry.fileName : parent.location + "/" + entry.fileName;
    var cds = getCdsObject(entry.location);
    if (!cds)
      cds = {};
    if (!cds.metaData)
      cds.metaData = {};
    cds.refID = cds.id;
    cds.id = -1;
    cds.sortKey = '';
    if (!cds.mimetype)
      cds.mimetype = entry.format;
    cds.location = entry.location;
    cds.title = entry.trackTitle ? entry.trackTitle : entry.fileName;
    cds.trackNumber = entry.track;
    cds.description = entry.description ? entry.description : ("Entry from " + cue.location);
    if (entry.performer) {
      cds.metaData[M_ARTIST] = [entry.performer];
      cds.metaData[M_ALBUMARTIST] = [entry.discArtist];
    }
    if (entry.discTitle)
      cds.metaData[M_ALBUM] = [entry.discTitle];

    return addCueObject(cds, entry, parentId, rootPath);
}
