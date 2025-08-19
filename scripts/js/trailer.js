/*GRB*
  Gerbera - https://gerbera.io/

  trailer.js - this file is part of Gerbera.

  Copyright (C) 2018-2025 Gerbera Contributors

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

// doc-add-trailer-begin
function importOnlineItem(obj, cont, rootPath, autoscanId, containerType) {
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  obj.sortKey = '';
  const chain = {
    trailerRoot: {
      id: boxSetup[BK_trailerRoot].id,
      title: boxSetup[BK_trailerRoot].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerRoot].class,
      upnpShortcut: boxSetup[BK_trailerRoot].upnpShortcut,
      metaData: {}
    },
    appleTrailers: {
      id: boxSetup[BK_trailerApple].id,
      title: boxSetup[BK_trailerApple].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerApple].class,
      upnpShortcut: boxSetup[BK_trailerApple].upnpShortcut,
      metaData: {}
    },
    allTrailers: {
      id: boxSetup[BK_trailerAll].id,
      title: boxSetup[BK_trailerAll].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerAll].class,
      upnpShortcut: boxSetup[BK_trailerAll].upnpShortcut,
      metaData: {}
    },
    allGenres: {
      id: boxSetup[BK_trailerAllGenres].id,
      title: boxSetup[BK_trailerAllGenres].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerAllGenres].class,
      upnpShortcut: boxSetup[BK_trailerAllGenres].upnpShortcut,
      metaData: {}
    },
    relDate: {
      id: boxSetup[BK_trailerRelDate].id,
      title: boxSetup[BK_trailerRelDate].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerRelDate].class,
      upnpShortcut: boxSetup[BK_trailerRelDate].upnpShortcut,
      metaData: {}
    },
    postDate: {
      id: boxSetup[BK_trailerPostDate].id,
      title: boxSetup[BK_trailerPostDate].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerPostDate].class,
      upnpShortcut: boxSetup[BK_trailerPostDate].upnpShortcut,
      metaData: {}
    },

    genre: {
      title: boxSetup[BK_trailerUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
      metaData: {}
    },
    date: {
      title: boxSetup[BK_trailerUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {}
    }
  };
  // First we will add the item to the 'All Trailers' container, so
  // that we get a nice long playlist:

  const result = [];
  result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.allTrailers]), rootPath));

  // We also want to sort the trailers by genre, however we need to
  // take some extra care here: the genre property here is a comma
  // separated value list, so one trailer can have several matching
  // genres that will be returned as one string. We will split that
  // string and create individual genre containers.

  if (boxSetup[BK_trailerAllGenres].enabled && obj.metaData[M_GENRE] && obj.metaData[M_GENRE][0]) {
    var genre = obj.metaData[M_GENRE][0];

    // A genre string "Science Fiction, Thriller" will be split to
    // "Science Fiction" and "Thriller" respectively.

    const genres = genre.split(', ');
    for (var i = 0; i < genres.length; i++) {
      chain.genre.title = genres[i];
      chain.genre.metaData[M_GENRE] = [genres[i]];
      result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.allGenres, chain.genre]), rootPath));
    }
  }

  // The release date is offered in a YYYY-MM-DD format, we won't do
  // too much extra checking regading validity, however we only want
  // to group the trailers by year and month:

  if (boxSetup[BK_trailerRelDate].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
    var reldate = obj.metaData[M_DATE][0];
    if (reldate.length >= 7) {
      chain.date.title = reldate.slice(0, 7);
      chain.date.metaData[M_DATE] = [reldate.slice(0, 7)];
      chain.date.metaData[M_UPNP_DATE] = [reldate.slice(0, 7)];
      result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.relDate, chain.date]), rootPath));
    }
  }

  // We also want to group the trailers by the date when they were
  // originally posted, the post date is available via the aux
  // array. Similar to the release date, we will cut off the day and
  // create our containres in the YYYY-MM format.
  if (boxSetup[BK_trailerPostDate].enabled) {
    var postdate = obj.aux[APPLE_TRAILERS_AUXDATA_POST_DATE];
    if (postdate && postdate.length >= 7) {
      chain.date.title = postdate.slice(0, 7);
      chain.date.metaData[M_DATE] = [postdate.slice(0, 7)];
      chain.date.metaData[M_UPNP_DATE] = [postdate.slice(0, 7)];
      result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.postDate, chain.date]), rootPath));
    }
  }
  return result;
}
// doc-add-trailer-end
