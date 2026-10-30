/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// @file content/scripting/asx_playlist.h

#ifndef GERBERA_ASX_PLAYLIST_H
#define GERBERA_ASX_PLAYLIST_H

#include "xml_playlist.h"

class AsxPlaylist : public XmlPlaylist {
public:
    struct Reference {
        std::string href;
        int writeThrough { -1 };
    };

    struct Metadata {
        std::string title;
        std::string author;
        std::string copyright;
        std::string description;
        std::map<std::string, std::string> parameters;
    };

    struct Entry {
        fs::path source;
        std::string base;
        Metadata metadata;
        std::vector<Reference> references;
    };

    struct Plan {
        Metadata metadata;
        std::vector<Entry> entries;
        std::vector<std::string> warnings;
    };

    explicit AsxPlaylist(Loader loader);
    /// Builds the complete bounded plan before importing any positions.
    /// REF alternatives stay ordered for the content importer to resolve.
    static Plan readPlan(const pugi::xml_node& root, const fs::path& source, const Loader& loader);
    unsigned int getParseFlags() const override { return pugi::parse_default; }
    std::optional<std::reference_wrapper<const XmlPlaylistEntry>> readPlaylistEntry() override;

protected:
    void readEntries(pugi::xml_node root, const fs::path& path) override;

private:
    static constexpr std::size_t ASX_MAX_POSITIONS = 10000;
    static constexpr std::size_t ASX_MAX_NESTING = 5;
    static constexpr std::size_t ASX_MAX_REFERENCES = 10000;
    enum class RepeatContext { Document,
        Repeat };

    static pugi::xml_attribute readAttribute(const pugi::xml_node& node, const std::string& name);
    static std::string readBase(const pugi::xml_node& node, std::string fallback = {});
    static Metadata readMetadata(const pugi::xml_node& node, Metadata inherited = {});
    static void addWarning(Plan& plan, const std::string& message, const fs::path& source);
    static void warnUnsupported(const pugi::xml_node& node, Plan& plan, const fs::path& source);
    static void appendEntries(std::vector<Entry>& target, const std::vector<Entry>& sequence, std::size_t count = 1);
    static std::size_t readRepeatCount(const pugi::xml_node& node, Plan& plan, const fs::path& source);
    static std::vector<Entry> readDocumentEntries(const pugi::xml_node& root, const fs::path& source, const Loader& loader,
        std::vector<fs::path>& active, Plan& plan, Metadata defaults, std::size_t& references);
    static std::vector<Entry> readSequenceEntries(const pugi::xml_node& parent, const fs::path& source, const Loader& loader,
        std::vector<fs::path>& active, Plan& plan, const Metadata& defaults, const std::string& base, RepeatContext context, std::size_t& references);

    Loader loader;
    Plan plan;
    XmlPlaylistEntry currentEntry;
};

#endif // GERBERA_ASX_PLAYLIST_H
