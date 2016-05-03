package com.greatlittleapps.nzbm.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.greatlittleapps.nzbm.Config;
import com.greatlittleapps.nzbm.Paths;
import com.greatlittleapps.nzbm.R;
import com.greatlittleapps.utility.Utility;
import com.greatlittleapps.utility.UtilityDecompressRaw;
import com.greatlittleapps.utility.UtilityMessage;
import com.nononsenseapps.filepicker.FilePickerActivity;

import java.io.File;

import im.delight.android.webview.AdvancedWebView;

public class MainActivity extends NZBMServiceActivity
        implements AdvancedWebView.Listener, NavigationView.OnNavigationItemSelectedListener {

    private Config conf;
    private UtilityMessage dialog;
    private Paths paths;
    private AdvancedWebView serverView;
    private AdView adView;
    private int FILE_CODE = 7172;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        if(fab != null) {
            fab.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    Snackbar.make(view, "Select a file", Snackbar.LENGTH_LONG)
                            .setAction("Action", null).show();
                    openFilePicker();
                }
            });
        }
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
            isStoragePermissionGranted();
            conf = new Config(MainActivity.this);
            startService();
        } else {
            Utility.log("missing webui directory...");
            isStoragePermissionGranted();
            extractData();
        }
        loadAdd();
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

        if (id == R.id.nav_add_nzb) {
            openFilePicker();
        } else if (id == R.id.nav_shutdown) {
            if(nzbservice != null && nzbservice.isRunning()) {
                nzbservice.running = false;
                nzbservice.stopSelf();
            }
            finish();
        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {

        if (requestCode == FILE_CODE && resultCode == AppCompatActivity.RESULT_OK) {
            Uri uri = intent.getData();
            Utility.log("uri="+uri.getPath());
            if(this.nzbget != null) {
                this.nzbget.addNZB(uri.getPath());
            }
        } else {
            serverView.onActivityResult(requestCode, resultCode, intent);
        }
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

    private void openFilePicker() {

        Intent i = new Intent(this, FilePickerActivity.class);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, false);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, Environment.getExternalStorageDirectory().getPath());
        startActivityForResult(i, FILE_CODE);
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
        d.add("nzbgetconf", paths.nzbget + "/");
        d.process();
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

    private void loadAdd()
    {
        //if(!showAdds)
        //    return;

        unloadAdd();
        if( adView == null )
        {
            adView = (AdView)this.findViewById(R.id.adView);

            // Set the AdListener.
            adView.setAdListener(new AdListener() {
                /** Called when an ad is clicked and about to return to the application. */
                @Override
                public void onAdClosed() {
                    Utility.log( "onAdClosed");
                }

                /** Called when an ad failed to load. */
                @Override
                public void onAdFailedToLoad(int error) {
                    Utility.log( "error="+error);
                    adView.setVisibility(View.GONE);
                }

                /**
                 * Called when an ad is clicked and going to start a new Activity that will
                 * leave the application (e.g. breaking out to the Browser or Maps
                 * application).
                 */
                @Override
                public void onAdLeftApplication() {
                    Utility.log( "onAdLeftApplication");
                }

                /**
                 * Called when an Activity is created in front of the app (e.g. an
                 * interstitial is shown, or an ad is clicked and launches a new Activity).
                 */
                @Override
                public void onAdOpened() {
                    Utility.log( "onAdOpened");
                }

                /** Called when an ad is loaded. */
                @Override
                public void onAdLoaded() {
                    Utility.log( "onAdLoaded");
                    adView.setVisibility(View.VISIBLE);
                }
            });
        }
        //adView.setVisibility(View.VISIBLE);
        AdRequest adRequest = new AdRequest.Builder()
                .addTestDevice(AdRequest.DEVICE_ID_EMULATOR)
                .addTestDevice("50854E428E4AE737283AEFD68B7E36A3")
                .addTestDevice("B6014D29667B80ED30ECE79BA39F486F")
                .build();
        adView.loadAd(adRequest);
    }

    private void unloadAdd()
    {
        if( adView != null )
        {
            adView.setVisibility(View.GONE);
            //adView.destroy();
        }
    }
}
