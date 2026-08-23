package com.fctvclean.updater;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.android.apksig.ApkSigner;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;
import java.util.zip.Adler32;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

import javax.security.auth.x500.X500Principal;

public class MainActivity extends Activity {
    private static final int PICK_APK = 1001;
    private static final int SAVE_APK = 1002;

    private static final String TARGET_PACKAGE = "com.fctv77.app";
    private static final String KEY_ALIAS = "fctv_clean_local_signer_v1";

    /*
     * Dati verificati direttamente nell'APK FCTV33 3.0.320 originale.
     * L'app originale usa questo stesso progetto Firebase/Firestore e questo documento.
     */
    private static final String FIREBASE_PROJECT = "rbapp-672ea";
    private static final String FIRESTORE_DOC = "foth_APP_KVS/params_production";
    private static final String FIREBASE_API_KEY = "AIzaSyC4pdqs2LpHOvA3S6IpbX9YnyGpve69amI";
    private static final String FIRESTORE_URL =
            "https://firestore.googleapis.com/v1/projects/" + FIREBASE_PROJECT +
            "/databases/(default)/documents/" + FIRESTORE_DOC + "?key=" + FIREBASE_API_KEY;

    /* Certificato SHA-256 dell'APK FCTV33 3.0.320 originale analizzato. */
    private static final String EXPECTED_FCTV_CERT_SHA256 =
            "9DC7D4202E31B81FCEF60937D02CFEAC67E260559C299B47D82CC0C521F9117E";

    private TextView status;
    private Button checkButton;
    private Button manualButton;
    private Button exportButton;
    private File lastSigned;
    private String lastOutputName;

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);

        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(28, 28, 28, 28);

        TextView title = new TextView(this);
        title.setText("FCTV CLEAN Updater 3.0");
        title.setTextSize(26);
        body.addView(title);

        TextView info = new TextView(this);
        info.setText("Controlla lo stesso backend Firestore usato da FCTV33, scarica l'APK ufficiale, verifica package e firma, rimuove telemetria/pubblicita', rifirma con AndroidKeyStore e salva il risultato in Download. Non installa APK e non richiede REQUEST_INSTALL_PACKAGES.");
        info.setTextSize(15);
        body.addView(info);

        status = new TextView(this);
        status.setText("Avvio controllo ultima versione...");
        status.setTextSize(16);
        status.setPadding(0, 24, 0, 24);
        body.addView(status);

        checkButton = new Button(this);
        checkButton.setText("Controlla e prepara ultima FCTV33");
        body.addView(checkButton);

        manualButton = new Button(this);
        manualButton.setText("Seleziona APK FCTV33 manualmente");
        body.addView(manualButton);

        exportButton = new Button(this);
        exportButton.setText("Esporta ultimo APK CLEAN");
        exportButton.setEnabled(false);
        body.addView(exportButton);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(body);
        setContentView(scroll);

        checkButton.setOnClickListener(v -> checkLatest(false));
        manualButton.setOnClickListener(v -> chooseApk());
        exportButton.setOnClickListener(v -> {
            if (lastSigned != null) exportSigned(lastSigned, lastOutputName, true);
        });

        /* Controllo automatico ad ogni apertura. */
        checkLatest(true);
    }

    private void setStatus(final String text) {
        runOnUiThread(() -> status.setText(text));
    }

    private void setBusy(final boolean busy) {
        runOnUiThread(() -> {
            checkButton.setEnabled(!busy);
            manualButton.setEnabled(!busy);
        });
    }

    private void checkLatest(boolean automatic) {
        setBusy(true);
        setStatus("Controllo Firestore FCTV33...");

        new Thread(() -> {
            try {
                RemoteVersion remote = loadRemoteVersion();
                if (remote.apkUrl == null || remote.apkUrl.trim().isEmpty()) {
                    throw new Exception("versionInfo non contiene apkUrl");
                }

                StringBuilder msg = new StringBuilder();
                msg.append("Ultima versione backend: ").append(remote.name);
                if (remote.code > 0) msg.append(" (code ").append(remote.code).append(")");
                msg.append("\nURL APK trovato e validato come URL HTTPS.");
                setStatus(msg.toString());

                InstalledState installed = getInstalledState();
                String localSigner = localSigningCertificateSha256();

                if (installed.present && remote.code > 0 && installed.versionCode >= remote.code &&
                        localSigner.equalsIgnoreCase(installed.signerSha256)) {
                    setStatus("FCTV CLEAN gia' installata e aggiornata: " + installed.versionName +
                            " (code " + installed.versionCode + ").\nNessun download necessario.");
                    return;
                }

                /*
                 * Se FCTV non e' installata, e' installata l'originale, oppure e' firmata con
                 * una vecchia chiave CLEAN, prepariamo comunque l'ultima versione.
                 */
                downloadCleanSignExport(remote);
            } catch (Exception e) {
                setStatus("Controllo automatico BLOCCATO (fail-closed):\n" + safeMessage(e) +
                        "\n\nPuoi usare 'Seleziona APK FCTV33 manualmente': anche in quel caso package e firma originale vengono verificati prima della pulizia.");
            } finally {
                setBusy(false);
            }
        }).start();
    }

    private RemoteVersion loadRemoteVersion() throws Exception {
        HttpURLConnection c = openGet(FIRESTORE_URL);
        int code = c.getResponseCode();
        if (code != 200) {
            String err = readText(c.getErrorStream());
            throw new Exception("Firestore HTTP " + code + (err.isEmpty() ? "" : ": " + limit(err, 180)));
        }

        String json = readText(c.getInputStream());
        JSONObject document = new JSONObject(json);
        JSONObject fields = document.optJSONObject("fields");
        if (fields == null) throw new Exception("Documento Firestore senza fields");

        JSONObject versionField = fields.optJSONObject("versionInfo");
        if (versionField == null) throw new Exception("Campo versionInfo assente");

        Object value = fireValue(versionField);
        value = unwrapJsonString(value);

        List<JSONObject> candidates = new ArrayList<>();
        collectVersionCandidates(value, candidates);
        if (candidates.isEmpty()) throw new Exception("Nessun VersionInfo con apkUrl trovato");

        RemoteVersion best = null;
        for (JSONObject o : candidates) {
            RemoteVersion v = RemoteVersion.from(o);
            if (v.apkUrl == null || !v.apkUrl.toLowerCase(Locale.ROOT).startsWith("https://")) continue;
            if (best == null || v.code > best.code) best = v;
        }
        if (best == null) throw new Exception("Nessun apkUrl HTTPS valido in versionInfo");
        return best;
    }

    private static Object fireValue(JSONObject v) throws Exception {
        if (v.has("stringValue")) return v.optString("stringValue", "");
        if (v.has("integerValue")) {
            String s = v.optString("integerValue", "0");
            try { return Long.parseLong(s); } catch (Exception e) { return 0L; }
        }
        if (v.has("doubleValue")) return v.optDouble("doubleValue", 0.0);
        if (v.has("booleanValue")) return v.optBoolean("booleanValue", false);
        if (v.has("nullValue")) return JSONObject.NULL;

        JSONObject map = v.optJSONObject("mapValue");
        if (map != null) {
            JSONObject src = map.optJSONObject("fields");
            JSONObject out = new JSONObject();
            if (src != null) {
                JSONArray names = src.names();
                if (names != null) {
                    for (int i = 0; i < names.length(); i++) {
                        String k = names.getString(i);
                        out.put(k, fireValue(src.getJSONObject(k)));
                    }
                }
            }
            return out;
        }

        JSONObject arr = v.optJSONObject("arrayValue");
        if (arr != null) {
            JSONArray in = arr.optJSONArray("values");
            JSONArray out = new JSONArray();
            if (in != null) {
                for (int i = 0; i < in.length(); i++) out.put(fireValue(in.getJSONObject(i)));
            }
            return out;
        }

        return JSONObject.NULL;
    }

    private static Object unwrapJsonString(Object value) {
        if (!(value instanceof String)) return value;
        String s = ((String) value).trim();
        if (!(s.startsWith("{") || s.startsWith("["))) return value;
        try { return new JSONTokener(s).nextValue(); }
        catch (Exception ignored) { return value; }
    }

    private static void collectVersionCandidates(Object value, List<JSONObject> out) {
        if (value == null || value == JSONObject.NULL) return;
        if (value instanceof String) {
            Object parsed = unwrapJsonString(value);
            if (parsed != value) collectVersionCandidates(parsed, out);
            return;
        }
        if (value instanceof JSONArray) {
            JSONArray a = (JSONArray) value;
            for (int i = 0; i < a.length(); i++) collectVersionCandidates(a.opt(i), out);
            return;
        }
        if (!(value instanceof JSONObject)) return;

        JSONObject o = (JSONObject) value;
        if (!o.optString("apkUrl", "").isEmpty()) out.add(o);

        JSONArray names = o.names();
        if (names != null) {
            for (int i = 0; i < names.length(); i++) {
                String k = names.optString(i);
                Object child = o.opt(k);
                if (child instanceof JSONObject || child instanceof JSONArray || child instanceof String) {
                    collectVersionCandidates(child, out);
                }
            }
        }
    }

    private void downloadCleanSignExport(RemoteVersion remote) throws Exception {
        setStatus("Download FCTV33 " + remote.name + "...");
        File original = new File(getCacheDir(), "fctv_official_latest.apk");
        download(remote.apkUrl, original);

        PackageInfo pi = verifyOfficialFctv(original, remote.code);
        long actualCode = packageVersionCode(pi);
        String actualName = pi.versionName == null ? remote.name : pi.versionName;

        setStatus("APK ufficiale verificato: " + actualName + " (code " + actualCode + ")\nPulizia telemetria/pubblicita'...");

        File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");
        Sanitizer.sanitize(original, unsigned);

        setStatus("Pulizia completata. Firma con chiave AndroidKeyStore locale...");
        File signed = new File(getCacheDir(), "fctv_clean_signed.apk");
        signApk(unsigned, signed);

        /* Verifica minima post-firma prima dell'esportazione. */
        PackageInfo cleanInfo = getPackageManager().getPackageArchiveInfo(signed.getAbsolutePath(), PackageManager.GET_SIGNATURES);
        if (cleanInfo == null || !TARGET_PACKAGE.equals(cleanInfo.packageName)) {
            throw new Exception("APK CLEAN firmato non riconoscibile come " + TARGET_PACKAGE);
        }

        lastSigned = signed;
        lastOutputName = "FCTV33_" + sanitizeFilePart(actualName) + "_CLEAN.apk";
        runOnUiThread(() -> exportButton.setEnabled(true));
        exportSigned(signed, lastOutputName, false);
    }

    private void chooseApk() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("application/vnd.android.package-archive");
        startActivityForResult(i, PICK_APK);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) return;

        if (requestCode == PICK_APK) {
            Uri uri = data.getData();
            setBusy(true);
            new Thread(() -> {
                try {
                    File original = new File(getCacheDir(), "fctv_manual.apk");
                    copyUri(uri, original);
                    PackageInfo pi = verifyOfficialFctv(original, 0);
                    RemoteVersion manual = new RemoteVersion();
                    manual.code = (int) Math.min(Integer.MAX_VALUE, packageVersionCode(pi));
                    manual.name = pi.versionName == null ? String.valueOf(manual.code) : pi.versionName;
                    manual.apkUrl = null;

                    setStatus("APK manuale ufficiale verificato. Pulizia...");
                    File unsigned = new File(getCacheDir(), "fctv_clean_unsigned.apk");
                    Sanitizer.sanitize(original, unsigned);
                    File signed = new File(getCacheDir(), "fctv_clean_signed.apk");
                    signApk(unsigned, signed);
                    lastSigned = signed;
                    lastOutputName = "FCTV33_" + sanitizeFilePart(manual.name) + "_CLEAN.apk";
                    runOnUiThread(() -> exportButton.setEnabled(true));
                    exportSigned(signed, lastOutputName, false);
                } catch (Exception e) {
                    setStatus("APK manuale BLOCCATO (fail-closed):\n" + safeMessage(e));
                } finally {
                    setBusy(false);
                }
            }).start();
        } else if (requestCode == SAVE_APK && lastSigned != null) {
            try {
                copyFileToUri(lastSigned, data.getData());
                setStatus("APK CLEAN salvato.\nApri il file dal file manager/Download per avviare l'installer Android.\nL'Updater non possiede permessi di installazione APK.");
            } catch (Exception e) {
                setStatus("Esportazione fallita: " + safeMessage(e));
            }
        }
    }

    private PackageInfo verifyOfficialFctv(File apk, int expectedVersionCode) throws Exception {
        int flags = Build.VERSION.SDK_INT >= 28 ? PackageManager.GET_SIGNING_CERTIFICATES : PackageManager.GET_SIGNATURES;
        PackageInfo pi = getPackageManager().getPackageArchiveInfo(apk.getAbsolutePath(), flags);
        if (pi == null) throw new Exception("File non riconosciuto come APK Android");
        if (!TARGET_PACKAGE.equals(pi.packageName)) {
            throw new Exception("Package rifiutato: " + pi.packageName + " (atteso " + TARGET_PACKAGE + ")");
        }

        long actualCode = packageVersionCode(pi);
        if (expectedVersionCode > 0 && actualCode != expectedVersionCode) {
            throw new Exception("Version code APK " + actualCode + " diverso dal backend " + expectedVersionCode);
        }

        String signer = packageInfoSignerSha256(pi);
        if (!EXPECTED_FCTV_CERT_SHA256.equalsIgnoreCase(signer)) {
            throw new Exception("Firma FCTV33 originale non riconosciuta. SHA-256=" + signer);
        }
        return pi;
    }

    private InstalledState getInstalledState() {
        InstalledState out = new InstalledState();
        try {
            int flags = Build.VERSION.SDK_INT >= 28 ? PackageManager.GET_SIGNING_CERTIFICATES : PackageManager.GET_SIGNATURES;
            PackageInfo pi = getPackageManager().getPackageInfo(TARGET_PACKAGE, flags);
            out.present = true;
            out.versionCode = packageVersionCode(pi);
            out.versionName = pi.versionName == null ? String.valueOf(out.versionCode) : pi.versionName;
            out.signerSha256 = packageInfoSignerSha256(pi);
        } catch (Exception ignored) {
            out.present = false;
        }
        return out;
    }

    private long packageVersionCode(PackageInfo pi) {
        if (Build.VERSION.SDK_INT >= 28) return pi.getLongVersionCode();
        return pi.versionCode;
    }

    private String packageInfoSignerSha256(PackageInfo pi) throws Exception {
        Signature[] signatures;
        if (Build.VERSION.SDK_INT >= 28 && pi.signingInfo != null) {
            signatures = pi.signingInfo.hasMultipleSigners()
                    ? pi.signingInfo.getApkContentsSigners()
                    : pi.signingInfo.getSigningCertificateHistory();
        } else {
            signatures = pi.signatures;
        }
        if (signatures == null || signatures.length == 0) throw new Exception("APK senza certificato leggibile");

        for (Signature s : signatures) {
            String fp = sha256Hex(s.toByteArray());
            if (EXPECTED_FCTV_CERT_SHA256.equalsIgnoreCase(fp)) return fp;
        }
        return sha256Hex(signatures[0].toByteArray());
    }

    private String localSigningCertificateSha256() throws Exception {
        KeyStore.PrivateKeyEntry e = getSigningEntry();
        return sha256Hex(e.getCertificate().getEncoded());
    }

    private KeyStore.PrivateKeyEntry getSigningEntry() throws Exception {
        KeyStore ks = KeyStore.getInstance("AndroidKeyStore");
        ks.load(null);
        if (!ks.containsAlias(KEY_ALIAS)) {
            Calendar from = Calendar.getInstance();
            Calendar to = Calendar.getInstance();
            to.add(Calendar.YEAR, 25);

            KeyPairGenerator kpg = KeyPairGenerator.getInstance(KeyProperties.KEY_ALGORITHM_RSA, "AndroidKeyStore");
            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(
                    KEY_ALIAS, KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                    .setKeySize(2048)
                    .setDigests(KeyProperties.DIGEST_SHA256, KeyProperties.DIGEST_SHA512)
                    .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
                    .setCertificateSubject(new X500Principal("CN=FCTV CLEAN LOCAL"))
                    .setCertificateSerialNumber(BigInteger.ONE)
                    .setCertificateNotBefore(from.getTime())
                    .setCertificateNotAfter(to.getTime())
                    .build();
            kpg.initialize(spec);
            kpg.generateKeyPair();
        }
        return (KeyStore.PrivateKeyEntry) ks.getEntry(KEY_ALIAS, null);
    }

    private void signApk(File in, File out) throws Exception {
        if (out.exists() && !out.delete()) throw new Exception("Impossibile sostituire APK CLEAN precedente");
        KeyStore.PrivateKeyEntry e = getSigningEntry();
        ApkSigner.SignerConfig cfg = new ApkSigner.SignerConfig.Builder(
                "FCTV CLEAN",
                e.getPrivateKey(),
                Collections.singletonList((X509Certificate) e.getCertificate()))
                .build();

        new ApkSigner.Builder(Collections.singletonList(cfg))
                .setInputApk(in)
                .setOutputApk(out)
                .setV1SigningEnabled(true)
                .setV2SigningEnabled(true)
                .setV3SigningEnabled(false)
                .build()
                .sign();
    }

    private void exportSigned(File apk, String fileName, boolean explicit) {
        if (apk == null || !apk.exists()) return;
        if (fileName == null || fileName.isEmpty()) fileName = "FCTV33_CLEAN.apk";

        if (Build.VERSION.SDK_INT >= 29) {
            final String name = fileName;
            new Thread(() -> {
                try {
                    ContentResolver r = getContentResolver();
                    ContentValues v = new ContentValues();
                    v.put(MediaStore.Downloads.DISPLAY_NAME, name);
                    v.put(MediaStore.Downloads.MIME_TYPE, "application/vnd.android.package-archive");
                    v.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
                    v.put(MediaStore.Downloads.IS_PENDING, 1);
                    Uri uri = r.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, v);
                    if (uri == null) throw new Exception("MediaStore non ha creato il file Download");
                    copyFileToUri(apk, uri);
                    ContentValues done = new ContentValues();
                    done.put(MediaStore.Downloads.IS_PENDING, 0);
                    r.update(uri, done, null, null);
                    setStatus("PRONTO: " + name + "\nSalvato in Download.\n\nAprilo dal file manager per installarlo. L'Updater 3.0 non possiede REQUEST_INSTALL_PACKAGES e non chiama PackageInstaller.");
                } catch (Exception e) {
                    setStatus("APK CLEAN creato ma salvataggio automatico fallito: " + safeMessage(e) +
                            "\nPremi 'Esporta ultimo APK CLEAN' per scegliere manualmente dove salvarlo.");
                }
            }).start();
        } else {
            Intent i = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("application/vnd.android.package-archive");
            i.putExtra(Intent.EXTRA_TITLE, fileName);
            startActivityForResult(i, SAVE_APK);
        }
    }

    private void download(String rawUrl, File out) throws Exception {
        String current = rawUrl;
        HttpURLConnection c = null;
        for (int redirects = 0; redirects < 6; redirects++) {
            c = (HttpURLConnection) new URL(current).openConnection();
            c.setConnectTimeout(20000);
            c.setReadTimeout(90000);
            c.setInstanceFollowRedirects(false);
            c.setRequestProperty("User-Agent", "FCTV-Clean-Updater/3.0 Android");
            c.setRequestProperty("Accept", "application/vnd.android.package-archive,*/*");
            int code = c.getResponseCode();
            if (code >= 300 && code < 400) {
                String next = c.getHeaderField("Location");
                c.disconnect();
                if (next == null || next.isEmpty()) throw new Exception("Redirect download senza Location");
                current = new URL(new URL(current), next).toString();
                if (!current.toLowerCase(Locale.ROOT).startsWith("https://")) throw new Exception("Redirect non HTTPS rifiutato");
                continue;
            }
            if (code != 200) throw new Exception("Download APK HTTP " + code);
            break;
        }
        if (c == null || c.getResponseCode() != 200) throw new Exception("Troppi redirect download");

        long expected = c.getContentLengthLong();
        if (expected > 150L * 1024L * 1024L) throw new Exception("APK remoto troppo grande: " + expected);

        try (InputStream is = c.getInputStream(); OutputStream os = new FileOutputStream(out)) {
            byte[] buf = new byte[65536];
            long total = 0;
            long nextUi = 1024L * 1024L;
            int n;
            while ((n = is.read(buf)) > 0) {
                os.write(buf, 0, n);
                total += n;
                if (total >= nextUi) {
                    final long mb = total / (1024L * 1024L);
                    setStatus("Download FCTV33... " + mb + " MB");
                    nextUi += 1024L * 1024L;
                }
                if (total > 150L * 1024L * 1024L) throw new Exception("Download APK oltre limite sicurezza");
            }
            if (total < 1024L * 1024L) throw new Exception("Download troppo piccolo per essere FCTV33: " + total + " bytes");
            if (expected > 0 && total != expected) throw new Exception("Download incompleto: " + total + "/" + expected);
        } finally {
            c.disconnect();
        }
    }

    private static HttpURLConnection openGet(String url) throws Exception {
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        c.setConnectTimeout(15000);
        c.setReadTimeout(30000);
        c.setRequestMethod("GET");
        c.setRequestProperty("Accept", "application/json");
        c.setRequestProperty("User-Agent", "FCTV-Clean-Updater/3.0 Android");
        return c;
    }

    private void copyUri(Uri uri, File out) throws Exception {
        try (InputStream is = getContentResolver().openInputStream(uri); OutputStream os = new FileOutputStream(out)) {
            if (is == null) throw new Exception("URI APK non leggibile");
            copy(is, os);
        }
    }

    private void copyFileToUri(File file, Uri uri) throws Exception {
        try (InputStream is = new FileInputStream(file); OutputStream os = getContentResolver().openOutputStream(uri, "w")) {
            if (os == null) throw new Exception("Destinazione non scrivibile");
            copy(is, os);
        }
    }

    private static void copy(InputStream is, OutputStream os) throws Exception {
        byte[] b = new byte[65536];
        int n;
        while ((n = is.read(b)) > 0) os.write(b, 0, n);
        os.flush();
    }

    private static String readText(InputStream is) throws Exception {
        if (is == null) return "";
        try (InputStream input = is; ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            copy(input, out);
            return out.toString("UTF-8");
        }
    }

    private static String sha256Hex(byte[] data) throws Exception {
        byte[] d = MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder s = new StringBuilder(d.length * 2);
        for (byte b : d) s.append(String.format(Locale.ROOT, "%02X", b & 0xff));
        return s.toString();
    }

    private static String sanitizeFilePart(String s) {
        if (s == null || s.isEmpty()) return "latest";
        return s.replaceAll("[^0-9A-Za-z._-]", "_");
    }

    private static String safeMessage(Exception e) {
        String s = e.getMessage();
        return s == null || s.isEmpty() ? e.getClass().getSimpleName() : s;
    }

    private static String limit(String s, int n) {
        return s.length() <= n ? s : s.substring(0, n) + "...";
    }

    private static final class RemoteVersion {
        int code;
        String name;
        int minCode;
        String apkUrl;
        String downloadUrl;
        String title;
        String msg;

        static RemoteVersion from(JSONObject o) {
            RemoteVersion v = new RemoteVersion();
            v.code = optIntFlexible(o, "vCode");
            v.minCode = optIntFlexible(o, "minCode");
            v.name = o.optString("vName", "");
            if (v.name.isEmpty() && v.code > 0) v.name = String.valueOf(v.code);
            v.apkUrl = o.optString("apkUrl", "");
            v.downloadUrl = o.optString("downloadUrl", "");
            v.title = o.optString("title", "");
            v.msg = o.optString("msg", "");
            return v;
        }

        static int optIntFlexible(JSONObject o, String key) {
            Object x = o.opt(key);
            if (x instanceof Number) return ((Number) x).intValue();
            if (x != null) {
                try { return Integer.parseInt(String.valueOf(x)); } catch (Exception ignored) {}
            }
            return 0;
        }
    }

    private static final class InstalledState {
        boolean present;
        long versionCode;
        String versionName = "";
        String signerSha256 = "";
    }

    /*
     * Sanitizer FCTV33 fail-closed.
     * Le patch funzionali sono applicate ai metodi dell'app FCTV, non a classi Android.
     * Se i metodi cambiano/sono ambigui, l'aggiornamento viene rifiutato.
     */
    static final class Sanitizer {
        private static final String GA_CLASS = "Lcom/rblive/common/utils/GA;";
        private static final String GA_METHOD = "postEvent";

        private static final String[][] BOOL_METHODS = {
                {"Lcom/rblive/common/AppEnv;", "getAD_ENABLE"},
                {"Lcom/rblive/common/manager/ADManager;", "isEnableAD"},
                {"Lcom/rblive/common/model/entity/ListBannerConfig;", "getEnable"},
                {"Lcom/rblive/common/model/entity/MoneTagConfig;", "getEnable"},
                {"Lcom/rblive/common/model/entity/PlayerDirectLink;", "getEnable"},
                {"Lcom/rblive/common/model/entity/PlayerInterstitialConfig;", "getEnable"}
        };

        private static final String[] HOSTS = {
                "app-measurement.com",
                "firebase-settings.crashlytics.com",
                "google-analytics.com",
                "googleadservices.com",
                "doubleclick.net",
                "mixpanel.com",
                "pagead2.googlesyndication.com"
        };

        static void sanitize(File input, File output) throws Exception {
            Map<String, byte[]> replaced = new LinkedHashMap<>();
            Map<String, Integer> methodHits = new HashMap<>();
            Map<String, Integer> hostHits = new HashMap<>();
            int gaHits = 0;
            boolean analyticsMarker = false;
            boolean crashMarker = false;

            for (String[] m : BOOL_METHODS) methodHits.put(m[0] + "->" + m[1], 0);
            for (String h : HOSTS) hostHits.put(h, 0);

            try (ZipFile z = new ZipFile(input)) {
                Enumeration<? extends ZipEntry> en = z.entries();
                while (en.hasMoreElements()) {
                    ZipEntry entry = en.nextElement();
                    String dexName = entry.getName();
                    if (!dexName.matches("classes([0-9]+)?\\.dex")) continue;

                    byte[] data = readAll(z.getInputStream(entry));
                    Dex dex = new Dex(data);

                    analyticsMarker |= containsAscii(dex.data, "FirebaseAnalytics");
                    crashMarker |= containsAscii(dex.data, "FirebaseCrashlytics");

                    int ga = dex.findMethod(GA_CLASS, GA_METHOD, "V");
                    if (ga == -2) throw new Exception("GA.postEvent ambiguo");
                    if (ga >= 0) {
                        dex.patchCode(ga, new int[]{0x000e}); /* return-void */
                        gaHits++;
                    }

                    for (String[] m : BOOL_METHODS) {
                        int off = dex.findMethod(m[0], m[1], "Z");
                        if (off == -2) throw new Exception("Metodo ambiguo: " + m[0] + "->" + m[1]);
                        if (off >= 0) {
                            /* const/4 v0, #0 ; return v0 */
                            dex.patchCode(off, new int[]{0x0012, 0x000f});
                            String key = m[0] + "->" + m[1];
                            methodHits.put(key, methodHits.get(key) + 1);
                        }
                    }

                    for (String h : HOSTS) {
                        int n = replaceAscii(dex.data, h, invalid(h));
                        hostHits.put(h, hostHits.get(h) + n);
                    }

                    dex.fix();
                    replaced.put(dexName, dex.data);
                }
            }

            if (replaced.isEmpty()) throw new Exception("Nessun classes*.dex trovato");
            if (gaHits != 1) throw new Exception("Layout non supportato: GA.postEvent hits=" + gaHits);

            for (String[] m : BOOL_METHODS) {
                String key = m[0] + "->" + m[1];
                int n = methodHits.get(key);
                if (n != 1) throw new Exception("Layout non supportato: " + key + " hits=" + n);
            }

            if (analyticsMarker && hostHits.get("app-measurement.com") == 0) {
                throw new Exception("Firebase Analytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");
            }
            if (crashMarker && hostHits.get("firebase-settings.crashlytics.com") == 0) {
                throw new Exception("Crashlytics presente ma endpoint noto non localizzato: sanitizer da aggiornare");
            }

            try (ZipFile zin = new ZipFile(input); ZipOutputStream zout = new ZipOutputStream(new FileOutputStream(output))) {
                Enumeration<? extends ZipEntry> en = zin.entries();
                while (en.hasMoreElements()) {
                    ZipEntry old = en.nextElement();
                    String n = old.getName();
                    String u = n.toUpperCase(Locale.ROOT);

                    /* Togliamo la firma originale: dopo la modifica non sarebbe piu' valida. */
                    if (u.startsWith("META-INF/") &&
                            (u.endsWith(".RSA") || u.endsWith(".DSA") || u.endsWith(".EC") ||
                                    u.endsWith(".SF") || u.endsWith("MANIFEST.MF"))) {
                        continue;
                    }

                    byte[] bytes = replaced.containsKey(n) ? replaced.get(n) : readAll(zin.getInputStream(old));
                    ZipEntry ne = new ZipEntry(n);
                    ne.setTime(old.getTime());
                    ne.setComment(old.getComment());
                    ne.setExtra(old.getExtra());
                    ne.setMethod(old.getMethod());
                    if (old.getMethod() == ZipEntry.STORED) {
                        ne.setSize(bytes.length);
                        CRC32 crc = new CRC32();
                        crc.update(bytes);
                        ne.setCrc(crc.getValue());
                    }
                    zout.putNextEntry(ne);
                    if (!old.isDirectory()) zout.write(bytes);
                    zout.closeEntry();
                }
            }
        }

        private static String invalid(String h) {
            if (h.length() < 8) return repeat('x', h.length());
            return repeat('x', h.length() - 8) + ".invalid";
        }

        private static String repeat(char c, int n) {
            char[] a = new char[n];
            Arrays.fill(a, c);
            return new String(a);
        }

        private static boolean containsAscii(byte[] b, String s) throws Exception {
            byte[] a = s.getBytes("UTF-8");
            outer:
            for (int i = 0; i <= b.length - a.length; i++) {
                for (int j = 0; j < a.length; j++) if (b[i + j] != a[j]) continue outer;
                return true;
            }
            return false;
        }

        private static int replaceAscii(byte[] b, String old, String neu) throws Exception {
            byte[] a = old.getBytes("UTF-8");
            byte[] r = neu.getBytes("UTF-8");
            if (a.length != r.length) throw new Exception("Sostituzione host con lunghezza diversa");
            int hits = 0;
            outer:
            for (int i = 0; i <= b.length - a.length; i++) {
                for (int j = 0; j < a.length; j++) if (b[i + j] != a[j]) continue outer;
                System.arraycopy(r, 0, b, i, r.length);
                hits++;
                i += a.length - 1;
            }
            return hits;
        }

        private static byte[] readAll(InputStream is) throws Exception {
            try (InputStream input = is; ByteArrayOutputStream out = new ByteArrayOutputStream()) {
                byte[] b = new byte[65536];
                int n;
                while ((n = input.read(b)) > 0) out.write(b, 0, n);
                return out.toByteArray();
            }
        }
    }

    static final class Dex {
        byte[] data;
        String[] strings;
        String[] types;
        Proto[] protos;
        Method[] methods;
        Map<String, List<ClassMethod>> classMethods = new HashMap<>();

        Dex(byte[] d) throws Exception {
            data = d;
            parse();
        }

        int u16(int o) {
            return (data[o] & 255) | ((data[o + 1] & 255) << 8);
        }

        int u32(int o) {
            return (data[o] & 255) | ((data[o + 1] & 255) << 8) |
                    ((data[o + 2] & 255) << 16) | ((data[o + 3] & 255) << 24);
        }

        void put16(int o, int v) {
            data[o] = (byte) v;
            data[o + 1] = (byte) (v >>> 8);
        }

        int[] uleb(int o) {
            int v = 0;
            int s = 0;
            while (true) {
                int x = data[o++] & 255;
                v |= (x & 0x7f) << s;
                if ((x & 0x80) == 0) return new int[]{v, o};
                s += 7;
            }
        }

        String cstr(int o) throws Exception {
            int[] q = uleb(o);
            o = q[1];
            int e = o;
            while (data[e] != 0) e++;
            return new String(data, o, e - o, "UTF-8");
        }

        void parse() throws Exception {
            int ss = u32(0x38), so = u32(0x3c);
            int ts = u32(0x40), to = u32(0x44);
            int ps = u32(0x48), po = u32(0x4c);
            int ms = u32(0x58), mo = u32(0x5c);
            int cs = u32(0x60), co = u32(0x64);

            strings = new String[ss];
            for (int i = 0; i < ss; i++) strings[i] = cstr(u32(so + 4 * i));

            types = new String[ts];
            for (int i = 0; i < ts; i++) types[i] = strings[u32(to + 4 * i)];

            protos = new Proto[ps];
            for (int i = 0; i < ps; i++) {
                int ret = u32(po + 12 * i + 4);
                protos[i] = new Proto(types[ret]);
            }

            methods = new Method[ms];
            for (int i = 0; i < ms; i++) {
                int cls = u16(mo + 8 * i);
                int proto = u16(mo + 8 * i + 2);
                int name = u32(mo + 8 * i + 4);
                methods[i] = new Method(types[cls], strings[name], protos[proto].ret);
            }

            for (int i = 0; i < cs; i++) {
                int clsIdx = u32(co + 32 * i);
                int off = u32(co + 32 * i + 24);
                if (off == 0) continue;
                String cls = types[clsIdx];

                int[] q = uleb(off); int sf = q[0]; off = q[1];
                q = uleb(off); int inf = q[0]; off = q[1];
                q = uleb(off); int dm = q[0]; off = q[1];
                q = uleb(off); int vm = q[0]; off = q[1];

                for (int k = 0; k < sf + inf; k++) {
                    q = uleb(off); off = q[1];
                    q = uleb(off); off = q[1];
                }

                List<ClassMethod> list = new ArrayList<>();
                for (int count : new int[]{dm, vm}) {
                    int methodIdx = 0;
                    for (int k = 0; k < count; k++) {
                        q = uleb(off); methodIdx += q[0]; off = q[1];
                        q = uleb(off); off = q[1];
                        q = uleb(off); int code = q[0]; off = q[1];
                        list.add(new ClassMethod(methodIdx, code));
                    }
                }
                classMethods.put(cls, list);
            }
        }

        int findMethod(String cls, String name, String ret) {
            List<ClassMethod> list = classMethods.get(cls);
            if (list == null) return -1;
            int found = -1;
            for (ClassMethod cm : list) {
                Method m = methods[cm.idx];
                if (m.name.equals(name) && m.ret.equals(ret)) {
                    if (found != -1) return -2;
                    found = cm.code;
                }
            }
            return found;
        }

        void patchCode(int codeOff, int[] units) throws Exception {
            if (codeOff <= 0) throw new Exception("code_off non valido");
            int insns = codeOff + 16;
            for (int i = 0; i < units.length; i++) put16(insns + 2 * i, units[i]);
        }

        void fix() throws Exception {
            MessageDigest sha = MessageDigest.getInstance("SHA-1");
            sha.update(data, 32, data.length - 32);
            byte[] s = sha.digest();
            System.arraycopy(s, 0, data, 12, 20);

            Adler32 a = new Adler32();
            a.update(data, 12, data.length - 12);
            long v = a.getValue();
            data[8] = (byte) v;
            data[9] = (byte) (v >>> 8);
            data[10] = (byte) (v >>> 16);
            data[11] = (byte) (v >>> 24);
        }

        static final class Proto {
            String ret;
            Proto(String r) { ret = r; }
        }

        static final class Method {
            String cls, name, ret;
            Method(String c, String n, String r) { cls = c; name = n; ret = r; }
        }

        static final class ClassMethod {
            int idx, code;
            ClassMethod(int i, int c) { idx = i; code = c; }
        }
    }
}
