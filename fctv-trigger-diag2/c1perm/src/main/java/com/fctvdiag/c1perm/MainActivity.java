package com.fctvdiag.c1perm;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    @Override public void onCreate(Bundle b) {
        super.onCreate(b);
        TextView v=new TextView(this);
        v.setText("C1: solo REQUEST_INSTALL_PACKAGES. Nessun PackageInstaller, nessun APK modificato.");
        v.setTextSize(20);
        v.setPadding(32,32,32,32);
        setContentView(v);
    }
}
