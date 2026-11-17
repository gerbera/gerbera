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

#include "cds/cds_container.h"
#include "cds/cds_objects.h"
#include "common.h"
#include "content/import_service.h"
#include "database/database.h"
#include "database/db_param.h"
#include "exceptions.h"
#include "metadata/metadata_enums.h"

#include <algorithm>
#include <limits>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>

class LayoutRefreshService::LayoutGraph {
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

    std::shared_ptr<CdsObject> at(int id) const { return objects.at(id); }
    const std::vector<int>& childrenOf(int id) const { return children.at(id); }

    using MatchKey = std::tuple<CdsEntryType, unsigned int, std::string, std::vector<int>>;

    MatchKey matchKey(int id) const
    {
        const auto& object = at(id);
        return { object->getEntryType(), object->getObjectType(), object->getClass(), references.at(id) };
    }

    void matchChildren(const LayoutGraph& desired,
        int currentParent, int desiredParent, std::vector<std::shared_ptr<CdsObject>>& changes) const;

    std::string exactKey(int id) const
    {
        const auto& object = at(id);
        return object->isContainer() ? object->getLocation().string() : object->getTitle();
    }

private:
    const LayoutObjects& objects;
    std::map<int, std::vector<int>> children;
    std::map<int, std::vector<int>> references;

    const std::vector<int>& collectReferences(int id)
    {
        auto& result = references[id];
        if (id != CDS_ID_ROOT && at(id)->isItem()) {
            const int refId = at(id)->getRefID();
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

void LayoutRefreshService::LayoutGraph::matchChildren(const LayoutGraph& desired,
    int currentParent, int desiredParent, std::vector<std::shared_ptr<CdsObject>>& changes) const
{
    struct Siblings {
        std::vector<int> current;
        std::vector<int> desired;
    };
    std::map<MatchKey, Siblings> byStructure;
    for (int id : childrenOf(currentParent))
        byStructure[matchKey(id)].current.push_back(id);
    for (int id : desired.childrenOf(desiredParent))
        byStructure[desired.matchKey(id)].desired.push_back(id);

    const auto match = [&](int currentId, int desiredId) {
        const auto oldObject = at(currentId);
        const auto newObject = desired.at(desiredId);
        const auto oldTitle = oldObject->getTitle();
        const auto newTitle = newObject->getTitle();
        const bool titleChanged = oldTitle != newTitle;
        const bool isContainer = oldObject->isContainer();
        const bool pathChanged = isContainer && oldObject->getLocation() != newObject->getLocation();
        if (titleChanged || pathChanged) {
            auto update = CdsObject::createObject(oldObject->getEntryType());
            oldObject->copyTo(update);
            update->setUTime(oldObject->getUTime());
            if (titleChanged) {
                update->setTitle(newTitle);
                const auto oldSortKey = oldObject->getSortKey();
                if (oldSortKey.empty() || oldSortKey == oldTitle)
                    update->setSortKey(newTitle);
                if (isContainer) {
                    update->removeMetaData(MetadataFields::M_TITLE);
                    update->addMetaData(MetadataFields::M_TITLE, newTitle);
                }
            }
            if (pathChanged)
                update->setLocation(newObject->getLocation(), oldObject->getEntryType());
            changes.push_back(std::move(update));
        }
        if (isContainer)
            matchChildren(desired, currentId, desiredId, changes);
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
            byExactKey[exactKey(id)].current.push_back(id);
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

class LayoutRefreshService::AudioLayoutCapture {
public:
    explicit AudioLayoutCapture(const std::shared_ptr<Database>& database)
        : database(database)
    {
    }

    int trackId { INVALID_OBJECT_ID };
    int nextId { std::numeric_limits<int>::max() };
    std::map<int, std::shared_ptr<CdsObject>> objects;

    int addContainer(int parent, const std::shared_ptr<CdsObject>& object, const std::string& path)
    {
        if (!object->isContainer() || object->getEntryType() != CdsEntryType::VirtualContainer)
            throw_std_runtime_error("Audio layout refresh encountered a nonvirtual container");

        parent = keepExistingParent(parent);
        const auto entry = paths.find(path);
        if (entry != paths.end()) {
            const auto& existing = objects.at(entry->second);
            if (existing->getParentID() != parent || existing->getTitle() != object->getTitle())
                throw_std_runtime_error("Audio layout refresh found conflicting container paths: {}", path);
            return entry->second;
        }

        auto copy = CdsObject::createObject(object->getObjectType());
        object->copyTo(copy);
        copy->setLocation(path, CdsEntryType::VirtualContainer);
        copy->setParentID(parent);
        copy->setID(nextId--);
        objects.emplace(copy->getID(), copy);
        paths.emplace(path, copy->getID());
        return copy->getID();
    }

    void addItem(const std::shared_ptr<CdsObject>& object)
    {
        if (!object->isItem() || object->getRefID() != trackId)
            throw_std_runtime_error("Audio layout refresh encountered an unexpected item");
        object->setParentID(keepExistingParent(object->getParentID()));

        // Match the existing database deduplication key for a virtual item.
        auto [entry, inserted] = items.try_emplace(std::make_tuple(object->getParentID(), trackId, object->getTitle()), nextId);
        object->setID(entry->second);
        if (!inserted)
            return;

        --nextId;
        auto copy = CdsObject::createObject(object->getObjectType());
        object->copyTo(copy);
        objects.emplace(copy->getID(), copy);
    }

private:
    std::shared_ptr<Database> database;
    std::map<std::string, int> paths;
    std::map<std::tuple<int, int, std::string>, int> items;
    std::map<int, int> existingParents;

    int keepExistingParent(int parent)
    {
        if (parent == CDS_ID_ROOT || objects.count(parent))
            return parent;
        const auto known = existingParents.find(parent);
        if (known != existingParents.end())
            return known->second;
        if (parent <= CDS_ID_ROOT || parent > nextId)
            throw_std_runtime_error("Audio layout refresh received an invalid parent: {}", parent);

        auto existing = database->loadObject(parent);
        const auto canonicalParent = keepExistingParent(existing->getParentID());
        const auto [path, inserted] = paths.try_emplace(existing->getLocation().string(), parent);
        if (inserted) {
            existing->setParentID(canonicalParent);
            objects.emplace(parent, std::move(existing));
        } else if (objects.at(path->second)->getParentID() != canonicalParent) {
            throw_std_runtime_error("Audio layout refresh found conflicting existing container parents");
        }
        existingParents.emplace(parent, path->second);
        return path->second;
    }
};

LayoutRefreshService::LayoutRefreshService(std::shared_ptr<Database> database)
    : database(std::move(database))
{
}

std::shared_ptr<LayoutRefreshService::AudioLayoutCapture> LayoutRefreshService::getActiveCapture()
{
    const auto lock = std::scoped_lock(captureMutex);
    return captureThread == std::this_thread::get_id() ? activeCapture.lock() : nullptr;
}

bool LayoutRefreshService::captureItem(const std::shared_ptr<CdsObject>& object)
{
    auto capture = getActiveCapture();
    if (!capture)
        return false;
    capture->addItem(object);
    return true;
}

std::optional<std::pair<int, bool>> LayoutRefreshService::captureContainerTree(
    const std::shared_ptr<ImportService>& importService,
    int rootId,
    const std::vector<std::shared_ptr<CdsObject>>& chain,
    const std::shared_ptr<CdsObject>& refItem)
{
    auto capture = getActiveCapture();
    if (!capture)
        return {};

    std::vector<int> createdIds;
    return importService->addContainerTree(rootId, chain, refItem, createdIds,
        [capture](int parent, const auto& object, const auto& path) {
            return capture->addContainer(parent, object, path);
        });
}

LayoutRefreshService::IndexedAudioLayout LayoutRefreshService::collectIndexedAudioLayout() const
{
    std::unordered_set<int> physicalIds;
    database->getObjects(CDS_ID_FS_ROOT, false, physicalIds, true, INVALID_OBJECT_ID);

    IndexedAudioLayout layout;
    for (const auto id : physicalIds) {
        const auto source = database->loadObject(id);
        if (source->getEntryType() != CdsEntryType::File || source->getMediaType() != ObjectType::Audio)
            continue;

        // Exclude playlist branches and user-managed entries.
        bool hasLayout = false;
        for (const auto refId : database->getRefObjects(id, CdsEntryType::VirtualItem)) {
            const auto reference = database->loadObject(refId);
            if (reference->getSource() != ObjectSource::Import)
                continue;
            std::vector<std::shared_ptr<CdsObject>> branch { reference };
            auto parent = reference->getParentID();
            while (parent != CDS_ID_ROOT && !layout.objects.count(parent)) {
                const auto container = database->loadObject(parent);
                if (container->getEntryType() != CdsEntryType::VirtualContainer
                    || container->isSubClass(UPNP_CLASS_PLAYLIST_CONTAINER)
                    || container->getSource() != ObjectSource::Import)
                    break;
                branch.push_back(container);
                parent = container->getParentID();
            }
            if (parent != CDS_ID_ROOT && !layout.objects.count(parent))
                continue;
            for (const auto& object : branch)
                layout.objects.emplace(object->getID(), object);
            hasLayout = true;
        }

        if (hasLayout)
            layout.sources.push_back(source);
    }
    return layout;
}

std::vector<std::shared_ptr<CdsObject>> LayoutRefreshService::planAudioLayoutRefresh(const ImportServiceResolver& resolveImportService)
{
    const auto current = collectIndexedAudioLayout();
    auto capture = std::make_shared<AudioLayoutCapture>(database);
    {
        // Never hold this lock while invoking layouts or their synchronous callbacks.
        const auto lock = std::scoped_lock(captureMutex);
        if (!activeCapture.expired())
            throw_std_runtime_error("Audio layout refresh is already capturing a layout");
        captureThread = std::this_thread::get_id();
        // The local owner restores ordinary imports on every return or exception.
        activeCapture = capture;
    }
    for (const auto& source : current.sources) {
        capture->trackId = source->getID();
        const auto parent = std::dynamic_pointer_cast<CdsContainer>(database->loadObject(source->getParentID()));
        resolveImportService(source)->refreshAudioLayout(source, parent);
    }

    // Only containers leading to captured tracks participate in the comparison.
    std::map<int, std::shared_ptr<CdsObject>> desired;
    for (const auto& [id, item] : capture->objects) {
        if (!item->isItem())
            continue;
        auto ancestor = id;
        while (ancestor != CDS_ID_ROOT && !desired.count(ancestor)) {
            const auto& object = capture->objects.at(ancestor);
            desired.emplace(ancestor, object);
            ancestor = object->getParentID();
        }
    }

    std::vector<std::shared_ptr<CdsObject>> changes;
    const LayoutGraph currentGraph(current.objects);
    const LayoutGraph desiredGraph(desired);
    currentGraph.matchChildren(desiredGraph, CDS_ID_ROOT, CDS_ID_ROOT, changes);
    // Check destinations and children that are outside the matched audio layout.
    std::unordered_set<int> renamedPaths;
    for (const auto& object : changes) {
        if (object->isContainer() && object->getLocation() != current.objects.at(object->getID())->getLocation())
            renamedPaths.insert(object->getID());
    }

    for (const auto& object : changes) {
        if (!renamedPaths.count(object->getID()))
            continue;
        const auto collision = database->findObjectByPath(object->getLocation(), UNUSED_CLIENT_GROUP, DbFileType::Virtual);
        if (collision && !renamedPaths.count(collision->getID()))
            throw_std_runtime_error("An existing container already uses {}", object->getLocation().string());

        auto children = BrowseParam(current.objects.at(object->getID()), BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS | BROWSE_CONTAINERS);
        children.setDynamicContainers(false);
        for (const auto& child : database->browse(children)) {
            if (!current.objects.count(child->getID()))
                throw_std_runtime_error("A separate entry would be affected: {}", child->getTitle());
        }
    }
    return changes;
}
