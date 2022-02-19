// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<ledger::mojom::VirtualGrantSpendStatusDataView, sync_pb::VgSpendStatusSpecifics>::Read(ledger::mojom::VirtualGrantSpendStatusDataView data, sync_pb::VgSpendStatusSpecifics* out) {
  out->set_token_id(data.token_id());
  out->set_redeemed_at(data.redeemed_at());
  out->set_redeem_type(static_cast<std::int32_t>(data.redeem_type()));

  return true;
}

}  // namespace mojo
