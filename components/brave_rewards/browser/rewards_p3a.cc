/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_p3a.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/pref_names.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace {

uint64_t RoundProbiToUint64(base::StringPiece probi) {
  if (probi.size() < 18)
    return 0;
  uint64_t grant = 0;
  base::StringToUint64(probi.substr(0, probi.size() - 18), &grant);
  return grant;
}

}  // namespace

namespace brave_rewards {
namespace p3a {

void RecordWalletState(const WalletState& state) {
  int answer = 0;
  if (state.wallet_created && !state.rewards_enabled) {
    answer = 5;
  } else if (state.rewards_enabled) {
    DCHECK(state.wallet_created);
    if (state.grants_claimed && state.funds_added) {
      answer = 4;
    } else if (state.funds_added) {
      answer = 3;
    } else if (state.grants_claimed) {
      answer = 2;
    } else {
      answer = 1;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.WalletState", answer, 6);
}

void RecordWalletBalance(bool wallet_created, bool rewards_enabled, size_t b) {
  int answer = 0;
  if (wallet_created && !rewards_enabled) {
    answer = 1;
  } else if (rewards_enabled) {
    DCHECK(wallet_created);
    if (b == 0) {
      answer = 2;
    } else if (b < 10) {
      answer = 3;
    } else if (10 <= b && b < 50) {
      answer = 4;
    } else if (50 <= b) {
      answer = 5;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.WalletBalance.3", answer, 6);
}

void RecordAutoContributionsState(AutoContributionsState state, int count) {
  DCHECK_GE(count, 0);
  int answer = 0;
  switch (state) {
    case AutoContributionsState::kNoWallet:
      break;
    case AutoContributionsState::kRewardsDisabled:
      answer = 1;
      break;
    case AutoContributionsState::kWalletCreatedAutoContributeOff:
      answer = 2;
      break;
    case AutoContributionsState::kAutoContributeOn:
      switch (count) {
        case 0:
          answer = 3;
          break;
        case 1:
          answer = 4;
          break;
        default:
          answer = 5;
      }
      break;
    default:
      NOTREACHED();
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.AutoContributionsState.2", answer,
                             6);
}

void RecordTipsState(bool wallet_created,
                     bool rewards_enabled,
                     int one_time_count,
                     int recurring_count) {
  DCHECK_GE(one_time_count, 0);
  DCHECK_GE(recurring_count, 0);

  int answer = 0;
  if (wallet_created && !rewards_enabled) {
    answer = 1;
  } else if (rewards_enabled) {
    DCHECK(wallet_created);
    if (one_time_count == 0 && recurring_count == 0) {
      answer = 2;
    } else if (one_time_count > 0 && recurring_count > 0) {
      answer = 5;
    } else if (one_time_count > 0) {
      answer = 3;
    } else {
      answer = 4;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Rewards.TipsState.2", answer, 6);
}

void RecordAdsState(AdsState state) {
  UMA_HISTOGRAM_ENUMERATION("Brave.Rewards.AdsState.2", state);
}

void UpdateAdsStateOnPreferenceChange(PrefService* prefs,
                                      const std::string& pref) {
  const bool ads_enabled = prefs->GetBoolean(ads::prefs::kEnabled);
  if (pref == ads::prefs::kEnabled) {
    if (ads_enabled) {
      RecordAdsState(AdsState::kAdsEnabled);
      prefs->SetBoolean(brave_ads::prefs::kAdsWereDisabled, false);
    } else {
      // Apparently, the pref was disabled.
      RecordAdsState(AdsState::kAdsEnabledThenDisabledRewardsOn);
      prefs->SetBoolean(brave_ads::prefs::kAdsWereDisabled, true);
    }
  }
}

void MaybeRecordInitialAdsState(PrefService* prefs) {
  if (!prefs->GetBoolean(brave_ads::prefs::kHasAdsP3AState)) {
    const bool ads_state = prefs->GetBoolean(ads::prefs::kEnabled);
    RecordAdsState(ads_state ? AdsState::kAdsEnabled : AdsState::kAdsDisabled);
    prefs->SetBoolean(brave_ads::prefs::kHasAdsP3AState, true);
  }
}

void RecordNoWalletCreatedForAllMetrics() {
  RecordWalletState({});
  RecordWalletBalance(false, false, 0);
  RecordAutoContributionsState(AutoContributionsState::kNoWallet, 0);
  RecordTipsState(false, false, 0, 0);
  RecordAdsState(AdsState::kNoWallet);
}

void RecordRewardsDisabledForSomeMetrics() {
  RecordWalletBalance(true, false, 1);
  RecordAutoContributionsState(AutoContributionsState::kRewardsDisabled, 0);
  RecordTipsState(true, false, 0, 0);
  // Ads state is handled separately.
}

double CalcWalletBalance(base::flat_map<std::string, double> wallets,
                         double user_funds) {
  double balance_minus_grant = 0.0;
  for (const auto& wallet : wallets) {
    // Skip anonymous and unblinded wallets, since they can contain grants.
    if (wallet.first == "anonymous" || wallet.first == "blinded") {
      continue;
    }
    balance_minus_grant += static_cast<size_t>(wallet.second);
  }

  // |user_funds| is the amount of user-funded BAT in the anonymous
  // wallet (ex: not grants).
  balance_minus_grant += user_funds;
  return balance_minus_grant;
}

void RecordRewardsEnabledDuration(PrefService* prefs, bool rewards_enabled) {
  base::Time enabled_timestamp = prefs->GetTime(prefs::kEnabledTimestamp);
  auto enabled_duration = RewardsEnabledDuration::kNever;

  if (enabled_timestamp.is_null()) {
    // No previous timestamp, so record one of the non-duration states.
    if (!rewards_enabled) {
      // Rewards have been disabled with no previous timestamp.
      // Probably they've been on since we started measuring.
      // Ignore this interval since we can't measure it and treat it
      // the same as never having enabled.
      enabled_duration = RewardsEnabledDuration::kNever;
    } else {
      // Rewards have been enabled.
      // Remember when so we can measure the duration on later changes.
      prefs->SetTime(prefs::kEnabledTimestamp, base::Time::Now());
      enabled_duration = RewardsEnabledDuration::kStillEnabled;
    }
  } else {
    // Previous timestamp available.
    if (!rewards_enabled) {
      // Rewards have been disabled. Record the duration they were on.
      // Set the threshold at three units so each bin represents the
      // nominal value as an order-of-magnitude: more than three days
      // is a week, more than three weeks is a month, and so on.
      constexpr int threshold = 3;
      constexpr int days_per_week = 7;
      constexpr double days_per_month = 30.44;  // average length
      base::TimeDelta duration = base::Time::Now() - enabled_timestamp;
      VLOG(1) << "Rewards disabled after " << duration;
      if (duration < base::Hours(threshold)) {
        enabled_duration = RewardsEnabledDuration::kHours;
      } else if (duration < base::Days(threshold)) {
        enabled_duration = RewardsEnabledDuration::kDays;
      } else if (duration < base::Days(threshold * days_per_week)) {
        enabled_duration = RewardsEnabledDuration::kWeeks;
      } else if (duration < base::Days(threshold * days_per_month)) {
        enabled_duration = RewardsEnabledDuration::kMonths;
      } else {
        enabled_duration = RewardsEnabledDuration::kQuarters;
      }
      // Null the timestamp so we're ready for a fresh measurement.
      prefs->SetTime(prefs::kEnabledTimestamp, base::Time());
    } else {
      // Rewards have been enabled. Overwrite the previous timestamp.
      // Normally we null the timestamp when rewards are disabled,
      // so typically we'll take the other `else` branch above instead.
      // We nevertheless mark a new timestamp here to maintain consistent
      // measurement even if our prefs change through some other path.
      prefs->SetTime(prefs::kEnabledTimestamp, base::Time::Now());
      enabled_duration = RewardsEnabledDuration::kStillEnabled;
    }
  }

  UMA_HISTOGRAM_ENUMERATION("Brave.Rewards.EnabledDuration", enabled_duration);
}

void ExtractAndLogStats(const base::DictionaryValue& dict) {
  const base::Value* probi_value =
      dict.FindPath({"walletProperties", "probi_"});
  if (!probi_value || !probi_value->is_string()) {
    LOG(WARNING) << "Bad ledger state";
    return;
  }

  // Get grants.
  const base::Value* grants_value = dict.FindKey("grants");
  uint64_t total_grants = 0;
  if (grants_value) {
    if (!grants_value->is_list()) {
      LOG(WARNING) << "Bad grant value in ledger_state.";
    } else {
      const auto& grants = grants_value->GetList();
      // Sum all grants.
      for (const auto& grant : grants) {
        if (!grant.is_dict()) {
          LOG(WARNING) << "Bad grant value in ledger_state.";
          continue;
        }
        const base::Value* grant_amount = grant.FindKey("probi_");
        const base::Value* grant_currency = grant.FindKey("altcurrency");
        if (grant_amount->is_string() && grant_currency->is_string() &&
            grant_currency->GetString() == "BAT") {
          // Some kludge computations because we don't want to be very precise
          // for P3A purposes. Assuming grants can't be negative and are
          // greater than 1 BAT.
          const std::string& grant_str = grant_amount->GetString();
          total_grants += RoundProbiToUint64(grant_str);
        }
      }
    }
  }
  const uint64_t total =
      RoundProbiToUint64(probi_value->GetString()) - total_grants;
  RecordWalletBalance(true, true, total);
}

}  // namespace p3a
}  // namespace brave_rewards
