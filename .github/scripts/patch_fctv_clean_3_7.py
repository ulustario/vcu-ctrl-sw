from pathlib import Path

# Base sicura 3.5: niente riscrittura string_data, gate firma Splash via bytecode,
# neutralizzazione strutturale dei metodi GA/ADS.
exec(Path('.github/scripts/patch_fctv_clean_3_5.py').read_text(encoding='utf-8'), {})

p = Path('fctv-clean-updater-v3/app/src/main/java/com/fctvclean/updater/MainActivity.java')
s = p.read_text(encoding='utf-8')

def req(old, new, count=1):
    global s
    n = s.count(old)
    if n != count:
        raise SystemExit(f'expected {count}, found {n}: {old[:160]!r}')
    s = s.replace(old, new, count)

# Correzione 3.6 reale: code-unit DEX little-endian dell'if-nez v7 e' 0x0739.
s = s.replace('op != 0x3907 || rel != 0x000d', 'op != 0x0739 || rel != 0x000d')
# Non usiamo il vecchio controllo lessicografico Java sulle stringhe DEX.
s = s.replace('                    dex.validateStringIdsSorted();\n', '')
if '        void validateStringIdsSorted() throws Exception {' in s:
    a = s.index('        void validateStringIdsSorted() throws Exception {')
    b = s.index('        void patchCode(', a)
    s = s[:a] + s[b:]

# Identita' 3.7 DIAG.
s = s.replace('FCTV CLEAN Updater 3.5', 'FCTV CLEAN Updater 3.7 DIAG')
s = s.replace('FCTV-Clean-Updater/3.5 Android', 'FCTV-Clean-Updater/3.7 Android')
s = s.replace("L'Updater 3.5 non possiede", "L'Updater 3.7 non possiede")

# Modalita' manuale selezionata.
req('''    private File lastSigned;\n    private String lastOutputName;\n''',
'''    private File lastSigned;\n    private String lastOutputName;\n    private int pendingManualMode = 1;\n''')

# Quattro test indipendenti: A solo rifirma, B solo GA, C solo ADS, D equivalente FULL 3.6.
req('''        manualButton = new Button(this);\n        manualButton.setText("Seleziona APK FCTV33 manualmente");\n        body.addView(manualButton);\n\n        exportButton = new Button(this);\n''',
'''        manualButton = new Button(this);\n        manualButton.setText("DIAG A - SOLO RIFIRMA");\n        body.addView(manualButton);\n\n        Button gaButton = new Button(this);\n        gaButton.setText("DIAG B - RIFIRMA + SOLO GA OFF");\n        body.addView(gaButton);\n\n        Button adsButton = new Button(this);\n        adsButton.setText("DIAG C - RIFIRMA + SOLO ADS OFF");\n        body.addView(adsButton);\n\n        Button fullButton = new Button(this);\n        fullButton.setText("DIAG D - FULL COME 3.6");\n        body.addView(fullButton);\n\n        exportButton = new Button(this);\n''')

req('''        checkButton.setOnClickListener(v -> checkLatest(false));\n        manualButton.setOnClickListener(v -> chooseApk());\n        exportButton.setOnClickListener(v -> {\n''',
'''        checkButton.setVisibility(View.GONE);\n        manualButton.setOnClickListener(v -> { pendingManualMode = 1; chooseApk(); });\n        gaButton.setOnClickListener(v -> { pendingManualMode = 2; chooseApk(); });\n        adsButton.setOnClickListener(v -> { pendingManualMode = 3; chooseApk(); });\n        fullButton.setOnClickListener(v -> { pendingManualMode = 4; chooseApk(); });\n        exportButton.setOnClickListener(v -> {\n''')

# Niente preparazione automatica FULL all'apertura: deve essere un test controllato.
req('''        /* Controllo automatico ad ogni apertura. */\n        checkLatest(true);\n''',
'''        /* 3.7 DIAG: nessuna modifica automatica. Scegli esplicitamente A/B/C/D. */\n        status.setText("Scegli un test. Inizia da DIAG A - SOLO RIFIRMA.");\n''')

# Percorso online (nascosto) resta FULL, solo per coerenza compilativa.
req('Sanitizer.sanitize(original, unsigned);',
    'Sanitizer.sanitize(original, unsigned, true, true);', 1)

# Percorso manuale seleziona GA/ADS in modo indipendente.
req('''                    File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");\n                    Sanitizer.sanitize(original, unsigned);\n                    File signed = new File(getCacheDir(), "fctv_clean_signed.apk");\n''',
'''                    File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");\n                    boolean patchGa = pendingManualMode == 2 || pendingManualMode == 4;\n                    boolean patchAds = pendingManualMode == 3 || pendingManualMode == 4;\n                    Sanitizer.sanitize(original, unsigned, patchGa, patchAds);\n                    File signed = new File(getCacheDir(), "fctv_clean_signed.apk");\n''')

req('''                    lastSigned = signed;\n                    lastOutputName = "FCTV33_" + sanitizeFilePart(manual.name) + "_CLEAN.apk";\n''',
'''                    lastSigned = signed;\n                    String suffix;\n                    if (pendingManualMode == 1) suffix = "DIAG_A_RESIGN_ONLY";\n                    else if (pendingManualMode == 2) suffix = "DIAG_B_GA_ONLY";\n                    else if (pendingManualMode == 3) suffix = "DIAG_C_ADS_ONLY";\n                    else suffix = "DIAG_D_FULL_36";\n                    lastOutputName = "FCTV33_" + sanitizeFilePart(manual.name) + "_" + suffix + ".apk";\n''')

# Sanitizer parametrico.
req('static void sanitize(File input, File output) throws Exception {',
    'static void sanitize(File input, File output, boolean patchGa, boolean patchAds) throws Exception {')

# GA condizionale.
req('''                    int ga = dex.findMethod(GA_CLASS, GA_METHOD, "V");\n                    if (ga == -2) throw new Exception("GA.postEvent ambiguo");\n                    if (ga >= 0) {\n                        dex.neutralizeVoidMethod(ga); /* return-void + NOP fino a fine metodo */\n                        gaHits++;\n                    }\n\n                    for (String[] m : BOOL_METHODS) {\n                        int off = dex.findMethod(m[0], m[1], "Z");\n                        if (off == -2) throw new Exception("Metodo ambiguo: " + m[0] + "->" + m[1]);\n                        if (off >= 0) {\n                            /* const/4 v0, #0 ; return v0 */\n                            dex.neutralizeBooleanFalseMethod(off);\n                            String key = m[0] + "->" + m[1];\n                            methodHits.put(key, methodHits.get(key) + 1);\n                        }\n                    }\n''',
'''                    if (patchGa) {\n                        int ga = dex.findMethod(GA_CLASS, GA_METHOD, "V");\n                        if (ga == -2) throw new Exception("GA.postEvent ambiguo");\n                        if (ga >= 0) {\n                            dex.neutralizeVoidMethod(ga);\n                            gaHits++;\n                        }\n                    }\n\n                    if (patchAds) {\n                        for (String[] m : BOOL_METHODS) {\n                            int off = dex.findMethod(m[0], m[1], "Z");\n                            if (off == -2) throw new Exception("Metodo ambiguo: " + m[0] + "->" + m[1]);\n                            if (off >= 0) {\n                                dex.neutralizeBooleanFalseMethod(off);\n                                String key = m[0] + "->" + m[1];\n                                methodHits.put(key, methodHits.get(key) + 1);\n                            }\n                        }\n                    }\n''')

# Validazioni fail-closed solo per le patch richieste.
req('''            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");\n            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);\n            if (splashSignatureGateHits != 1) {\n''',
'''            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");\n            if (patchGa && gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);\n            if (splashSignatureGateHits != 1) {\n''')

req('''            for (String[] m : BOOL_METHODS) {\n                String key = m[0] + "->" + m[1];\n                int n = methodHits.get(key);\n                if (n != 1) throw new Exception("Layout non supportato: " + key + " hits=" + n);\n            }\n''',
'''            if (patchAds) {\n                for (String[] m : BOOL_METHODS) {\n                    String key = m[0] + "->" + m[1];\n                    int n = methodHits.get(key);\n                    if (n != 1) throw new Exception("Layout non supportato: " + key + " hits=" + n);\n                }\n            }\n''')

p.write_text(s, encoding='utf-8')

# Versione updater 3.7.
g = Path('fctv-clean-updater-v3/app/build.gradle')
t = g.read_text(encoding='utf-8')
if 'versionCode 350' not in t or "versionName '3.5'" not in t:
    raise SystemExit('3.5 version block not found')
t = t.replace('versionCode 350', 'versionCode 370')
t = t.replace("versionName '3.5'", "versionName '3.7'")
g.write_text(t, encoding='utf-8')

m = Path('fctv-clean-updater-v3/app/src/main/AndroidManifest.xml')
ms = m.read_text(encoding='utf-8').replace('FCTV CLEAN Updater 3.5', 'FCTV CLEAN Updater 3.7 DIAG')
m.write_text(ms, encoding='utf-8')
