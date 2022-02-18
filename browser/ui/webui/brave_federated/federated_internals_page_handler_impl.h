/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_FEDERATED_INTERNALS_PAGE_HANDLER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_FEDERATED_INTERNALS_PAGE_HANDLER_IMPL_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/brave_federated/federated_internals.mojom.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/data_stores/ad_notification_timing_data_store.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

class FederatedInternalsPageHandlerImpl: public federated_internals::mojom::PageHandler {
  public:
    FederatedInternalsPageHandlerImpl(
        mojo::PendingReceiver<federated_internals::mojom::PageHandler> receiver, 
        mojo::PendingRemote<federated_internals::mojom::Page> page,
        Profile* profile);
    ~FederatedInternalsPageHandlerImpl() override;

    FederatedInternalsPageHandlerImpl(
      const FederatedInternalsPageHandlerImpl&) = delete;
    FederatedInternalsPageHandlerImpl& operator=(
        const FederatedInternalsPageHandlerImpl&) = delete;

    void GetAdStoreInfo() override;

  private:
    void OnAdStoreInfoAvailable(
        brave_federated::AdNotificationTimingDataStore::
          IdToAdNotificationTimingTaskLogMap ad_notification_timing_logs);
    
    mojo::Receiver<federated_internals::mojom::PageHandler> receiver_;
    mojo::Remote<federated_internals::mojom::Page> page_;
    brave_federated::DataStoreService* data_store_service_;

    base::WeakPtrFactory<FederatedInternalsPageHandlerImpl> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_FEDERATED_INTERNALS_PAGE_HANDLER_IMPL_H_