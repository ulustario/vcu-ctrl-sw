from pathlib import Path

# Partiamo dalla stessa trasformazione 3.2 (ApkVerifier + firma locale nel controllo FCTV).
exec(Path('.github/scripts/patch_fctv_clean_3_2.py').read_text(encoding='utf-8'), {})

p = Path('fctv-clean-updater-v3/app/src/main/java/com/fctvclean/updater/MainActivity.java')
s = p.read_text(encoding='utf-8')

# Versione UI/User-Agent 3.3.
s = s.replace('FCTV CLEAN Updater 3.2', 'FCTV CLEAN Updater 3.3')
s = s.replace('FCTV-Clean-Updater/3.2 Android', 'FCTV-Clean-Updater/3.3 Android')
s = s.replace("L'Updater 3.2 non possiede", "L'Updater 3.3 non possiede")

# BUG 3.0-3.2: GA.postEvent iniziava con new-instance (2 word), ma veniva sovrascritta
# con una sola word return-void, lasciando il DEX disallineato. Ora neutralizziamo
# l'intero corpo del metodo mantenendo insns_size e code item invariati.
old = 'dex.patchCode(ga, new int[]{0x000e}); /* return-void */'
new = 'dex.neutralizeVoidMethod(ga); /* return-void + NOP fino a fine metodo */'
if s.count(old) != 1:
    raise SystemExit(f'GA patch point count={s.count(old)}')
s = s.replace(old, new)

# Per coerenza strutturale neutralizziamo integralmente anche i getter booleani.
old = 'dex.patchCode(off, new int[]{0x0012, 0x000f});'
new = 'dex.neutralizeBooleanFalseMethod(off);'
if s.count(old) != 1:
    raise SystemExit(f'boolean patch point count={s.count(old)}')
s = s.replace(old, new)

needle = '''        void patchCode(int codeOff, int[] units) throws Exception {
            if (codeOff <= 0) throw new Exception("code_off non valido");
            int insns = codeOff + 16;
            for (int i = 0; i < units.length; i++) put16(insns + 2 * i, units[i]);
        }
'''
replacement = '''        void neutralizeVoidMethod(int codeOff) throws Exception {
            if (codeOff <= 0) throw new Exception("code_off non valido");
            int triesSize = u16(codeOff + 6);
            int insnsSize = u32(codeOff + 12);
            if (triesSize != 0) throw new Exception("Metodo void con try/catch non supportato dal neutralizer");
            if (insnsSize < 1) throw new Exception("Metodo void senza istruzioni");
            int insns = codeOff + 16;
            put16(insns, 0x000e); /* return-void */
            for (int i = 1; i < insnsSize; i++) put16(insns + 2 * i, 0x0000); /* nop */
        }

        void neutralizeBooleanFalseMethod(int codeOff) throws Exception {
            if (codeOff <= 0) throw new Exception("code_off non valido");
            int triesSize = u16(codeOff + 6);
            int insnsSize = u32(codeOff + 12);
            if (triesSize != 0) throw new Exception("Metodo boolean con try/catch non supportato dal neutralizer");
            if (insnsSize < 2) throw new Exception("Metodo boolean troppo corto");
            int insns = codeOff + 16;
            put16(insns, 0x0012);     /* const/4 v0, #0 */
            put16(insns + 2, 0x000f); /* return v0 */
            for (int i = 2; i < insnsSize; i++) put16(insns + 2 * i, 0x0000); /* nop */
        }

        void patchCode(int codeOff, int[] units) throws Exception {
            if (codeOff <= 0) throw new Exception("code_off non valido");
            int insns = codeOff + 16;
            for (int i = 0; i < units.length; i++) put16(insns + 2 * i, units[i]);
        }
'''
if s.count(needle) != 1:
    raise SystemExit('Dex.patchCode insertion point not unique')
s = s.replace(needle, replacement)

p.write_text(s, encoding='utf-8')

g = Path('fctv-clean-updater-v3/app/build.gradle')
t = g.read_text(encoding='utf-8')
if 'versionCode 320' not in t or "versionName '3.2'" not in t:
    raise SystemExit('3.2 version block not found after base patch')
t = t.replace('versionCode 320', 'versionCode 330')
t = t.replace("versionName '3.2'", "versionName '3.3'")
g.write_text(t, encoding='utf-8')

m = Path('fctv-clean-updater-v3/app/src/main/AndroidManifest.xml')
ms = m.read_text(encoding='utf-8').replace('FCTV CLEAN Updater 3.2', 'FCTV CLEAN Updater 3.3')
m.write_text(ms, encoding='utf-8')
