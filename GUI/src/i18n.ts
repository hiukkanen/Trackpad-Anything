// UI languages. Korean source strings are the keys; locales.json holds en/ja/zh.
// The same file is bundled into the app so the native host (menu bar, notices) uses identical text.
import table from './locales.json';

export type Lang = 'ko' | 'en' | 'ja' | 'zh';
export const languages: {value: string; label: string}[] = [
  {value: 'ko', label: '한국어'},
  {value: 'en', label: 'English'},
  {value: 'ja', label: '日本語'},
  {value: 'zh', label: '简体中文'},
];

const dict = table as Record<string, Record<string, string>>;
let current: Lang = 'ko';

export function setLang(lang: string | undefined) {
  current = lang === 'en' || lang === 'ja' || lang === 'zh' ? lang : 'ko';
}

export function lang(): Lang {
  return current;
}

/** Translate a Korean source string; {0}, {1}… are replaced by args. Missing → English → Korean. */
export function t(ko: string, ...args: (string | number)[]): string {
  let s = current === 'ko' ? ko : dict[current]?.[ko] ?? dict.en?.[ko] ?? ko;
  args.forEach((a, i) => { s = s.split(`{${i}}`).join(String(a)); });
  return s;
}
