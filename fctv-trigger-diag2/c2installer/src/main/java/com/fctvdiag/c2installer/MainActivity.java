package com.fctvdiag.c2installer;

import android.app.Activity;
import android.content.pm.PackageInstaller;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    @Override public void onCreate(Bundle b) {
        super.onCreate(b);
        PackageInstaller pi=getPackageManager().getPackageInstaller();
        PackageInstaller.SessionParams p=new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
        TextView v=new TextView(this);
        v.setText("C2: riferimento a PackageInstaller presente. Nessun REQUEST_INSTALL_PACKAGES e nessuna sessione creata/committata. class="+pi.getClass().getName()+" params="+p.getClass().getName());
        v.setTextSize(18);
        v.setPadding(32,32,32,32);
        setContentView(v);
    }
}
