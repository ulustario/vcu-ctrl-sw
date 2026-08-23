package com.fctvclean.diag;
import android.app.*;import android.os.*;import android.widget.*;
public class MainActivity extends Activity{public void onCreate(Bundle b){super.onCreate(b);TextView v=new TextView(this);v.setPadding(32,32,32,32);v.setTextSize(20);v.setText("DIAG A — UI soltanto\n\nNessun Keystore, nessun permesso installazione, nessuna firma APK, nessuna manipolazione DEX.\n\nSe Play Protect blocca già questo APK, il problema non è la funzione updater.");setContentView(v);}}
