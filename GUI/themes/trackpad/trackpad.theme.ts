// The trackpad theme. `astryx experimental theme editor` wrote this file once and never changes it again.
// The checked settings live in trackpad.settings.json and the generated trackpad.settings.ts;
// change them with `astryx experimental theme editor set trackpad ...`. Add your own changes below: they win over the settings.
import {defineTheme} from '@astryxdesign/core/theme';
import {trackpadSettings} from './trackpad.settings';

export const trackpadTheme = defineTheme({
  extends: trackpadSettings,
  name: 'trackpad',
});
