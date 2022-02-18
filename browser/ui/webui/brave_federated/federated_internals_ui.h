/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_FEDERATED_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_FEDERATED_INTERNALS_UI_H_


#include "brave/browser/ui/webui/brave_federated/federated_internals.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

class Profile;

namespace brave_federated {
class DataStoreService;
} // namespace brave_federated

class FederatedInternalsPageHandlerImpl;

class FederatedInternalsUI 
    : public ui::MojoWebUIController,
      public federated_internals::mojom::PageHandlerFactory {

 public:
  FederatedInternalsUI(content::WebUI* web_ui);
  ~FederatedInternalsUI() override;

  FederatedInternalsUI(const FederatedInternalsUI&) = delete;
  FederatedInternalsUI& operator=(const FederatedInternalsUI&) =
      delete;

  void BindInterface(mojo::PendingReceiver<federated_internals::mojom::PageHandlerFactory> receiver);

private:
  void CreatePageHandler(
      mojo::PendingRemote<federated_internals::mojom::Page> page,
      mojo::PendingReceiver<federated_internals::mojom::PageHandler> receiver) override;

  Profile* profile_;
  std::unique_ptr<FederatedInternalsPageHandlerImpl>
      federated_internals_page_handler_;
  mojo::Receiver<federated_internals::mojom::PageHandlerFactory>
      federated_internals_page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_FEDERATED_INTERNALS_UI_H_
