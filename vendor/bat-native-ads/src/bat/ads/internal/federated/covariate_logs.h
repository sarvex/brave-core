/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace base {
class Value;
}

namespace ads {

class CovariateLogEntry;

// |CovariateLogs| collect training data for federated services such as
// learning, tuning and evaluation. A row in the training data set is called
// "instance ". A column is called "feature". To differentiate between
// Chromium/griffin features and federated services features, we call them
// covariates instead. Covariate values can be of different data types as
// defined in |mojom::ads::Covariate|.
// All covariates are only session based at the moment, i.e no measurements
// are persistet across sessions
class CovariateLogs final {
 public:
  CovariateLogs();
  CovariateLogs(const CovariateLogs&) = delete;
  CovariateLogs& operator=(const CovariateLogs&) = delete;
  ~CovariateLogs();

  static CovariateLogs* Get();

  void SetCovariateLogEntry(std::unique_ptr<CovariateLogEntry> entry);
  mojom::TrainingInstancePtr GetTrainingInstance() const;

 private:
  base::flat_map<mojom::CovariateType, std::unique_ptr<CovariateLogEntry>>
      covariate_log_entries_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_H_
