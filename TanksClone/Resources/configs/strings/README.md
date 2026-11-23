# Adding a language

No code changes are needed.

1. Copy `english.json` to `<name>.json` here and translate the values. Keep every key, the placeholders and the format codes as they are. A missing key falls back to the default language and is logged at startup.
2. Register the file in `../resources/configs.json`: `"cfg.strings.<name>": "configs/strings/<name>.json"`.
3. Add an entry to `languages.json`:
   `{ "id": "xx", "name": "<name in its own language>", "strings": "cfg.strings.<name>" }`
   - `id` is what the player profile stores; the Settings option cycles through the list in order.
   - `default` (top of the file) is the fallback language for missing keys and unknown saved ids.
   - Optional `"fonts": { "decorative": "font.id", "plain": "font.id" }` (ids from `../resources/fonts.json`) if the default fonts lack the letters.
4. Run `TanksClone.exe --test=unit`: it checks that every key exists and that the fonts can draw every character. The same problems are logged at game start.
