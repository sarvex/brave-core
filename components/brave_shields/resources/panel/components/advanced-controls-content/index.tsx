import * as React from 'react'

import * as S from './style'
import Toggle from '../../../../../web-components/toggle'
import { getLocale } from '../../../../../common/locale'
import getPanelBrowserAPI, { AdBlockMode, CookieBlockMode, FingerprintMode } from '../../api/panel_browser_api'
import DataContext from '../../state/context'
import { ViewType } from '../../state/component_types'

const adBlockModeOptions = [
  { value: AdBlockMode.AGGRESSIVE, text: getLocale('braveShieldsTrackersAndAdsBlockedAgg') },
  { value: AdBlockMode.STANDARD, text: getLocale('braveShieldsTrackersAndAdsBlockedStd') },
  { value: AdBlockMode.ALLOW, text: getLocale('braveShieldsTrackersAndAdsAllowAll') }
]

const cookieBlockModeOptions = [
  { value: CookieBlockMode.CROSS_SITE_BLOCKED, text: getLocale('braveShieldsCrossCookiesBlocked') },
  { value: CookieBlockMode.BLOCKED, text: getLocale('braveShieldsCookiesBlocked') },
  { value: CookieBlockMode.ALLOW, text: getLocale('braveShieldsCookiesBlockedAll') }
]

const fingerprintModeOptions = [
  { value: FingerprintMode.STANDARD, text: getLocale('braveShieldsFingerprintingBlockedStd') },
  { value: FingerprintMode.STRICT, text: getLocale('braveShieldsFingerprintingBlockedAgg') },
  { value: FingerprintMode.ALLOW, text: getLocale('braveShieldsFingerprintingAllowAll') }
]

function GlobalSettings () {
  const handleAnchorClick = (e: React.MouseEvent<HTMLAnchorElement>) => {
    e.preventDefault()
    const target = e.target as HTMLAnchorElement
    getPanelBrowserAPI().panelHandler.openURL({ url: target.href })
  }

  return (
    <S.SettingsBox>
      <a
        href='chrome://settings/shields'
        onClick={handleAnchorClick}
      >
        <S.GlobeIcon />
        {getLocale('braveShieldsChangeDefaults')}
      </a>
      <a
        href='chrome://adblock'
        onClick={handleAnchorClick}
      >
        <S.ListIcon />
        {getLocale('braveShieldsCustomizeAdblockLists')}
      </a>
    </S.SettingsBox>
  )
}

function AdvancedControlsContent () {
  const { siteBlockInfo, siteSettings, getSiteSettings, setViewType } = React.useContext(DataContext)

  const handleAdBlockModeChange = (e: React.FormEvent<HTMLSelectElement>) => {
    const target = e.target as HTMLSelectElement
    getPanelBrowserAPI().dataHandler.setAdBlockMode(parseInt(target.value))
    if (getSiteSettings) getSiteSettings()
  }

  const handleFingerprintModeChange = (e: React.FormEvent<HTMLSelectElement>) => {
    const target = e.target as HTMLSelectElement
    getPanelBrowserAPI().dataHandler.setFingerprintMode(parseInt(target.value))
    if (getSiteSettings) getSiteSettings()
  }

  const handleCookieBlockModeChange = (e: React.FormEvent<HTMLSelectElement>) => {
    const target = e.target as HTMLSelectElement
    getPanelBrowserAPI().dataHandler.setCookieBlockMode(parseInt(target.value))
    if (getSiteSettings) getSiteSettings()
  }

  const handleIsNoScriptEnabledChange = (isEnabled: boolean) => {
    getPanelBrowserAPI().dataHandler.setIsNoScriptsEnabled(isEnabled)
    if (getSiteSettings) getSiteSettings()
  }

  const handleHTTPSEverywhereEnabledChange = (isEnabled: boolean) => {
    getPanelBrowserAPI().dataHandler.setHTTPSEverywhereEnabled(isEnabled)
    if (getSiteSettings) getSiteSettings()
  }

  const adsListCount = siteBlockInfo?.adsList.length
  const httpRedirectsListCount = siteBlockInfo?.httpRedirectsList.length
  const jsListCount = siteBlockInfo?.jsList.length

  return (
    <section
      id='advanced-controls-content'
      aria-label={getLocale('braveShieldsAdvancedCtrls')}
    >
      <S.SettingsBox>
        <S.ControlGroup>
          <select
            value={siteSettings?.adBlockMode}
            onChange={handleAdBlockModeChange}
            aria-label={getLocale('braveShieldsTrackersAndAds')}
          >
            {adBlockModeOptions.map((entry, i) => {
              return (
                <option value={entry.value} key={i}>
                  {entry.text}
                </option>
              )
            })}
          </select>
          <S.CountButton
            aria-label={getLocale('braveShieldsTrackersAndAds')}
            onClick={() => setViewType?.(ViewType.AdsList)}
          >
            <span>{adsListCount}</span>
          </S.CountButton>
        </S.ControlGroup>
        <S.ControlGroup>
          <label>
            <span>{getLocale('braveShieldsConnectionsUpgraded')}</span>
            <Toggle
              onChange={handleHTTPSEverywhereEnabledChange}
              isOn={siteSettings?.isHttpsEverywhereEnabled}
              size='sm'
              accessibleLabel='Enable HTTPS'
            />
          </label>
          <S.CountButton
            aria-label={getLocale('braveShieldsConnectionsUpgraded')}
            onClick={() => setViewType?.(ViewType.HttpsList)}
          >
            {httpRedirectsListCount}
          </S.CountButton>
        </S.ControlGroup>
        <S.ControlGroup>
          <label>
            <span>{getLocale('braveShieldsScriptsBlocked')}</span>
            <Toggle
              onChange={handleIsNoScriptEnabledChange}
              isOn={siteSettings?.isNoscriptEnabled}
              size='sm'
              accessibleLabel={getLocale('braveShieldsScriptsBlockedEnable')}
            />
          </label>
          <S.CountButton
            aria-label={getLocale('braveShieldsScriptsBlocked')}
            onClick={() => setViewType?.(ViewType.ScriptsList)}
          >
            {jsListCount}
          </S.CountButton>
        </S.ControlGroup>
        <S.ControlGroup>
          <select
            onChange={handleFingerprintModeChange}
            value={siteSettings?.fingerprintMode}
            aria-label={getLocale('braveShieldsFingerprintingBlocked')}
          >
            {fingerprintModeOptions.map((entry, i) => {
                return (
                  <option value={entry.value} key={i}>
                    {entry.text}
                  </option>
                )
              })}
          </select>
        </S.ControlGroup>
        <S.ControlGroup>
          <select
            onChange={handleCookieBlockModeChange}
            value={siteSettings?.cookieBlockMode}
            aria-label={getLocale('braveShieldsCookiesBlocked')}
          >
              {cookieBlockModeOptions.map((entry, i) => {
                return (
                  <option value={entry.value} key={i}>
                    {entry.text}
                  </option>
                )
              })}
          </select>
        </S.ControlGroup>
        <S.SettingsDesc>{getLocale('braveShieldSettingsDescription')}</S.SettingsDesc>
      </S.SettingsBox>
      <GlobalSettings />
  </section>
  )
}

export default AdvancedControlsContent
