////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_STORAGE_ENGINE_STORAGE_ENGINE_FEATURE_H
#define ARANGOD_STORAGE_ENGINE_STORAGE_ENGINE_FEATURE_H 1

#include "ApplicationFeatures/ApplicationFeature.h"
#include "Basics/Common.h"

namespace arangodb {

// a stub class that other features can use to check whether a storage
// engine (no matter what type) is ready
class StorageEngineFeature : public application_features::ApplicationFeature {
 public:
  explicit StorageEngineFeature(application_features::ApplicationServer& server)
      : application_features::ApplicationFeature(server, "StorageEngine") {
    setOptional(false);
    startsAfter("BasicsPhase");
  }
};

}  // namespace arangodb

#endif
