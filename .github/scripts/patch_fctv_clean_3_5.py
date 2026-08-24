from pathlib import Path

# Base 3.3: ApkVerifier + neutralizzazione strutturale GA/ADS.
exec(Path('.github/scripts/patch_fctv_clean_3_3.py').read_text(encoding='utf-8'), {})

p = Path('fctv-clean-updater-v3/app/src/main/java/com/fctvclean/updater/MainActivity.java')
s = p.read_text(encoding='utf-8')

def req(old, new, count=1):
    global s
    n = s.count(old)
    if n != count:
        raise SystemExit(f'expected {count}, found {n}: {old[:160]!r}')
    s = s.replace(old, new, count)

# Versione UI/User-Agent.
s = s.replace('FCTV CLEAN Updater 3.3', 'FCTV CLEAN Updater 3.5')
s = s.replace('FCTV-Clean-Updater/3.3 Android', 'FCTV-Clean-Updater/3.5 Android')
s = s.replace("L'Updater 3.3 non possiede", "L'Updater 3.5 non possiede")

# Non modifichiamo piu' stringhe dentro string_data_item DEX: cambiare il testo in-place
# puo' rendere string_ids non piu' ordinata e ART rifiuta l'intero DEX.
# Anche l'hash firma non viene piu' sostituito come stringa: patchiamo invece il gate
# di controllo in SplashActivity con un salto incondizionato di pari lunghezza.
req('''        String localSignerSha1 = localSigningCertificateSha1();\n        Sanitizer.sanitize(original, unsigned, localSignerSha1);\n''',
'''        Sanitizer.sanitize(original, unsigned);\n''', 1)
req('''                    String localSignerSha1 = localSigningCertificateSha1();\n                    Sanitizer.sanitize(original, unsigned, localSignerSha1);\n''',
'''                    Sanitizer.sanitize(original, unsigned);\n''', 1)

req('static void sanitize(File input, File output, String localSignerSha1) throws Exception {',
    'static void sanitize(File input, File output) throws Exception {')

req('''            int gaHits = 0;\n            int signatureHashHits = 0;\n            boolean analyticsMarker = false;\n            boolean crashMarker = false;\n\n            if (localSignerSha1 == null || !localSignerSha1.matches("[0-9A-Fa-f]{40}")) {\n                throw new Exception("SHA-1 certificato CLEAN locale non valido");\n            }\n            localSignerSha1 = localSignerSha1.toUpperCase(Locale.ROOT);\n''',
'''            int gaHits = 0;\n            int splashSignatureGateHits = 0;\n            boolean analyticsMarker = false;\n            boolean crashMarker = false;\n''')

# Elimina completamente la riscrittura degli host. Gli endpoint restano come stringhe originali;
# la telemetria applicativa viene fermata nei metodi GA/ADS senza toccare string_ids.
req('''                    for (String h : HOSTS) {\n                        int n = replaceAscii(dex.data, h, invalid(h));\n                        hostHits.put(h, hostHits.get(h) + n);\n                    }\n\n                    /*\n                     * Preserva il controllo anti-manomissione FCTV33: non lo bypassa.\n                     * Sostituisce lo SHA-1 del certificato originale con quello della chiave\n                     * AndroidKeyStore che firmera' questo APK CLEAN (40 caratteri -> 40).\n                     */\n                    signatureHashHits += replaceAscii(dex.data, ORIGINAL_FCTV_CERT_SHA1, localSignerSha1);\n\n                    dex.fix();\n''',
'''                    /*\n                     * FCTV33 3.0.320 esegue in SplashActivity.onCreate():\n                     *   if-nez v7, success\n                     * dove v7 e' il risultato di SignatureUtils.getSignature().\n                     * Dopo la rifirma la firma cambia necessariamente. Per evitare di mutare\n                     * stringhe DEX (che romperebbe l'ordinamento string_ids), convertiamo SOLO\n                     * quel gate in un goto/16 verso lo stesso ramo success. La patch e'\n                     * fail-closed e verifica i due code-unit originali esatti.\n                     */\n                    int splash = dex.findMethod(\n                            "Lcom/rblive/launcher/splash/SplashActivity;", "onCreate", "V");\n                    if (splash == -2) throw new Exception("SplashActivity.onCreate ambiguo");\n                    if (splash >= 0) {\n                        dex.patchSplashSignatureGate(splash);\n                        splashSignatureGateHits++;\n                    }\n\n                    dex.validateStringIdsSorted();\n                    dex.fix();\n''')

req('''            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);\n            if (signatureHashHits != 1) {\n                throw new Exception("Controllo firma interno FCTV33 non localizzato in modo univoco: hits=" + signatureHashHits);\n            }\n''',
'''            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);\n            if (splashSignatureGateHits != 1) {\n                throw new Exception("Gate firma SplashActivity non localizzato in modo univoco: hits=" + splashSignatureGateHits);\n            }\n''')

# Queste validazioni erano legate alle sostituzioni host, ora deliberatamente assenti.
req('''            if (analyticsMarker && hostHits.get("app-measurement.com") == 0) {\n                throw new Exception("Firebase Analytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");\n            }\n            if (crashMarker && hostHits.get("firebase-settings.crashlytics.com") == 0) {\n                throw new Exception("Crashlytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");\n            }\n''', '')

# Aggiunge patch gate Splash e controllo esplicito dell'ordinamento string_ids.
needle = '''        void patchCode(int codeOff, int[] units) throws Exception {\n            if (codeOff <= 0) throw new Exception("code_off non valido");\n            int insns = codeOff + 16;\n            for (int i = 0; i < units.length; i++) put16(insns + 2 * i, units[i]);\n        }\n'''
replacement = '''        void patchSplashSignatureGate(int codeOff) throws Exception {\n            if (codeOff <= 0) throw new Exception("code_off Splash non valido");\n            int triesSize = u16(codeOff + 6);\n            int insnsSize = u32(codeOff + 12);\n            if (triesSize != 0) throw new Exception("SplashActivity.onCreate con try/catch inatteso");\n            if (insnsSize <= 10) throw new Exception("SplashActivity.onCreate troppo corto");\n            int insns = codeOff + 16;\n\n            /* FCTV33 3.0.320: code unit 9 = if-nez v7 (+0x0d), unit 10 = 0x000d. */\n            int op = u16(insns + 2 * 9);\n            int rel = u16(insns + 2 * 10);\n            if (op != 0x3907 || rel != 0x000d) {\n                throw new Exception(String.format(Locale.ROOT,\n                        "Gate firma Splash inatteso: %04X %04X", op, rel));\n            }\n\n            /* goto/16 +0x000d: stessa larghezza (2 code unit) e stesso target :0016. */\n            put16(insns + 2 * 9, 0x0029);\n            put16(insns + 2 * 10, 0x000d);\n        }\n\n        void validateStringIdsSorted() throws Exception {\n            for (int i = 1; i < strings.length; i++) {\n                if (strings[i - 1].compareTo(strings[i]) > 0) {\n                    throw new Exception("DEX string_ids fuori ordine: '" + strings[i - 1] +\n                            "' > '" + strings[i] + "'");\n                }\n            }\n        }\n\n        void patchCode(int codeOff, int[] units) throws Exception {\n            if (codeOff <= 0) throw new Exception("code_off non valido");\n            int insns = codeOff + 16;\n            for (int i = 0; i < units.length; i++) put16(insns + 2 * i, units[i]);\n        }\n'''
if s.count(needle) != 1:
    raise SystemExit('patchCode insertion point not unique')
s = s.replace(needle, replacement)

p.write_text(s, encoding='utf-8')

# Versione package updater.
g = Path('fctv-clean-updater-v3/app/build.gradle')
t = g.read_text(encoding='utf-8')
if 'versionCode 330' not in t or "versionName '3.3'" not in t:
    raise SystemExit('3.3 version block not found')
t = t.replace('versionCode 330', 'versionCode 350')
t = t.replace("versionName '3.3'", "versionName '3.5'")
g.write_text(t, encoding='utf-8')

m = Path('fctv-clean-updater-v3/app/src/main/AndroidManifest.xml')
ms = m.read_text(encoding='utf-8').replace('FCTV CLEAN Updater 3.3', 'FCTV CLEAN Updater 3.5')
m.write_text(ms, encoding='utf-8')
