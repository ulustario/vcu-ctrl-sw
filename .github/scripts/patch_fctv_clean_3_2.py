from pathlib import Path

java = Path('fctv-clean-updater-v3/app/src/main/java/com/fctvclean/updater/MainActivity.java')
s = java.read_text(encoding='utf-8')

def req(old, new, count=1):
    global s
    n = s.count(old)
    if n != count:
        raise SystemExit(f'expected {count} occurrence(s), found {n}: {old[:100]!r}')
    s = s.replace(old, new, count)

# 3.1: verifica firma APK non installato tramite ApkVerifier helper gia' nel progetto.
req('String signer = packageInfoSignerSha256(pi);',
    'String signer = ApkCert.signerSha256(apk, EXPECTED_FCTV_CERT_SHA256);')
marker = 'signApk(unsigned, signed);'
if s.count(marker) != 2:
    raise SystemExit(f'expected two signApk calls, found {s.count(marker)}')
s = s.replace(marker, marker + '\n        ApkCert.assertVerified(signed);')

# 3.2 UI / User-Agent.
req('FCTV CLEAN Updater 3.0', 'FCTV CLEAN Updater 3.2')
s = s.replace('FCTV-Clean-Updater/3.0 Android', 'FCTV-Clean-Updater/3.2 Android')
s = s.replace("L'Updater 3.0 non possiede", "L'Updater 3.2 non possiede")

# SHA-1 originale usato dal controllo firma interno FCTV33.
req('    private static final String EXPECTED_FCTV_CERT_SHA256 =\n            "9DC7D4202E31B81FCEF60937D02CFEAC67E260559C299B47D82CC0C521F9117E";\n',
'''    private static final String EXPECTED_FCTV_CERT_SHA256 =
            "9DC7D4202E31B81FCEF60937D02CFEAC67E260559C299B47D82CC0C521F9117E";

    /*
     * FCTV33 controlla la propria firma in SplashActivity tramite SignatureUtils.
     * Questo e' lo SHA-1 del certificato originale FCTV33 3.0.320 verificato.
     * Il sanitizer lo sostituisce con lo SHA-1 della chiave CLEAN locale.
     */
    private static final String ORIGINAL_FCTV_CERT_SHA1 =
            "0AD61B698455C4A4A56AF6875A398BD22AF5E92F";
''')

# Online: passa la firma CLEAN locale al sanitizer.
req('''        File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");
        Sanitizer.sanitize(original, unsigned);
''',
'''        File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");
        String localSignerSha1 = localSigningCertificateSha1();
        Sanitizer.sanitize(original, unsigned, localSignerSha1);
''', 1)

# Manuale: stessa correzione.
req('''                    File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");
                    Sanitizer.sanitize(original, unsigned);
''',
'''                    File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");
                    String localSignerSha1 = localSigningCertificateSha1();
                    Sanitizer.sanitize(original, unsigned, localSignerSha1);
''', 1)

# Helper SHA-1 certificato locale AndroidKeyStore.
req('''    private String localSigningCertificateSha256() throws Exception {
        KeyStore.PrivateKeyEntry e = getSigningEntry();
        return sha256Hex(e.getCertificate().getEncoded());
    }
''',
'''    private String localSigningCertificateSha256() throws Exception {
        KeyStore.PrivateKeyEntry e = getSigningEntry();
        return sha256Hex(e.getCertificate().getEncoded());
    }

    private String localSigningCertificateSha1() throws Exception {
        KeyStore.PrivateKeyEntry e = getSigningEntry();
        return sha1Hex(e.getCertificate().getEncoded());
    }
''')

req('''    private static String sha256Hex(byte[] data) throws Exception {
        byte[] d = MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder s = new StringBuilder(d.length * 2);
        for (byte b : d) s.append(String.format(Locale.ROOT, "%02X", b & 0xff));
        return s.toString();
    }
''',
'''    private static String sha256Hex(byte[] data) throws Exception {
        byte[] d = MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder s = new StringBuilder(d.length * 2);
        for (byte b : d) s.append(String.format(Locale.ROOT, "%02X", b & 0xff));
        return s.toString();
    }

    private static String sha1Hex(byte[] data) throws Exception {
        byte[] d = MessageDigest.getInstance("SHA-1").digest(data);
        StringBuilder s = new StringBuilder(d.length * 2);
        for (byte b : d) s.append(String.format(Locale.ROOT, "%02X", b & 0xff));
        return s.toString();
    }
''')

# Sanitizer: aggiorna l'hash anti-manomissione alla firma CLEAN locale, fail-closed.
req('        static void sanitize(File input, File output) throws Exception {\n',
    '        static void sanitize(File input, File output, String localSignerSha1) throws Exception {\n')

req('''            int gaHits = 0;
            boolean analyticsMarker = false;
            boolean crashMarker = false;
''',
'''            int gaHits = 0;
            int signatureHashHits = 0;
            boolean analyticsMarker = false;
            boolean crashMarker = false;

            if (localSignerSha1 == null || !localSignerSha1.matches("[0-9A-Fa-f]{40}")) {
                throw new Exception("SHA-1 certificato CLEAN locale non valido");
            }
            localSignerSha1 = localSignerSha1.toUpperCase(Locale.ROOT);
''')

req('''                    for (String h : HOSTS) {
                        int n = replaceAscii(dex.data, h, invalid(h));
                        hostHits.put(h, hostHits.get(h) + n);
                    }

                    dex.fix();
''',
'''                    for (String h : HOSTS) {
                        int n = replaceAscii(dex.data, h, invalid(h));
                        hostHits.put(h, hostHits.get(h) + n);
                    }

                    /*
                     * Preserva il controllo anti-manomissione FCTV33: non lo bypassa.
                     * Sostituisce lo SHA-1 del certificato originale con quello della chiave
                     * AndroidKeyStore che firmera' questo APK CLEAN (40 caratteri -> 40).
                     */
                    signatureHashHits += replaceAscii(dex.data, ORIGINAL_FCTV_CERT_SHA1, localSignerSha1);

                    dex.fix();
''')

req('''            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");
            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);
''',
'''            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");
            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);
            if (signatureHashHits != 1) {
                throw new Exception("Controllo firma interno FCTV33 non localizzato in modo univoco: hits=" + signatureHashHits);
            }
''')

java.write_text(s, encoding='utf-8')

# Versione package updater.
gradle = Path('fctv-clean-updater-v3/app/build.gradle')
g = gradle.read_text(encoding='utf-8')
if "versionCode 300" not in g or "versionName '3.0'" not in g:
    raise SystemExit('version block 3.0 not found')
g = g.replace('versionCode 300', 'versionCode 320')
g = g.replace("versionName '3.0'", "versionName '3.2'")
gradle.write_text(g, encoding='utf-8')

# Etichetta manifest.
manifest = Path('fctv-clean-updater-v3/app/src/main/AndroidManifest.xml')
m = manifest.read_text(encoding='utf-8').replace('FCTV CLEAN Updater 3.0', 'FCTV CLEAN Updater 3.2')
manifest.write_text(m, encoding='utf-8')
