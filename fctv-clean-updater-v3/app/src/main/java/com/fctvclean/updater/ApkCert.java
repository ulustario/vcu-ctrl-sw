package com.fctvclean.updater;

import com.android.apksig.ApkVerifier;

import java.io.File;
import java.security.MessageDigest;
import java.security.cert.X509Certificate;
import java.util.List;
import java.util.Locale;

final class ApkCert {
    private ApkCert() {}

    static String signerSha256(File apk, String expectedSha256) throws Exception {
        ApkVerifier.Result result = new ApkVerifier.Builder(apk).build().verify();
        if (!result.isVerified()) {
            throw new Exception("Firma APK non valida secondo ApkVerifier");
        }

        List<X509Certificate> certs = result.getSignerCertificates();
        if (certs == null || certs.isEmpty()) {
            throw new Exception("APK senza certificato leggibile da ApkVerifier");
        }

        String first = null;
        for (X509Certificate cert : certs) {
            String fp = sha256Hex(cert.getEncoded());
            if (first == null) first = fp;
            if (expectedSha256 != null && expectedSha256.equalsIgnoreCase(fp)) return fp;
        }
        return first == null ? "" : first;
    }

    static void assertVerified(File apk) throws Exception {
        ApkVerifier.Result result = new ApkVerifier.Builder(apk).build().verify();
        if (!result.isVerified()) throw new Exception("APK CLEAN appena firmato non supera ApkVerifier");
        List<X509Certificate> certs = result.getSignerCertificates();
        if (certs == null || certs.isEmpty()) throw new Exception("APK CLEAN firmato senza certificato leggibile");
    }

    private static String sha256Hex(byte[] data) throws Exception {
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder b = new StringBuilder(digest.length * 2);
        for (byte v : digest) b.append(String.format(Locale.US, "%02X", v & 0xFF));
        return b.toString();
    }
}
