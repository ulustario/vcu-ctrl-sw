package com.fctvclean.diag;
import android.app.*;import android.os.*;import android.widget.*;import com.android.apksig.ApkSigner;
public class MainActivity extends Activity{public void onCreate(Bundle b){super.onCreate(b);TextView v=new TextView(this);v.setPadding(32,32,32,32);v.setTextSize(19);v.setText("DIAG D — libreria apksig incorporata\n\nNessun REQUEST_INSTALL_PACKAGES, nessuna manipolazione DEX e nessuna installazione.\n\nClasse caricata: "+ApkSigner.class.getName());setContentView(v);}}
