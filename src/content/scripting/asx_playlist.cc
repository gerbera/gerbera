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

/// @file content/scripting/asx_playlist.cc

#define GRB_LOG_FAC GrbLogFacility::script

#include "asx_playlist.h" // API

#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <algorithm>
#include <charconv>
#include <regex>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

pugi::xml_attribute AsxPlaylist::readAttribute(const pugi::xml_node& node,
    const std::string& name)
{
    pugi::xml_attribute result;
    for (auto attribute : node.attributes()) {
        if (toLower(attribute.name()) == name)
            result = attribute;
    }
    return result;
}

std::string AsxPlaylist::readBase(const pugi::xml_node& node,
    std::string fallback)
{
    for (auto child : node.children()) {
        if (toLower(child.name()) == "base")
            return readAttribute(child, "href").as_string();
    }
    return fallback;
}

AsxPlaylist::Metadata AsxPlaylist::readMetadata(const pugi::xml_node& node,
    Metadata metadata)
{
    std::set<std::string> seen;
    for (auto child : node.children()) {
        auto name = toLower(child.name());
        if (name == "param") {
            auto parameter = readAttribute(child, "name");
            if (parameter)
                metadata.parameters[parameter.value()] = readAttribute(child, "value").as_string();
        } else if (seen.insert(name).second) {
            auto value = child.text().as_string();
            if (name == "title")
                metadata.title = value;
            else if (name == "author")
                metadata.author = value;
            else if (name == "copyright")
                metadata.copyright = value;
            else if (name == "abstract")
                metadata.description = value;
        }
    }
    return metadata;
}

void AsxPlaylist::addWarning(Plan& plan, const std::string& message,
    const fs::path& source)
{
    auto warning = message + ": " + source.string();
    if (std::find(plan.warnings.begin(), plan.warnings.end(), warning) == plan.warnings.end())
        plan.warnings.push_back(std::move(warning));
}

void AsxPlaylist::warnUnsupported(const pugi::xml_node& node, Plan& plan,
    const fs::path& source)
{
    auto name = toLower(node.name());
    if (name == "starttime" || name == "duration" || name == "previewduration" || name == "startmarker" || name == "endmarker")
        addWarning(plan, "ASX clip/marker playback semantics are unsupported", source);
    else if (name == "event" || name == "banner" || name == "moreinfo" || name == "logurl" || name == "skin")
        addWarning(plan, "ASX player element is unsupported (" + name + ")", source);
}

void AsxPlaylist::appendEntries(std::vector<Entry>& target,
    const std::vector<Entry>& sequence,
    std::size_t count)
{
    if (sequence.size() > (ASX_MAX_POSITIONS - target.size()) / count)
        throw std::runtime_error("ASX exceeds 10000 expanded positions");
    for (std::size_t i = 0; i < count; ++i)
        target.insert(target.end(), sequence.begin(), sequence.end());
}

std::size_t AsxPlaylist::readRepeatCount(const pugi::xml_node& node,
    Plan& plan,
    const fs::path& source)
{
    auto attribute = readAttribute(node, "count");
    if (!attribute) {
        addWarning(plan, "ASX unbounded REPEAT imported once", source);
        return 1;
    }
    std::string_view value = attribute.value();
    std::size_t count = 0;
    auto parsed = std::from_chars(value.data(), value.data() + value.size(), count);
    if (value.empty() || parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() || count > ASX_MAX_POSITIONS)
        throw std::runtime_error(
            "ASX REPEAT COUNT must be an integer between 0 and 10000");
    return std::max<std::size_t>(1, count);
}

std::vector<AsxPlaylist::Entry>
AsxPlaylist::readSequenceEntries(const pugi::xml_node& parent, const fs::path& source,
    const Loader& loader, std::vector<fs::path>& active,
    Plan& plan, const Metadata& defaults,
    const std::string& base, RepeatContext context, std::size_t& references)
{
    std::vector<Entry> entries;
    bool repeated = false;
    for (auto node : parent.children()) {
        auto name = toLower(node.name());
        if (name == "entry") {
            if (active.size() > 1 && toLower(readAttribute(node, "skipifref").as_string()) == "yes")
                continue;
            Entry entry {
                source, readBase(node, base), readMetadata(node, defaults), {}
            };
            if (toLower(readAttribute(node, "clientskip").as_string()) == "no")
                addWarning(plan, "ASX CLIENTSKIP playback control is unsupported", source);
            for (auto child : node.children()) {
                if (toLower(child.name()) == "ref") {
                    auto href = readAttribute(child, "href");
                    if (href && *href.value())
                        entry.references.push_back(
                            { href.value(), readAttribute(child, "writethrough").as_int(-1) });
                    for (auto marker : child.children())
                        warnUnsupported(marker, plan, source);
                } else
                    warnUnsupported(child, plan, source);
            }
            if (!entry.references.empty())
                appendEntries(entries, { entry });
        } else if (name == "entryref") {
            if (++references > ASX_MAX_REFERENCES)
                throw std::runtime_error("ASX exceeds 10000 ENTRYREF traversals");
            auto href = readAttribute(node, "href");
            if (!href || !*href.value()) {
                addWarning(plan, "ASX ENTRYREF without HREF skipped", source);
                continue;
            }
            auto child = loader(href.value(), source);
            if (!child.xml) {
                addWarning(plan,
                    child.warning.empty() ? "ASX unavailable ENTRYREF skipped"
                                          : child.warning,
                    source);
                continue;
            }
            if (active.size() > ASX_MAX_NESTING)
                throw std::runtime_error("ASX exceeds five nested ENTRYREF documents");
            auto canonical = child.source.lexically_normal();
            if (std::find(active.begin(), active.end(), canonical) != active.end())
                throw std::runtime_error("ASX ENTRYREF cycle");
            active.push_back(canonical);
            auto nested = readDocumentEntries(child.xml->document_element(), canonical,
                loader, active, plan, readMetadata(child.xml->document_element()), references);
            active.pop_back();
            appendEntries(entries, nested);
        } else if (name == "repeat") {
            if (context == RepeatContext::Repeat || repeated) {
                addWarning(plan, "ASX subsequent or nested REPEAT ignored", source);
                continue;
            }
            repeated = true;
            auto count = readRepeatCount(node, plan, source);
            auto sequence = readSequenceEntries(node, source, loader, active, plan, defaults, base, RepeatContext::Repeat, references);
            appendEntries(entries, sequence, count);
        } else
            warnUnsupported(node, plan, source);
    }
    return entries;
}

std::vector<AsxPlaylist::Entry>
AsxPlaylist::readDocumentEntries(const pugi::xml_node& root, const fs::path& source,
    const Loader& loader,
    std::vector<fs::path>& active, Plan& plan, Metadata defaults, std::size_t& references)
{
    if (toLower(root.name()) != "asx")
        throw std::runtime_error("Expected an ASX document");
    auto version = readAttribute(root, "version");
    if (version && std::string(version.value()) != "3.0" && std::string(version.value()) != "3")
        throw std::runtime_error("Only ASX version 3 is supported");
    defaults.title.clear();
    return readSequenceEntries(root, source, loader, active, plan, defaults, readBase(root),
        RepeatContext::Document, references);
}

AsxPlaylist::Plan AsxPlaylist::readPlan(const pugi::xml_node& root,
    const fs::path& source,
    const Loader& loader)
{
    Plan plan;
    plan.metadata = readMetadata(root);
    auto canonical = source.lexically_normal();
    std::vector<fs::path> active { canonical };
    std::size_t references = 0;
    plan.entries = readDocumentEntries(root, canonical, loader, active, plan, plan.metadata, references);
    return plan;
}

AsxPlaylist::AsxPlaylist(Loader loader)
    : loader(std::move(loader))
{
    if (!this->loader)
        throw_std_runtime_error("ASX requires an explicit ENTRYREF loader");
}

void AsxPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    plan = {};
    currentEntry = {};
    // The complete bounded plan owns its strings; no caller DOM is retained.
    plan = readPlan(root, fs::weakly_canonical(path), loader);
    for (const auto& warning : plan.warnings)
        log_warning("{}", warning);
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> AsxPlaylist::readPlaylistEntry()
{
    currentEntry = {};
    if (!readComplete)
        return std::nullopt;
    // Preserve the existing ASX BASE extension's ASCII scheme policy.
    static const std::regex scheme("^[a-zA-Z][a-zA-Z0-9+.-]*:");
    while (entryPosition < plan.entries.size()) {
        const auto& entry = plan.entries[entryPosition++];
        currentEntry = {};
        currentEntry.title = entry.metadata.title;
        currentEntry.description = entry.metadata.description;
        if (!entry.metadata.author.empty())
            currentEntry.defaults["upnp:author"] = entry.metadata.author;
        if (!entry.metadata.copyright.empty())
            currentEntry.defaults["dc:rights"] = entry.metadata.copyright;
        for (const auto& [name, value] : entry.metadata.parameters) {
            if (name == "size")
                currentEntry.size = value;
            else if (name == "mimetype")
                currentEntry.mimetype = value;
            else if (name == "protocol")
                currentEntry.protocol = value;
            else
                currentEntry.extra[name] = value;
        }
        auto base = entry.base;
        while (!base.empty() && base.back() == '/')
            base.pop_back();
        for (const auto& reference : entry.references) {
            auto href = reference.href;
            if (!entry.base.empty() && href.front() != '/' && !std::regex_search(href, scheme))
                href = base + '/' + href;
            try {
                if (auto location = URLUtils::resolvePlaylistReference(href, entry.source)) {
                    currentEntry.locations.push_back(*location);
                    currentEntry.writeThroughs.push_back(reference.writeThrough);
                }
            } catch (const std::runtime_error& e) {
                log_warning("Cannot resolve playlist reference {}: {}", href, EX(e));
            }
        }
        if (!currentEntry.locations.empty())
            return std::cref(currentEntry);
    }
    return std::nullopt;
}
