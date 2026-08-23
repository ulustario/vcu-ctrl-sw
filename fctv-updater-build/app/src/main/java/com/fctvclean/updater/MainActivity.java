package com.fctvclean.updater;

import android.app.*;
import android.content.*;
import android.content.pm.*;
import android.net.Uri;
import android.os.*;
import android.provider.Settings;
import android.security.keystore.*;
import android.view.*;
import android.widget.*;

import com.android.apksig.ApkSigner;

import java.io.*;
import java.math.BigInteger;
import java.nio.*;
import java.security.*;
import java.security.cert.X509Certificate;
import java.util.*;
import java.util.zip.*;
import javax.security.auth.x500.X500Principal;

public class MainActivity extends Activity {
    static final int PICK_APK=1001;
    static final String TARGET="com.fctv77.app";
    static final String KEY_ALIAS="fctv_clean_local_signer_v1";
    TextView status;
    Button pick, installLast;
    File lastSigned;

    @Override public void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root=new LinearLayout(this); root.setOrientation(LinearLayout.VERTICAL); root.setPadding(28,28,28,28);
        TextView title=new TextView(this); title.setText("FCTV CLEAN Updater"); title.setTextSize(26); root.addView(title);
        status=new TextView(this); status.setText("Seleziona l'APK ufficiale FCTV. Verrà ripulito, firmato con una chiave locale non esportabile e poi passato a PackageInstaller."); status.setTextSize(16); root.addView(status);
        pick=new Button(this); pick.setText("Seleziona APK ufficiale"); root.addView(pick);
        installLast=new Button(this); installLast.setText("Installa ultimo APK CLEAN"); installLast.setEnabled(false); root.addView(installLast);
        setContentView(root);
        pick.setOnClickListener(v->chooseApk());
        installLast.setOnClickListener(v->{ if(lastSigned!=null) requestInstall(lastSigned); });
        Intent in=getIntent();
        if(Intent.ACTION_VIEW.equals(in.getAction()) && in.getData()!=null) processUri(in.getData());
    }

    void chooseApk(){
        Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT); i.addCategory(Intent.CATEGORY_OPENABLE); i.setType("application/vnd.android.package-archive"); startActivityForResult(i,PICK_APK);
    }
    @Override protected void onActivityResult(int req,int res,Intent data){ super.onActivityResult(req,res,data); if(req==PICK_APK && res==RESULT_OK && data!=null && data.getData()!=null) processUri(data.getData()); }

    void processUri(Uri uri){
        status.setText("Analisi APK...");
        new Thread(()->{
            try {
                File in=new File(getCacheDir(),"fctv_input.apk"); copyUri(uri,in);
                PackageInfo pi=getPackageManager().getPackageArchiveInfo(in.getAbsolutePath(),0);
                if(pi==null || !TARGET.equals(pi.packageName)) throw new Exception("Package non valido: atteso "+TARGET);
                File unsigned=new File(getCacheDir(),"fctv_clean_unsigned.apk");
                Sanitizer.sanitize(in,unsigned);
                File signed=new File(getCacheDir(),"fctv_clean_signed.apk");
                signApk(unsigned,signed);
                lastSigned=signed;
                runOnUiThread(()->{ status.setText("APK CLEAN creato e firmato. Ora installazione."); installLast.setEnabled(true); requestInstall(signed); });
            } catch(Exception e){ runOnUiThread(()->status.setText("BLOCCATO (fail-closed): "+e.getMessage())); }
        }).start();
    }

    void copyUri(Uri uri,File out) throws Exception {
        try(InputStream is=getContentResolver().openInputStream(uri); OutputStream os=new FileOutputStream(out)){ if(is==null) throw new IOException("URI non leggibile"); byte[] b=new byte[65536]; int n; while((n=is.read(b))>0) os.write(b,0,n); }
    }

    KeyStore.PrivateKeyEntry getSigningEntry() throws Exception {
        KeyStore ks=KeyStore.getInstance("AndroidKeyStore"); ks.load(null);
        if(!ks.containsAlias(KEY_ALIAS)){
            Calendar from=Calendar.getInstance(); Calendar to=Calendar.getInstance(); to.add(Calendar.YEAR,25);
            KeyPairGenerator kpg=KeyPairGenerator.getInstance(KeyProperties.KEY_ALGORITHM_RSA,"AndroidKeyStore");
            KeyGenParameterSpec spec=new KeyGenParameterSpec.Builder(KEY_ALIAS,KeyProperties.PURPOSE_SIGN|KeyProperties.PURPOSE_VERIFY)
                    .setKeySize(2048)
                    .setDigests(KeyProperties.DIGEST_SHA256,KeyProperties.DIGEST_SHA512)
                    .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
                    .setCertificateSubject(new X500Principal("CN=FCTV CLEAN LOCAL"))
                    .setCertificateSerialNumber(BigInteger.ONE)
                    .setCertificateNotBefore(from.getTime()).setCertificateNotAfter(to.getTime()).build();
            kpg.initialize(spec); kpg.generateKeyPair();
        }
        return (KeyStore.PrivateKeyEntry)ks.getEntry(KEY_ALIAS,null);
    }

    void signApk(File in,File out) throws Exception {
        KeyStore.PrivateKeyEntry e=getSigningEntry();
        ApkSigner.SignerConfig cfg=new ApkSigner.SignerConfig.Builder("FCTV CLEAN",e.getPrivateKey(),Collections.singletonList((X509Certificate)e.getCertificate())).build();
        new ApkSigner.Builder(Collections.singletonList(cfg)).setInputApk(in).setOutputApk(out).setV1SigningEnabled(true).setV2SigningEnabled(true).setV3SigningEnabled(false).build().sign();
    }

    void requestInstall(File apk){
        if(Build.VERSION.SDK_INT>=26 && !getPackageManager().canRequestPackageInstalls()){
            status.setText("Autorizza 'Installa app sconosciute' per FCTV Clean Updater, poi premi 'Installa ultimo APK CLEAN'.");
            Intent s=new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,Uri.parse("package:"+getPackageName())); startActivity(s); return;
        }
        new Thread(()->{
            try {
                PackageInstaller pi=getPackageManager().getPackageInstaller();
                PackageInstaller.SessionParams p=new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL); p.setAppPackageName(TARGET);
                int id=pi.createSession(p);
                try(PackageInstaller.Session session=pi.openSession(id); InputStream is=new FileInputStream(apk); OutputStream os=session.openWrite("base.apk",0,apk.length())){
                    byte[] b=new byte[65536]; int n; while((n=is.read(b))>0) os.write(b,0,n); session.fsync(os);
                    Intent result=new Intent(this,InstallResultReceiver.class); result.setAction("com.fctvclean.updater.INSTALL_RESULT");
                    PendingIntent pending=PendingIntent.getBroadcast(this,id,result,PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_MUTABLE);
                    session.commit(pending.getIntentSender());
                }
                runOnUiThread(()->status.setText("PackageInstaller avviato. Se la FCTV originale è ancora installata, Android può richiederne prima la disinstallazione perché la firma originale è diversa."));
            } catch(Exception e){ runOnUiThread(()->status.setText("Installazione non avviata: "+e.getMessage())); }
        }).start();
    }

    static class Sanitizer {
        static final String[][] BOOL_METHODS={
                {"Lcom/rblive/common/AppEnv;","getAD_ENABLE"},
                {"Lcom/rblive/common/manager/ADManager;","isEnableAD"},
                {"Lcom/rblive/common/model/entity/ListBannerConfig;","getEnable"},
                {"Lcom/rblive/common/model/entity/MoneTagConfig;","getEnable"},
                {"Lcom/rblive/common/model/entity/PlayerDirectLink;","getEnable"},
                {"Lcom/rblive/common/model/entity/PlayerInterstitialConfig;","getEnable"}
        };
        static final String[] HOSTS={"app-measurement.com","firebase-settings.crashlytics.com","google-analytics.com","googleadservices.com","doubleclick.net","mixpanel.com","pagead2.googlesyndication.com"};

        static void sanitize(File input,File output) throws Exception {
            Map<String,byte[]> replaced=new HashMap<>();
            try(ZipFile z=new ZipFile(input)){
                for(String dexName:new String[]{"classes.dex","classes2.dex"}){
                    ZipEntry e=z.getEntry(dexName); if(e==null) throw new Exception("Manca "+dexName);
                    byte[] d=readAll(z.getInputStream(e)); Dex dex=new Dex(d);
                    if("classes.dex".equals(dexName)) patchMethodExact(dex,"Lcom/rblive/common/utils/GA;","postEvent","V",new int[]{0x000e});
                    for(String[] m:BOOL_METHODS) patchMethodIfHere(dex,m[0],m[1],"Z",new int[]{0x0012,0x000f});
                    for(String h:HOSTS) replaceAscii(dex.data,h,invalid(h),!h.equals("pagead2.googlesyndication.com"));
                    dex.fix(); replaced.put(dexName,dex.data);
                }
            }
            for(String[] m:BOOL_METHODS){ int hits=0; for(byte[] d:replaced.values()) if(new Dex(d).findMethod(m[0],m[1],"Z")>=0) hits++; if(hits!=1) throw new Exception("Layout non supportato: "+m[0]+"->"+m[1]); }
            try(ZipFile zin=new ZipFile(input); ZipOutputStream zout=new ZipOutputStream(new FileOutputStream(output))){
                Enumeration<? extends ZipEntry> en=zin.entries();
                while(en.hasMoreElements()){
                    ZipEntry old=en.nextElement(); String n=old.getName(); String u=n.toUpperCase(Locale.ROOT);
                    if(u.startsWith("META-INF/")&&(u.endsWith(".RSA")||u.endsWith(".DSA")||u.endsWith(".EC")||u.endsWith(".SF")||u.endsWith("MANIFEST.MF"))) continue;
                    byte[] bytes=replaced.containsKey(n)?replaced.get(n):readAll(zin.getInputStream(old));
                    ZipEntry ne=new ZipEntry(n); ne.setTime(old.getTime()); ne.setComment(old.getComment()); ne.setExtra(old.getExtra()); ne.setMethod(old.getMethod());
                    if(old.getMethod()==ZipEntry.STORED){ ne.setSize(bytes.length); CRC32 c=new CRC32(); c.update(bytes); ne.setCrc(c.getValue()); }
                    zout.putNextEntry(ne); if(!old.isDirectory()) zout.write(bytes); zout.closeEntry();
                }
            }
        }
        static String invalid(String h){ return h.length()>=8?repeat('x',h.length()-8)+".invalid":repeat('x',h.length()); }
        static String repeat(char c,int n){ char[] a=new char[n]; Arrays.fill(a,c); return new String(a); }
        static byte[] readAll(InputStream is)throws Exception{ try(InputStream x=is; ByteArrayOutputStream o=new ByteArrayOutputStream()){ byte[] b=new byte[65536]; int n; while((n=x.read(b))>0)o.write(b,0,n); return o.toByteArray(); } }
        static void replaceAscii(byte[] b,String old,String neu,boolean required)throws Exception{ byte[] a=old.getBytes("UTF-8"), r=neu.getBytes("UTF-8"); if(a.length!=r.length)throw new Exception("Host length"); int hits=0; outer: for(int i=0;i<=b.length-a.length;i++){ for(int j=0;j<a.length;j++)if(b[i+j]!=a[j])continue outer; System.arraycopy(r,0,b,i,r.length); hits++; i+=a.length-1; } if(required&&hits==0)throw new Exception("Marker telemetria mancante: "+old); }
        static void patchMethodExact(Dex dex,String cls,String name,String ret,int[] units)throws Exception{ int off=dex.findMethod(cls,name,ret); if(off<0)throw new Exception("Metodo mancante: "+name); dex.patchCode(off,units); dex.fix(); }
        static void patchMethodIfHere(Dex dex,String cls,String name,String ret,int[] units)throws Exception{ int off=dex.findMethod(cls,name,ret); if(off>=0){ dex.patchCode(off,units); dex.fix(); } }
    }

    static class Dex {
        byte[] data; String[] strings,types; Proto[] protos; Method[] methods; Map<String,List<ClassMethod>> classMethods=new HashMap<>();
        Dex(byte[] d)throws Exception{ data=d; parse(); }
        int u16(int o){ return (data[o]&255)|((data[o+1]&255)<<8); }
        int u32(int o){ return (data[o]&255)|((data[o+1]&255)<<8)|((data[o+2]&255)<<16)|((data[o+3]&255)<<24); }
        void put16(int o,int v){ data[o]=(byte)v; data[o+1]=(byte)(v>>>8); }
        int[] uleb(int o){ int v=0,s=0; while(true){ int b=data[o++]&255; v|=(b&0x7f)<<s; if((b&0x80)==0)return new int[]{v,o}; s+=7; } }
        String cstr(int o)throws Exception{ int[] q=uleb(o); o=q[1]; int e=o; while(data[e]!=0)e++; return new String(data,o,e-o,"UTF-8"); }
        void parse()throws Exception{
            int ss=u32(0x38),so=u32(0x3c), ts=u32(0x40),to=u32(0x44), ps=u32(0x48),po=u32(0x4c), ms=u32(0x58),mo=u32(0x5c), cs=u32(0x60),co=u32(0x64);
            strings=new String[ss]; for(int i=0;i<ss;i++)strings[i]=cstr(u32(so+4*i));
            types=new String[ts]; for(int i=0;i<ts;i++)types[i]=strings[u32(to+4*i)];
            protos=new Proto[ps]; for(int i=0;i<ps;i++){ int r=u32(po+12*i+4), off=u32(po+12*i+8); protos[i]=new Proto(types[r]); }
            methods=new Method[ms]; for(int i=0;i<ms;i++){ int c=u16(mo+8*i),p=u16(mo+8*i+2),n=u32(mo+8*i+4); methods[i]=new Method(types[c],strings[n],protos[p].ret); }
            for(int i=0;i<cs;i++){ int clsIdx=u32(co+32*i), off=u32(co+32*i+24); if(off==0)continue; String cls=types[clsIdx]; int[] q=uleb(off); int sf=q[0]; off=q[1]; q=uleb(off); int inf=q[0]; off=q[1]; q=uleb(off); int dm=q[0]; off=q[1]; q=uleb(off); int vm=q[0]; off=q[1]; for(int k=0;k<sf+inf;k++){ q=uleb(off);off=q[1];q=uleb(off);off=q[1]; }
                List<ClassMethod> list=new ArrayList<>(); int[] counts={dm,vm}; for(int count:counts){ int midx=0; for(int k=0;k<count;k++){ q=uleb(off);midx+=q[0];off=q[1];q=uleb(off);off=q[1];q=uleb(off);int code=q[0];off=q[1];list.add(new ClassMethod(midx,code)); } } classMethods.put(cls,list);
            }
        }
        int findMethod(String cls,String name,String ret){ List<ClassMethod> l=classMethods.get(cls); if(l==null)return -1; int found=-1; for(ClassMethod cm:l){ Method m=methods[cm.idx]; if(m.name.equals(name)&&m.ret.equals(ret)){ if(found!=-1)return -2; found=cm.code; } } return found; }
        void patchCode(int codeOff,int[] units)throws Exception{ if(codeOff<=0)throw new Exception("code_off non valido"); int insns=codeOff+16; for(int i=0;i<units.length;i++)put16(insns+2*i,units[i]); }
        void fix()throws Exception{ MessageDigest sha=MessageDigest.getInstance("SHA-1"); sha.update(data,32,data.length-32); byte[] s=sha.digest(); System.arraycopy(s,0,data,12,20); Adler32 a=new Adler32(); a.update(data,12,data.length-12); long v=a.getValue(); data[8]=(byte)v; data[9]=(byte)(v>>>8); data[10]=(byte)(v>>>16); data[11]=(byte)(v>>>24); }
        static class Proto{ String ret; Proto(String r){ret=r;} }
        static class Method{ String cls,name,ret; Method(String c,String n,String r){cls=c;name=n;ret=r;} }
        static class ClassMethod{ int idx,code; ClassMethod(int i,int c){idx=i;code=c;} }
    }
}
