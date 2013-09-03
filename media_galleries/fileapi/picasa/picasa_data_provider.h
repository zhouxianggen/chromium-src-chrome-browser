// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_PICASA_PICASA_DATA_PROVIDER_H_
#define CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_PICASA_PICASA_DATA_PROVIDER_H_

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/common/media_galleries/picasa_types.h"

namespace picasa {

class SafePicasaAlbumTableReader;
class SafePicasaAlbumsIndexer;

// Created and owned by ImportedMediaGalleryRegistryTaskRunnerValues
class PicasaDataProvider {
 public:
  typedef base::Callback<void(bool)> ReadyCallback;

  enum DataType {
    LIST_OF_ALBUMS_AND_FOLDERS_DATA,
    ALBUMS_IMAGES_DATA
  };

  explicit PicasaDataProvider(const base::FilePath& database_path);
  virtual ~PicasaDataProvider();

  // Ask the data provider to refresh the data if necessary. |ready_callback|
  // will be called when the data is up to date.
  // TODO(tommycli): Investigate having the callback return a bool indicating
  // success or failure - and handling it intelligently in PicasaFileUtil.
  void RefreshData(DataType needed_data, const ReadyCallback& ready_callback);

  // These methods return scoped_ptrs because we want to return a copy that
  // will not change to the caller.
  scoped_ptr<AlbumMap> GetAlbums();
  scoped_ptr<AlbumMap> GetFolders();
  // |error| must be non-NULL.
  scoped_ptr<AlbumImages> FindAlbumImages(const std::string& key,
                                          base::PlatformFileError* error);

 private:
  enum State {
    STALE_DATA_STATE,
    INVALID_DATA_STATE,
    LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE,
    ALBUMS_IMAGES_FRESH_STATE
  };

  friend class PicasaFileUtilTest;
  friend class PicasaDataProviderInvalidateSimpleTest;
  friend class PicasaDataProviderInvalidateInflightTableReaderTest;
  friend class PicasaDataProviderInvalidateInflightAlbumsIndexerTest;

  // Notifies data provider that any currently cached data is stale.
  void InvalidateData();

  // Kicks off utility processes needed to fulfill any pending callbacks.
  void DoRefreshIfNecessary();

  void OnAlbumTableReaderDone(scoped_refptr<SafePicasaAlbumTableReader> reader,
                              bool parse_success,
                              const std::vector<AlbumInfo>& albums,
                              const std::vector<AlbumInfo>& folder);

  void OnAlbumsIndexerDone(scoped_refptr<SafePicasaAlbumsIndexer> indexer,
                           bool success,
                           const picasa::AlbumImagesMap& albums_images);

  static std::string DateToPathString(const base::Time& time);
  static void UniquifyNames(const std::vector<AlbumInfo>& info_list,
                            AlbumMap* result_map);

  // Methods are used in the browser test to tweak internal data for testing.
  void SetDatabasePathForTesting(const base::FilePath& database_path);
  void SetAlbumMapsForTesting(const AlbumMap& album_map,
                              const AlbumMap& folder_map);

  AlbumMap album_map_;
  AlbumMap folder_map_;
  AlbumImagesMap albums_images_;

  base::FilePath database_path_;

  State state_;

  // Callbacks that are waiting for their requested data to be ready.
  std::queue<ReadyCallback> album_list_ready_callbacks_;
  std::queue<ReadyCallback> albums_index_ready_callbacks_;

  // Stores the "live" in-flight utility processes. Callbacks from other
  // (older) utility processes are stale and ignored. Only one of these at a
  // time should be non-NULL.
  scoped_refptr<SafePicasaAlbumTableReader> album_table_reader_;
  scoped_refptr<SafePicasaAlbumsIndexer> albums_indexer_;

  base::WeakPtrFactory<PicasaDataProvider> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PicasaDataProvider);
};

}  // namespace picasa

#endif  // CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_PICASA_PICASA_DATA_PROVIDER_H_
