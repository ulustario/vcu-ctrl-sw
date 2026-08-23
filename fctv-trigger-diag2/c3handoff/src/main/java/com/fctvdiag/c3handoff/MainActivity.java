package com.fctvdiag.c3handoff;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

public class MainActivity extends Activity {
    @Override public void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout l=new LinearLayout(this); l.setOrientation(LinearLayout.VERTICAL); l.setPadding(32,32,32,32);
        TextView v=new TextView(this); v.setText("C3: nessun permesso d'installazione e nessun PackageInstaller. Usa solo il selettore documenti per consegnare/salvare un APK."); v.setTextSize(18); l.addView(v);
        Button x=new Button(this); x.setText("Crea file APK tramite sistema"); l.addView(x);
        x.setOnClickListener(q->{ Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT); i.addCategory(Intent.CATEGORY_OPENABLE); i.setType("application/vnd.android.package-archive"); i.putExtra(Intent.EXTRA_TITLE,"FCTV_clean_signed.apk"); startActivityForResult(i,7); });
        setContentView(l);
    }
}
