// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRALLALA_
#define TRALLALA_

#include "brave/components/sync/protocol/vg_specifics.pb.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<ledger::mojom::VirtualGrantSpendStatusDataView, sync_pb::VgSpendStatusSpecifics>
{
  static uint64_t token_id(const sync_pb::VgSpendStatusSpecifics& vg_spend_status) { return vg_spend_status.token_id(); }
  static uint64_t redeemed_at(const sync_pb::VgSpendStatusSpecifics& vg_spend_status) { return vg_spend_status.redeemed_at(); }
  static ledger::mojom::RewardsType redeem_type(const sync_pb::VgSpendStatusSpecifics& vg_spend_status) { return static_cast<ledger::mojom::RewardsType>(vg_spend_status.redeem_type()); }

  static bool Read(ledger::mojom::VirtualGrantSpendStatusDataView data, sync_pb::VgSpendStatusSpecifics* out);
};

}  // namespace mojo

#endif  // TRALLALA_
