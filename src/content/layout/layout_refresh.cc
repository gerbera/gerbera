/*GRB*

    Gerbera - https://gerbera.io/

    layout_refresh.cc - this file is part of Gerbera.

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
*/

/// @file content/layout/layout_refresh.cc

#include "layout_refresh.h" // API

#include "cds/cds_objects.h"
#include "common.h"
#include "exceptions.h"
#include "metadata/metadata_enums.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <utility>

namespace {

using LayoutObjects = std::map<int, std::shared_ptr<CdsObject>>;
using MatchKey = std::tuple<CdsEntryType, unsigned int, std::string, std::vector<int>>;

struct Siblings {
    std::vector<int> current;
    std::vector<int> desired;
};

class LayoutGraph {
public:
    explicit LayoutGraph(const LayoutObjects& objects)
        : objects(objects)
    {
        children.try_emplace(CDS_ID_ROOT);
        for (const auto& [id, object] : objects) {
            if (id == CDS_ID_ROOT)
                continue;
            if (!object || object->getID() != id || object->getTitle().empty()
                || (!object->isContainer() && !object->isItem())
                || (object->isContainer() && object->getLocation().empty()))
                throw_std_runtime_error("Invalid audio layout object {}", id);
            children.try_emplace(id);
            children[object->getParentID()].push_back(id);
        }

        collectReferences(CDS_ID_ROOT);
        const auto nodeCount = objects.count(CDS_ID_ROOT) ? objects.size() : objects.size() + 1;
        if (references.size() != nodeCount)
            throw_std_runtime_error("Audio layout contains objects without a path to the root");
    }

    CdsObject& at(int id) const { return *objects.at(id); }
    const std::vector<int>& childrenOf(int id) const { return children.at(id); }

    MatchKey matchKey(int id) const
    {
        const auto& object = at(id);
        return { object.getEntryType(), object.getObjectType(), object.getClass(), references.at(id) };
    }

    std::string exactKey(int id) const
    {
        const auto& object = at(id);
        return object.isContainer() ? object.getLocation().string() : object.getTitle();
    }

private:
    const LayoutObjects& objects;
    std::map<int, std::vector<int>> children;
    std::map<int, std::vector<int>> references;

    const std::vector<int>& collectReferences(int id)
    {
        auto& result = references[id];
        if (id != CDS_ID_ROOT && at(id).isItem()) {
            const int refId = at(id).getRefID();
            if (refId <= CDS_ID_ROOT || !childrenOf(id).empty())
                throw_std_runtime_error("Invalid audio layout track reference {}", id);
            result.push_back(refId);
            return result;
        }

        for (int child : childrenOf(id)) {
            const auto& refs = collectReferences(child);
            result.insert(result.end(), refs.begin(), refs.end());
        }
        std::sort(result.begin(), result.end());
        return result;
    }
};

void matchChildren(const LayoutGraph& current, const LayoutGraph& desired,
    int currentParent, int desiredParent, std::vector<std::shared_ptr<CdsObject>>& changes)
{
    std::map<MatchKey, Siblings> byStructure;
    for (int id : current.childrenOf(currentParent))
        byStructure[current.matchKey(id)].current.push_back(id);
    for (int id : desired.childrenOf(desiredParent))
        byStructure[desired.matchKey(id)].desired.push_back(id);

    const auto match = [&](int currentId, int desiredId) {
        auto& oldObject = current.at(currentId);
        const auto& newObject = desired.at(desiredId);
        const auto oldTitle = oldObject.getTitle();
        const auto newTitle = newObject.getTitle();
        const bool titleChanged = oldTitle != newTitle;
        const bool isContainer = oldObject.isContainer();
        const bool pathChanged = isContainer && oldObject.getLocation() != newObject.getLocation();
        if (titleChanged || pathChanged) {
            auto update = CdsObject::createObject(oldObject.getEntryType());
            oldObject.copyTo(update);
            update->setUTime(oldObject.getUTime());
            if (titleChanged) {
                update->setTitle(newTitle);
                const auto oldSortKey = oldObject.getSortKey();
                if (oldSortKey.empty() || oldSortKey == oldTitle)
                    update->setSortKey(newTitle);
                if (isContainer)
                    update->addMetaData(MetadataFields::M_TITLE, newTitle);
            }
            if (pathChanged)
                update->setLocation(newObject.getLocation(), oldObject.getEntryType());
            changes.push_back(std::move(update));
        }
        if (isContainer)
            matchChildren(current, desired, currentId, desiredId, changes);
    };

    for (const auto& [structure, siblings] : byStructure) {
        if (siblings.current.size() != siblings.desired.size())
            throw_std_runtime_error("Cannot preserve audio layout IDs: contents change under container {}", currentParent);
        if (siblings.current.size() == 1) {
            match(siblings.current.front(), siblings.desired.front());
            continue;
        }

        std::map<std::string, Siblings> byExactKey;
        for (int id : siblings.current)
            byExactKey[current.exactKey(id)].current.push_back(id);
        for (int id : siblings.desired)
            byExactKey[desired.exactKey(id)].desired.push_back(id);

        Siblings unmatched;
        for (const auto& [key, exact] : byExactKey) {
            if (exact.current.size() == 1 && exact.desired.size() == 1) {
                match(exact.current.front(), exact.desired.front());
                continue;
            }
            unmatched.current.insert(unmatched.current.end(), exact.current.begin(), exact.current.end());
            unmatched.desired.insert(unmatched.desired.end(), exact.desired.begin(), exact.desired.end());
        }

        if (unmatched.current.size() > 1)
            throw_std_runtime_error("Cannot preserve audio layout IDs: ambiguous titles under container {}", currentParent);
        if (!unmatched.current.empty())
            match(unmatched.current.front(), unmatched.desired.front());
    }
}

} // namespace

std::vector<std::shared_ptr<CdsObject>> planAudioLayoutTitles(
    const LayoutObjects& current, const LayoutObjects& desired)
{
    std::vector<std::shared_ptr<CdsObject>> changes;
    matchChildren(LayoutGraph(current), LayoutGraph(desired), CDS_ID_ROOT, CDS_ID_ROOT, changes);
    return changes;
}
