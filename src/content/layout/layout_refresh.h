/*GRB*

    Gerbera - https://gerbera.io/

    layout_refresh.h - this file is part of Gerbera.

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

/// @file content/layout/layout_refresh.h

#ifndef __LAYOUT_REFRESH_H__
#define __LAYOUT_REFRESH_H__

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

class CdsObject;
class Database;
class ImportService;

/// @brief Capture configured audio layouts and plan identity-preserving title updates.
class LayoutRefreshService {
public:
    /// @brief Select the configured importer for an indexed source.
    using ImportServiceResolver = std::function<std::shared_ptr<ImportService>(const std::shared_ptr<CdsObject>&)>;

    explicit LayoutRefreshService(std::shared_ptr<Database> database);

    /// @brief Generate verified title changes from unchanged indexed physical sources.
    /// @throws std::runtime_error If the layout cannot be matched without changing identities.
    std::vector<std::shared_ptr<CdsObject>> planAudioLayoutRefresh(const ImportServiceResolver& resolveImportService);

    /// @brief Record callbacks during capture; otherwise leave ordinary imports to the caller.
    bool captureItem(const std::shared_ptr<CdsObject>& object);
    /// @brief Prepare and record container callbacks, returning no result outside capture.
    std::optional<std::pair<int, bool>> captureContainerTree(
        const std::shared_ptr<ImportService>& importService,
        int rootId,
        const std::vector<std::shared_ptr<CdsObject>>& chain,
        const std::shared_ptr<CdsObject>& refItem);

private:
    using LayoutObjects = std::map<int, std::shared_ptr<CdsObject>>;
    class AudioLayoutCapture;
    class LayoutGraph;
    struct IndexedAudioLayout {
        LayoutObjects objects;
        std::vector<std::shared_ptr<CdsObject>> sources;
    };

    std::shared_ptr<Database> database;
    std::mutex captureMutex;
    std::thread::id captureThread;
    std::weak_ptr<AudioLayoutCapture> activeCapture;

    std::shared_ptr<AudioLayoutCapture> getActiveCapture();
    IndexedAudioLayout collectIndexedAudioLayout() const;
};

#endif // __LAYOUT_REFRESH_H__
