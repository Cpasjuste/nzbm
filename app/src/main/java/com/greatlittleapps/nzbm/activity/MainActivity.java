package com.greatlittleapps.nzbm.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.view.View;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;

import com.greatlittleapps.nzbm.Config;
import com.greatlittleapps.nzbm.Paths;
import com.greatlittleapps.nzbm.R;
import com.greatlittleapps.utility.Utility;
import com.greatlittleapps.utility.UtilityDecompressRaw;
import com.greatlittleapps.utility.UtilityMessage;

import java.io.File;
import java.lang.reflect.Method;

import im.delight.android.webview.AdvancedWebView;

public class MainActivity extends NZBMServiceActivity
        implements AdvancedWebView.Listener, NavigationView.OnNavigationItemSelectedListener {

    private Config conf;
    private UtilityMessage dialog;
    private Paths paths;
    private AdvancedWebView serverView;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.setDrawerListener(toggle);
        toggle.syncState();

        NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);

        serverView = (AdvancedWebView) this.findViewById(R.id.ServerView);
        dialog = new UtilityMessage(MainActivity.this);
        paths = new Paths(MainActivity.this);

        if (new File(paths.webui).exists()
                && new File(paths.nzbget).exists()
                && new File(paths.config).exists()) {
            Utility.log("data found");
            conf = new Config(MainActivity.this);
            startService();
        } else {
            Utility.log("missing webui directory or unrar binary...");
            extractData();
        }
        isStoragePermissionGranted();
    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @SuppressWarnings("StatementWithEmptyBody")
    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        // Handle navigation view item clicks here.
        int id = item.getItemId();

        if (id == R.id.nav_camera) {
            // Handle the camera action
        } else if (id == R.id.nav_gallery) {

        } else if (id == R.id.nav_slideshow) {

        } else if (id == R.id.nav_manage) {

        } else if (id == R.id.nav_share) {

        } else if (id == R.id.nav_send) {

        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        serverView.onActivityResult(requestCode, resultCode, intent);
        /*
        if (requestCode == ServerView.FILECHOOSER_RESULTCODE) {
            if (this.serverView == null || this.serverView.uploadMessage == null)
                return;

            Uri result = (intent == null) || (resultCode != AppCompatActivity.RESULT_OK) ? null : intent.getData();
            this.serverView.uploadMessage.onReceiveValue(result);
            this.serverView.uploadMessage = null;
        }
        */
        /* TODO: IAP
        else if (!mHelper.handleActivityResult(requestCode, resultCode, intent))
        {
            super.onActivityResult(requestCode, resultCode, intent);
        }
        */
    }

    @Override
    protected void onServiceBindStart() {
        dialog.show("Please wait while starting nzbget service");
        super.onServiceBindStart();
    }

    @Override
    protected void onServiceBindEnd(final boolean success, final String info) {
        super.onServiceBindEnd(success, info);

        dialog.hide();
        if (!success) {
            dialog.showMessageError(info);
            return;
        }

        MainActivity.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                serverView.loadUrl(
                        "http://127.0.0.1:"
                                +conf.getControlPort()
                                +"/"+conf.getControlUsername()+":"+conf.getControlPassword()+"/");
            }
        });
    }

    private void extractData() {
        Utility.delete(new File(paths.webui));

        Utility.log("Please wait while extracting data...");
        dialog.show("Please wait while extracting data...");
        UtilityDecompressRaw d = new UtilityDecompressRaw(this) {
            @Override
            public void OnTerminate(boolean success) {
                dialog.hide();
                if (success) {
                    Utility.log("extract data success");
                    try {
                        Utility.log("chmod " + paths.unrar + "/unrar");
                        chmod(new File(paths.unrar + "/unrar"), 755);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            conf = new Config(MainActivity.this);
                            startService();
                        }
                    });
                } else
                    dialog.showMessageErrorExit("Sorry, a fatal error occured while extracting data");
            }
        };
        d.add("webui", paths.webui + "/");
        d.add("unrar", paths.unrar + "/");
        d.add("nzbgetconf", paths.nzbget + "/");
        d.process();
    }

    public int chmod(File path, int mode) throws Exception {
        Class<?> fileUtils = Class.forName("android.os.FileUtils");
        Method setPermissions = fileUtils.getMethod("setPermissions", String.class, int.class, int.class, int.class);
        return (Integer) setPermissions.invoke(null, path.getAbsolutePath(), mode, -1, -1);
    }

    public boolean isStoragePermissionGranted() {
        if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED) {
                //Log.v(TAG,"Permission is granted");
                return true;
            } else {

                //Log.v(TAG,"Permission is revoked");
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
                return false;
            }
        } else { //permission is automatically granted on sdk<23 upon installation
            //Log.v(TAG,"Permission is granted");
            return true;
        }
    }

    @Override
    public void onPageStarted(String url, Bitmap favicon) {

    }

    @Override
    public void onPageFinished(String url) {

    }

    @Override
    public void onPageError(int errorCode, String description, String failingUrl) {

    }

    @Override
    public void onDownloadRequested(String url, String userAgent, String contentDisposition, String mimetype, long contentLength) {

    }

    @Override
    public void onExternalPageRequest(String url) {

    }
}
