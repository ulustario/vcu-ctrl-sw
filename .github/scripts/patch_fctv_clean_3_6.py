from pathlib import Path

# Base 3.5: ApkVerifier + GA/ADS neutralizzati + gate Splash senza mutare stringhe.
exec(Path('.github/scripts/patch_fctv_clean_3_5.py').read_text(encoding='utf-8'), {})

p = Path('fctv-clean-updater-v3/app/src/main/java/com/fctvclean/updater/MainActivity.java')
s = p.read_text(encoding='utf-8')

# Versione UI/User-Agent.
s = s.replace('FCTV CLEAN Updater 3.5', 'FCTV CLEAN Updater 3.6')
s = s.replace('FCTV-Clean-Updater/3.5 Android', 'FCTV-Clean-Updater/3.6 Android')
s = s.replace("L'Updater 3.5 non possiede", "L'Updater 3.6 non possiede")

# 1) Rimuove il falso validatore string_ids della 3.5.
# Il DEX usa Modified UTF-8; il parser minimale dell'Updater usa UTF-8 Java solo per
# identificatori ASCII. Confrontare quelle String per validare l'ordinamento produce
# falsi positivi su stringhe MUTF-8. Inoltre la 3.6 non modifica alcuna string_data_item,
# quindi la string_ids originale resta invariata e non c'e' nulla da riordinare.
call = '                    dex.validateStringIdsSorted();\n'
if s.count(call) != 1:
    raise SystemExit(f'validateStringIdsSorted call count={s.count(call)}')
s = s.replace(call, '')

method = '''        void validateStringIdsSorted() throws Exception {\n            for (int i = 1; i < strings.length; i++) {\n                if (strings[i - 1].compareTo(strings[i]) > 0) {\n                    throw new Exception("DEX string_ids fuori ordine: '" + strings[i - 1] +\n                            "' > '" + strings[i] + "'");\n                }\n            }\n        }\n\n'''
if s.count(method) != 1:
    raise SystemExit(f'validateStringIdsSorted method count={s.count(method)}')
s = s.replace(method, '')

# 2) Corregge endianness code-unit verificata direttamente sul DEX originale 3.0.320.
# Formato DEX 21t: if-nez opcode 0x39, registro v7 nel byte alto => u16 LE = 0x0739.
old = '''            if (op != 0x3907 || rel != 0x000d) {\n'''
new = '''            if (op != 0x0739 || rel != 0x000d) {\n'''
if s.count(old) != 1:
    raise SystemExit(f'Splash endian check count={s.count(old)}')
s = s.replace(old, new)
s = s.replace('code unit 9 = if-nez v7 (+0x0d), unit 10 = 0x000d.',
              'code unit 9 = if-nez v7 (+0x0d), u16 LE 0x0739; unit 10 = 0x000d.')

p.write_text(s, encoding='utf-8')

# Versione package updater.
g = Path('fctv-clean-updater-v3/app/build.gradle')
t = g.read_text(encoding='utf-8')
if 'versionCode 350' not in t or "versionName '3.5'" not in t:
    raise SystemExit('3.5 version block not found')
t = t.replace('versionCode 350', 'versionCode 360')
t = t.replace("versionName '3.5'", "versionName '3.6'")
g.write_text(t, encoding='utf-8')

m = Path('fctv-clean-updater-v3/app/src/main/AndroidManifest.xml')
ms = m.read_text(encoding='utf-8').replace('FCTV CLEAN Updater 3.5', 'FCTV CLEAN Updater 3.6')
m.write_text(ms, encoding='utf-8')
