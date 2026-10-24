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

/// @file util/asx_playlist.cc

#include "asx_playlist.h" // API

#include "util/tools.h"

#include <algorithm>
#include <charconv>
#include <set>
#include <stdexcept>
#include <string_view>

static constexpr std::size_t ASX_MAX_POSITIONS = 10000;
static constexpr std::size_t ASX_MAX_NESTING = 5;
static constexpr std::size_t ASX_MAX_REFERENCES = 10000;

enum class AsxRepeatContext { Document,
    Repeat };

static pugi::xml_attribute asxAttribute(const pugi::xml_node& node,
    const std::string& name)
{
    for (auto attribute : node.attributes()) {
        if (toLower(attribute.name()) == name)
            return attribute;
    }
    return {};
}

static std::string asxBase(const pugi::xml_node& node,
    std::string fallback = {})
{
    for (auto child : node.children()) {
        if (toLower(child.name()) == "base")
            return asxAttribute(child, "href").as_string();
    }
    return fallback;
}

static AsxPlaylistMetadata asxMetadata(const pugi::xml_node& node,
    AsxPlaylistMetadata metadata = {})
{
    std::set<std::string> seen;
    for (auto child : node.children()) {
        auto name = toLower(child.name());
        if (name == "param") {
            auto parameter = asxAttribute(child, "name");
            if (parameter)
                metadata.parameters[parameter.value()] = asxAttribute(child, "value").as_string();
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

static void asxWarn(AsxPlaylistPlan& plan, const std::string& message,
    const fs::path& source)
{
    auto warning = message + ": " + source.string();
    if (std::find(plan.warnings.begin(), plan.warnings.end(), warning) == plan.warnings.end())
        plan.warnings.push_back(std::move(warning));
}

static void asxUnsupported(const pugi::xml_node& node, AsxPlaylistPlan& plan,
    const fs::path& source)
{
    auto name = toLower(node.name());
    if (name == "starttime" || name == "duration" || name == "previewduration" || name == "startmarker" || name == "endmarker")
        asxWarn(plan, "ASX clip/marker playback semantics are unsupported", source);
    else if (name == "event" || name == "banner" || name == "moreinfo" || name == "logurl" || name == "skin")
        asxWarn(plan, "ASX player element is unsupported (" + name + ")", source);
}

static void asxAppend(std::vector<AsxPlaylistEntry>& target,
    const std::vector<AsxPlaylistEntry>& sequence,
    std::size_t count = 1)
{
    if (sequence.size() > (ASX_MAX_POSITIONS - target.size()) / count)
        throw std::runtime_error("ASX exceeds 10000 expanded positions");
    for (std::size_t i = 0; i < count; ++i)
        target.insert(target.end(), sequence.begin(), sequence.end());
}

static std::size_t asxRepeatCount(const pugi::xml_node& node,
    AsxPlaylistPlan& plan,
    const fs::path& source)
{
    auto attribute = asxAttribute(node, "count");
    if (!attribute) {
        asxWarn(plan, "ASX unbounded REPEAT imported once", source);
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

static std::vector<AsxPlaylistEntry>
asxDocumentEntries(const pugi::xml_node& root, const fs::path& source,
    const AsxPlaylistLoader& loader,
    std::vector<fs::path>& active, AsxPlaylistPlan& plan, AsxPlaylistMetadata defaults, std::size_t& references);

static std::vector<AsxPlaylistEntry>
asxEntries(const pugi::xml_node& parent, const fs::path& source,
    const AsxPlaylistLoader& loader, std::vector<fs::path>& active,
    AsxPlaylistPlan& plan, const AsxPlaylistMetadata& defaults,
    const std::string& base, AsxRepeatContext context, std::size_t& references)
{
    std::vector<AsxPlaylistEntry> entries;
    bool repeated = false;
    for (auto node : parent.children()) {
        auto name = toLower(node.name());
        if (name == "entry") {
            if (active.size() > 1 && toLower(asxAttribute(node, "skipifref").as_string()) == "yes")
                continue;
            AsxPlaylistEntry entry {
                source, asxBase(node, base), asxMetadata(node, defaults), {}
            };
            if (toLower(asxAttribute(node, "clientskip").as_string()) == "no")
                asxWarn(plan, "ASX CLIENTSKIP playback control is unsupported", source);
            for (auto child : node.children()) {
                if (toLower(child.name()) == "ref") {
                    auto href = asxAttribute(child, "href");
                    if (href && *href.value())
                        entry.references.push_back(
                            { href.value(), asxAttribute(child, "writethrough").as_int(-1) });
                    for (auto marker : child.children())
                        asxUnsupported(marker, plan, source);
                } else
                    asxUnsupported(child, plan, source);
            }
            if (!entry.references.empty())
                asxAppend(entries, { entry });
        } else if (name == "entryref") {
            if (++references > ASX_MAX_REFERENCES)
                throw std::runtime_error("ASX exceeds 10000 ENTRYREF traversals");
            auto href = asxAttribute(node, "href");
            if (!href || !*href.value()) {
                asxWarn(plan, "ASX ENTRYREF without HREF skipped", source);
                continue;
            }
            auto child = loader(href.value(), source);
            if (!child.xml) {
                asxWarn(plan,
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
            auto nested = asxDocumentEntries(child.xml->document_element(), canonical,
                loader, active, plan, asxMetadata(child.xml->document_element()), references);
            active.pop_back();
            asxAppend(entries, nested);
        } else if (name == "repeat") {
            if (context == AsxRepeatContext::Repeat || repeated) {
                asxWarn(plan, "ASX subsequent or nested REPEAT ignored", source);
                continue;
            }
            repeated = true;
            auto count = asxRepeatCount(node, plan, source);
            auto sequence = asxEntries(node, source, loader, active, plan, defaults, base, AsxRepeatContext::Repeat, references);
            asxAppend(entries, sequence, count);
        } else
            asxUnsupported(node, plan, source);
    }
    return entries;
}

static std::vector<AsxPlaylistEntry>
asxDocumentEntries(const pugi::xml_node& root, const fs::path& source,
    const AsxPlaylistLoader& loader,
    std::vector<fs::path>& active, AsxPlaylistPlan& plan, AsxPlaylistMetadata defaults, std::size_t& references)
{
    if (toLower(root.name()) != "asx")
        throw std::runtime_error("Expected an ASX document");
    auto version = asxAttribute(root, "version");
    if (version && std::string(version.value()) != "3.0" && std::string(version.value()) != "3")
        throw std::runtime_error("Only ASX version 3 is supported");
    defaults.title.clear();
    return asxEntries(root, source, loader, active, plan, defaults, asxBase(root),
        AsxRepeatContext::Document, references);
}

AsxPlaylistPlan readAsxPlaylistPlan(const pugi::xml_node& root,
    const fs::path& source,
    const AsxPlaylistLoader& loader)
{
    AsxPlaylistPlan plan;
    plan.metadata = asxMetadata(root);
    auto canonical = source.lexically_normal();
    std::vector<fs::path> active { canonical };
    std::size_t references = 0;
    plan.entries = asxDocumentEntries(root, canonical, loader, active, plan, plan.metadata, references);
    return plan;
}
