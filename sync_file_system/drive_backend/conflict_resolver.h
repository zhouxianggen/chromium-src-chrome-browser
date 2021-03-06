// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CONFLICT_RESOLVER_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CONFLICT_RESOLVER_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/sync_file_system/sync_callbacks.h"
#include "chrome/browser/sync_file_system/sync_task.h"
#include "google_apis/drive/gdata_errorcode.h"

namespace drive {
class DriveServiceInterface;
}

namespace google_apis {
class ResourceEntry;
}

namespace sync_file_system {
namespace drive_backend {

class MetadataDatabase;
class SyncEngineContext;
class TrackerSet;

// Resolves server side file confliction.
// If a remote file has an active tracker and multiple managed parents,
// ConflictResolver detaches the file from all parents other than the parent
// of the active tracker.
// If multiple trackers have the same local path or the same remote file,
// ConflictResolver picks up one of them and delete others.
class ConflictResolver : public SyncTask {
 public:
  explicit ConflictResolver(SyncEngineContext* sync_context);
  virtual ~ConflictResolver();
  virtual void Run(const SyncStatusCallback& callback) OVERRIDE;

 private:
  typedef std::pair<std::string, std::string> FileIDAndETag;

  void DetachFromNonPrimaryParents(const SyncStatusCallback& callback);
  void DidDetachFromParent(const SyncStatusCallback& callback,
                           google_apis::GDataErrorCode error);

  std::string PickPrimaryFile(const TrackerSet& trackers);
  void RemoveNonPrimaryFiles(const SyncStatusCallback& callback);
  void DidRemoveFile(const SyncStatusCallback& callback,
                     const std::string& file_id,
                     google_apis::GDataErrorCode error);
  void DidUpdateDatabaseForRemovedFile(const SyncStatusCallback& callback,
                                       SyncStatusCode status);

  std::string target_file_id_;
  std::vector<std::string> parents_to_remove_;

  std::vector<FileIDAndETag> non_primary_file_ids_;

  bool IsContextReady();
  drive::DriveServiceInterface* drive_service();
  MetadataDatabase* metadata_database();

  SyncEngineContext* sync_context_;  // Not owned.

  base::WeakPtrFactory<ConflictResolver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConflictResolver);
};

}  // namespace drive_backend
}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CONFLICT_RESOLVER_H_
