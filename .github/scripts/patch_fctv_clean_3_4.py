from pathlib import Path

# Parte dalla 3.3: ApkVerifier + firma locale + neutralizzazione DEX strutturale.
exec(Path('.github/scripts/patch_fctv_clean_3_3.py').read_text(encoding='utf-8'), {})

p = Path('fctv-clean-updater-v3/app/src/main/java/com/fctvclean/updater/MainActivity.java')
s = p.read_text(encoding='utf-8')

def req(old, new, count=1):
    global s
    n = s.count(old)
    if n != count:
        raise SystemExit(f'expected {count}, found {n}: {old[:120]!r}')
    s = s.replace(old, new, count)

# Versione UI.
s = s.replace('FCTV CLEAN Updater 3.3', 'FCTV CLEAN Updater 3.4 DIAG')
s = s.replace('FCTV-Clean-Updater/3.3 Android', 'FCTV-Clean-Updater/3.4 Android')
s = s.replace("L'Updater 3.3 non possiede", "L'Updater 3.4 non possiede")

# Stato modalità manuale.
req('''    private File lastSigned;\n    private String lastOutputName;\n''',
'''    private File lastSigned;\n    private String lastOutputName;\n    private int pendingManualMode = 1;\n''')

# Trasforma il pulsante manuale in DIAG A e aggiunge DIAG B.
req('''        manualButton = new Button(this);\n        manualButton.setText("Seleziona APK FCTV33 manualmente");\n        body.addView(manualButton);\n\n        exportButton = new Button(this);\n''',
'''        manualButton = new Button(this);\n        manualButton.setText("DIAG A - SOLO RIFIRMA + CONTROLLO FIRMA");\n        body.addView(manualButton);\n\n        Button codeOnlyButton = new Button(this);\n        codeOnlyButton.setText("DIAG B - GA/ADS OFF, HOST INVARIATI");\n        body.addView(codeOnlyButton);\n\n        exportButton = new Button(this);\n''')

req('''        checkButton.setOnClickListener(v -> checkLatest(false));\n        manualButton.setOnClickListener(v -> chooseApk());\n        exportButton.setOnClickListener(v -> {\n''',
'''        checkButton.setOnClickListener(v -> checkLatest(false));\n        manualButton.setOnClickListener(v -> { pendingManualMode = 1; chooseApk(); });\n        codeOnlyButton.setOnClickListener(v -> { pendingManualMode = 2; chooseApk(); });\n        exportButton.setOnClickListener(v -> {\n''')

# Online resta FULL: codice + host.
req('Sanitizer.sanitize(original, unsigned, localSignerSha1);',
    'Sanitizer.sanitize(original, unsigned, localSignerSha1, true, true);', 1)

# Manuale: A = nessun codice/host, B = codice sì / host no.
req('''                    String localSignerSha1 = localSigningCertificateSha1();\n                    Sanitizer.sanitize(original, unsigned, localSignerSha1);\n                    File signed = new File(getCacheDir(), "fctv_clean_signed.apk");\n''',
'''                    String localSignerSha1 = localSigningCertificateSha1();\n                    boolean patchCode = pendingManualMode == 2;\n                    boolean patchHosts = false;\n                    Sanitizer.sanitize(original, unsigned, localSignerSha1, patchCode, patchHosts);\n                    File signed = new File(getCacheDir(), "fctv_clean_signed.apk");\n''')

req('''                    lastSigned = signed;\n                    lastOutputName = "FCTV33_" + sanitizeFilePart(manual.name) + "_CLEAN.apk";\n''',
'''                    lastSigned = signed;\n                    if (pendingManualMode == 1) {\n                        lastOutputName = "FCTV33_" + sanitizeFilePart(manual.name) + "_DIAG_A_RESIGN_ONLY.apk";\n                    } else {\n                        lastOutputName = "FCTV33_" + sanitizeFilePart(manual.name) + "_DIAG_B_CODE_ONLY.apk";\n                    }\n''')

# Parametri del sanitizer.
req('static void sanitize(File input, File output, String localSignerSha1) throws Exception {',
    'static void sanitize(File input, File output, String localSignerSha1, boolean patchTelemetryCode, boolean patchHosts) throws Exception {')

# GA e getter ads vengono patchati solo in modalità code.
req('''                    int ga = dex.findMethod(GA_CLASS, GA_METHOD, "V");\n                    if (ga == -2) throw new Exception("GA.postEvent ambiguo");\n                    if (ga >= 0) {\n                        dex.neutralizeVoidMethod(ga); /* return-void + NOP fino a fine metodo */\n                        gaHits++;\n                    }\n\n                    for (String[] m : BOOL_METHODS) {\n                        int off = dex.findMethod(m[0], m[1], "Z");\n                        if (off == -2) throw new Exception("Metodo ambiguo: " + m[0] + "->" + m[1]);\n                        if (off >= 0) {\n                            /* const/4 v0, #0 ; return v0 */\n                            dex.neutralizeBooleanFalseMethod(off);\n                            String key = m[0] + "->" + m[1];\n                            methodHits.put(key, methodHits.get(key) + 1);\n                        }\n                    }\n\n                    for (String h : HOSTS) {\n                        int n = replaceAscii(dex.data, h, invalid(h));\n                        hostHits.put(h, hostHits.get(h) + n);\n                    }\n''',
'''                    if (patchTelemetryCode) {\n                        int ga = dex.findMethod(GA_CLASS, GA_METHOD, "V");\n                        if (ga == -2) throw new Exception("GA.postEvent ambiguo");\n                        if (ga >= 0) {\n                            dex.neutralizeVoidMethod(ga);\n                            gaHits++;\n                        }\n\n                        for (String[] m : BOOL_METHODS) {\n                            int off = dex.findMethod(m[0], m[1], "Z");\n                            if (off == -2) throw new Exception("Metodo ambiguo: " + m[0] + "->" + m[1]);\n                            if (off >= 0) {\n                                dex.neutralizeBooleanFalseMethod(off);\n                                String key = m[0] + "->" + m[1];\n                                methodHits.put(key, methodHits.get(key) + 1);\n                            }\n                        }\n                    }\n\n                    if (patchHosts) {\n                        for (String h : HOSTS) {\n                            int n = replaceAscii(dex.data, h, invalid(h));\n                            hostHits.put(h, hostHits.get(h) + n);\n                        }\n                    }\n''')

# Validazioni condizionali.
req('''            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");\n            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);\n            if (signatureHashHits != 1) {\n''',
'''            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");\n            if (patchTelemetryCode && gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);\n            if (signatureHashHits != 1) {\n''')

req('''            for (String[] m : BOOL_METHODS) {\n                String key = m[0] + "->" + m[1];\n                int n = methodHits.get(key);\n                if (n != 1) throw new Exception("Layout non supportato: " + key + " hits=" + n);\n            }\n\n            if (analyticsMarker && hostHits.get("app-measurement.com") == 0) {\n                throw new Exception("Firebase Analytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");\n            }\n            if (crashMarker && hostHits.get("firebase-settings.crashlytics.com") == 0) {\n                throw new Exception("Crashlytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");\n            }\n''',
'''            if (patchTelemetryCode) {\n                for (String[] m : BOOL_METHODS) {\n                    String key = m[0] + "->" + m[1];\n                    int n = methodHits.get(key);\n                    if (n != 1) throw new Exception("Layout non supportato: " + key + " hits=" + n);\n                }\n            }\n\n            if (patchHosts) {\n                if (analyticsMarker && hostHits.get("app-measurement.com") == 0) {\n                    throw new Exception("Firebase Analytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");\n                }\n                if (crashMarker && hostHits.get("firebase-settings.crashlytics.com") == 0) {\n                    throw new Exception("Crashlytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");\n                }\n            }\n''')

p.write_text(s, encoding='utf-8')

g = Path('fctv-clean-updater-v3/app/build.gradle')
t = g.read_text(encoding='utf-8')
t = t.replace('versionCode 330', 'versionCode 340')
t = t.replace("versionName '3.3'", "versionName '3.4'")
g.write_text(t, encoding='utf-8')

m = Path('fctv-clean-updater-v3/app/src/main/AndroidManifest.xml')
ms = m.read_text(encoding='utf-8').replace('FCTV CLEAN Updater 3.3', 'FCTV CLEAN Updater 3.4 DIAG')
m.write_text(ms, encoding='utf-8')
