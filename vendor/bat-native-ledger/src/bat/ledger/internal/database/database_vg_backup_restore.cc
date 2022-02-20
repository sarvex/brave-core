/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <numeric>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/database/database_vg_backup_restore.h"
#include "bat/ledger/internal/ledger_impl.h"

template <typename Key, typename Value>
std::vector<Key> GetKeys(const std::map<Key, Value>& map) {
  std::vector<Key> keys{};
  keys.reserve(map.size());

  for (const auto& pair : map) {
    keys.push_back(pair.first);
  }

  return keys;
}

namespace ledger {
namespace database {

DatabaseVGBackupRestore::DatabaseVGBackupRestore(LedgerImpl* ledger)
    : DatabaseTable(ledger) {}

DatabaseVGBackupRestore::~DatabaseVGBackupRestore() = default;

void DatabaseVGBackupRestore::BackUpVGBody(
    type::CredsBatchType trigger_type,
    const std::string& trigger_id,
    BackUpVGBodyCallback callback) const {
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = R"(
    SELECT   cb.creds_id,
             cb.trigger_type,
             cb.creds,
             cb.blinded_creds,
             cb.signed_creds,
             cb.public_key,
             cb.batch_proof,
             cb.status,
             ut.token_id,
             ut.token_value,
             ut.value,
             ut.expires_at
    FROM     creds_batch AS cb
    JOIN     unblinded_tokens AS ut
    ON       ut.creds_id = cb.creds_id
    WHERE    cb.trigger_type = ? AND cb.trigger_id = ?
    ORDER BY ut.token_id
  )";

  BindInt(command.get(), 0, static_cast<int>(trigger_type));
  BindString(command.get(), 1, trigger_id);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,  // creds_id
      type::DBCommand::RecordBindingType::INT_TYPE,     // trigger_type
      type::DBCommand::RecordBindingType::STRING_TYPE,  // creds
      type::DBCommand::RecordBindingType::STRING_TYPE,  // blinded_creds
      type::DBCommand::RecordBindingType::STRING_TYPE,  // signed_creds
      type::DBCommand::RecordBindingType::STRING_TYPE,  // public_key
      type::DBCommand::RecordBindingType::STRING_TYPE,  // batch_proof
      type::DBCommand::RecordBindingType::INT_TYPE,     // status
      type::DBCommand::RecordBindingType::INT64_TYPE,   // token_id
      type::DBCommand::RecordBindingType::STRING_TYPE,  // token_value
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      type::DBCommand::RecordBindingType::INT64_TYPE};  // expires_at

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseVGBackupRestore::OnBackUpVGBody,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseVGBackupRestore::OnBackUpVGBody(
    BackUpVGBodyCallback callback,
    type::DBCommandResponsePtr response) const {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "BackUpVGBodyForTrigger failed: bad response!");
    return std::move(callback).Run({});
  }

  const auto& records = response->result->get_records();
  if (records.empty()) {
    BLOG(0, "BackUpVGBodyForTrigger failed: empty result set!");
    return std::move(callback).Run({});
  }

  auto* record_pointer = records[0].get();

  auto vg_body = type::VirtualGrantBody::New();
  vg_body->creds_id = GetStringColumn(record_pointer, 0);
  vg_body->trigger_type =
      static_cast<type::CredsBatchType>(GetIntColumn(record_pointer, 1));
  vg_body->creds = GetStringColumn(record_pointer, 2);
  vg_body->blinded_creds = GetStringColumn(record_pointer, 3);
  vg_body->signed_creds = GetStringColumn(record_pointer, 4);
  vg_body->public_key = GetStringColumn(record_pointer, 5);
  vg_body->batch_proof = GetStringColumn(record_pointer, 6);
  vg_body->status =
      static_cast<type::CredsBatchStatus>(GetIntColumn(record_pointer, 7));

  for (const auto& record : records) {
    record_pointer = record.get();

    auto token = type::VirtualGrantBodyTokenInfo::New();
    token->token_id = GetInt64Column(record_pointer, 8);
    token->token_value = GetStringColumn(record_pointer, 9);
    token->value = GetDoubleColumn(record_pointer, 10);
    token->expires_at = GetInt64Column(record_pointer, 11);

    vg_body->tokens.emplace_back(std::move(token));
  }

  std::move(callback).Run(std::move(vg_body));
}

void DatabaseVGBackupRestore::BackUpVGSpendStatuses(
    BackUpVGSpendStatusesCallback callback) const {
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = R"(
    WITH aux AS (
      SELECT SUM(
               CASE
                 WHEN redeem_id IS NOT NULL AND redeem_id != '' AND redeemed_at = 0 THEN 1
                 ELSE 0
               END
             ) AS in_progress
      FROM   unblinded_tokens
    )
    SELECT   NULL AS token_id,
             NULL AS redeemed_at,
             NULL AS redeem_type
    FROM     aux
    WHERE    aux.in_progress != 0
    UNION ALL
    SELECT   ut.token_id,
             ut.redeemed_at,
             ut.redeem_type
    FROM     creds_batch AS cb, aux
    JOIN     unblinded_tokens AS ut
    ON       ut.creds_id = cb.creds_id
    WHERE    aux.in_progress = 0
    ORDER BY ut.token_id
  )";

  command->record_bindings = {
      type::DBCommand::RecordBindingType::INT64_TYPE,   // token_id
      type::DBCommand::RecordBindingType::INT64_TYPE,   // redeemed_at
      type::DBCommand::RecordBindingType::INT64_TYPE};  // redeem_type

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseVGBackupRestore::OnBackUpVGSpendStatuses,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseVGBackupRestore::OnBackUpVGSpendStatuses(
    BackUpVGSpendStatusesCallback callback,
    type::DBCommandResponsePtr response) const {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "BackUpVGSpendStatus failed: bad response!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, {});
  }

  const auto& records = response->result->get_records();
  if (records.size() == 1 && AllNULLRecord(records[0])) {
    BLOG(1,
         "BackUpVGSpendStatus failed: there's at least one contribution or SKU "
         "order in progress.");
    return std::move(callback).Run(type::Result::IN_PROGRESS, {});
  }

  std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses;

  for (const auto& record : records) {
    auto* record_pointer = record.get();

    sync_pb::VgSpendStatusSpecifics vg_spend_status;
    vg_spend_status.set_token_id(GetInt64Column(record_pointer, 0));
    vg_spend_status.set_redeemed_at(GetInt64Column(record_pointer, 1));
    vg_spend_status.set_redeem_type(GetInt64Column(record_pointer, 2));

    vg_spend_statuses.emplace_back(std::move(vg_spend_status));
  }

  std::move(callback).Run(type::Result::LEDGER_OK,
                          std::move(vg_spend_statuses));
}

bool DatabaseVGBackupRestore::AllNULLRecord(
    const type::DBRecordPtr& record) const {
  DCHECK(record);
  auto* record_pointer = record.get();

  return !GetInt64Column(record_pointer, 0) &&  // token_id
         !GetInt64Column(record_pointer, 1) &&  // redeemed_at
         !GetInt64Column(record_pointer, 2);    // redeem_type
}

void DatabaseVGBackupRestore::RestoreVGs(type::VirtualGrants&& vgs,
                                         RestoreVGsCallback callback) const {
  Tables tables{};
  tables["creds_batch"];
  tables["unblinded_tokens"];

  GetCreateTableStatements(std::move(tables), std::move(vgs),
                           std::move(callback));
}

void DatabaseVGBackupRestore::GetCreateTableStatements(
    Tables&& tables,
    type::VirtualGrants&& vgs,
    RestoreVGsCallback callback) const {
  DCHECK(!tables.empty());

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT tbl_name, sql "
      "FROM sqlite_master "
      "WHERE tbl_name IN (%s) "
      "AND type = 'table'",
      GenerateStringInCase(GetKeys(tables)).c_str());
  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE};

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseVGBackupRestore::OnGetCreateTableStatements,
                     base::Unretained(this), std::move(tables), std::move(vgs),
                     std::move(callback)));
}

void DatabaseVGBackupRestore::OnGetCreateTableStatements(
    Tables&& tables,
    type::VirtualGrants&& vgs,
    RestoreVGsCallback callback,
    type::DBCommandResponsePtr response) const {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Couldn't get CREATE TABLE statements for tables!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  for (const auto& record : response->result->get_records()) {
    tables[GetStringColumn(record.get(), 0)] = GetStringColumn(record.get(), 1);
  }

  GetCreateIndexStatements(std::move(tables), std::move(vgs),
                           std::move(callback));
}

void DatabaseVGBackupRestore::GetCreateIndexStatements(
    Tables&& tables,
    type::VirtualGrants&& vgs,
    RestoreVGsCallback callback) const {
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT name, sql "
      "FROM sqlite_master "
      "WHERE tbl_name IN (%s) "
      "AND type = 'index' "
      "AND sql IS NOT NULL",
      GenerateStringInCase(GetKeys(tables)).c_str());
  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE};

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseVGBackupRestore::OnGetCreateIndexStatements,
                     base::Unretained(this), std::move(tables), std::move(vgs),
                     std::move(callback)));
}

void DatabaseVGBackupRestore::OnGetCreateIndexStatements(
    Tables&& tables,
    const type::VirtualGrants& vgs,
    RestoreVGsCallback callback,
    type::DBCommandResponsePtr response) const {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Couldn't get CREATE INDEX statements for tables!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  Indices indices{};
  for (const auto& record : response->result->get_records()) {
    indices[GetStringColumn(record.get(), 0)] =
        GetStringColumn(record.get(), 1);
  }

  RestoreVGs(tables, indices, vgs, std::move(callback));
}

void DatabaseVGBackupRestore::AlterTables(
    const Tables& tables,
    type::DBTransaction& transaction) const {
  for (const auto& table : tables) {
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::EXECUTE;
    command->command = "ALTER TABLE " + table.first + " RENAME TO " +
                       table.first + '_' +
                       std::to_string(util::GetCurrentTimeStamp());

    transaction.commands.push_back(std::move(command));
  }
}

void DatabaseVGBackupRestore::DropIndices(
    const Indices& indices,
    type::DBTransaction& transaction) const {
  for (const auto& index : indices) {
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::EXECUTE;
    command->command = "DROP INDEX IF EXISTS " + index.first;
    transaction.commands.push_back(std::move(command));
  }
}

void DatabaseVGBackupRestore::CreateTables(
    const Tables& tables,
    type::DBTransaction& transaction) const {
  for (const auto& table : tables) {
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::EXECUTE;
    command->command = table.second;
    transaction.commands.push_back(std::move(command));
  }
}

void DatabaseVGBackupRestore::CreateIndices(
    const Indices& indices,
    type::DBTransaction& transaction) const {
  for (const auto& index : indices) {
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::EXECUTE;
    command->command = index.second;
    transaction.commands.push_back(std::move(command));
  }
}

void DatabaseVGBackupRestore::RestoreVGs(const Tables& tables,
                                         const Indices& indices,
                                         const type::VirtualGrants& vgs,
                                         RestoreVGsCallback callback) const {
  auto transaction = type::DBTransaction::New();
  AlterTables(tables, *transaction);
  DropIndices(indices, *transaction);
  CreateTables(tables, *transaction);
  CreateIndices(indices, *transaction);

  for (auto creds_id_cit = vgs.cbegin(); creds_id_cit != vgs.cend();
       creds_id_cit = vgs.upper_bound(creds_id_cit->first)) {
    const auto& creds_id = creds_id_cit->first;
    auto creds_id_range = vgs.equal_range(creds_id);
    const auto& vg = *creds_id_range.first->second;

    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::RUN;
    command->command = R"(
      INSERT INTO creds_batch (
        creds_id,
        trigger_id,
        trigger_type,
        creds,
        blinded_creds,
        signed_creds,
        public_key,
        batch_proof,
        status
      )
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    BindString(command.get(), 0, vg.creds_id);
    BindString(command.get(), 1, "");  // trigger_id
    BindInt(command.get(), 2, static_cast<int>(vg.trigger_type));
    BindString(command.get(), 3, vg.creds);
    BindString(command.get(), 4, vg.blinded_creds);
    BindString(command.get(), 5, vg.signed_creds);
    BindString(command.get(), 6, vg.public_key);
    BindString(command.get(), 7, vg.batch_proof);
    BindInt(command.get(), 8, static_cast<int>(vg.status));

    transaction->commands.push_back(std::move(command));

    do {
      const auto& vg = *creds_id_range.first->second;

      command = type::DBCommand::New();
      command->type = type::DBCommand::Type::RUN;
      command->command = R"(
        INSERT INTO unblinded_tokens (
          token_id,
          token_value,
          public_key,
          value,
          creds_id,
          expires_at,
          redeemed_at,
          redeem_id,
          redeem_type,
          reserved_at
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
      )";
      BindInt64(command.get(), 0, vg.token_id);
      BindString(command.get(), 1, vg.token_value);
      BindString(command.get(), 2, vg.public_key);
      BindDouble(command.get(), 3, vg.value);
      BindString(command.get(), 4, vg.creds_id);
      BindInt64(command.get(), 5, vg.expires_at);
      BindInt64(command.get(), 6, vg.redeemed_at);
      BindString(command.get(), 7, "");  // redeem_id
      BindInt64(command.get(), 8, static_cast<int>(vg.redeem_type));
      BindInt64(command.get(), 9, 0);  // reserved_at

      transaction->commands.push_back(std::move(command));
    } while (++creds_id_range.first != creds_id_range.second);
  }

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseVGBackupRestore::OnRestoreVGs,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseVGBackupRestore::OnRestoreVGs(
    RestoreVGsCallback callback,
    type::DBCommandResponsePtr response) const {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Restore failed!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  std::move(callback).Run(type::Result::LEDGER_OK);
}

}  // namespace database
}  // namespace ledger
