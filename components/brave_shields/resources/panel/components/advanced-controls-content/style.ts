import styled from 'styled-components'
import globalIconUrl from '../../../../../web-components/icons/globe.svg'
import listsIconUrl from '../../../../../web-components/icons/lists.svg'

export const SettingsBox = styled.div`
  padding: 17px 22px;

  a {
    font-size: 13px;
    font-weight: 600;
    text-decoration: none;
    display: flex;
    align-items: center;
    margin-bottom: 20px;

    &:last-child {
      margin-bottom: 0;
    }
  }
`

export const SettingsDesc = styled.p`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const GlobeIcon = styled.i`
  display: block;
  width: 17px;
  height: 17px;
  background-image: url(${globalIconUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  margin-right: 5px;
`

export const ListIcon = styled(GlobeIcon)`
  height: 12px;
  background-image: url(${listsIconUrl});
`

export const ControlGroup = styled.div`
  display: grid;
  grid-gap: 15px;
  grid-template-columns: 1fr 30px;
  align-items: center;
  margin-bottom: 10px;

  select,
  label {
    width: 100%;
  }

  label {
    display: flex;
    align-items: center;
    justify-content: space-between;
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 12px;
    color: ${(p) => p.theme.color.text01};

    span {
      margin-right: 5px;
    }
  }
`

export const CountButton = styled.button`
  background-color: transparent;
  border: 0;
  padding: 0;
  text-align: center;
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;
  color: ${(p) => p.theme.color.interactive05};
  padding: 5px 6px;
  max-width: 30px;
  border-radius: 4px;
  border: 2px solid transparent;

  &:hover {
    background-color: ${(p) => p.theme.color.disabled};
  }

  &:focus-visible {
    border-color: ${(p) => p.theme.color.focusBorder};
  }

  &:disabled,
  [disabled] {
    color: ${(p) => p.theme.color.disabled};
  }

`
