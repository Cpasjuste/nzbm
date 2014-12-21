package com.greatlittleapps.nzbm.service;

import com.malothetoad.nzbgetrpc.Status;
import com.malothetoad.nzbgetrpc.NZBGetClient;
import com.greatlittleapps.nzbm.Config;
import com.greatlittleapps.nzbm.Paths;
import com.greatlittleapps.nzbm.R;
import com.greatlittleapps.nzbm.activity.MainActivity;
import com.greatlittleapps.utility.Utility;

import android.annotation.SuppressLint;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;
import android.provider.Settings;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Builder;
import android.support.v4.app.TaskStackBuilder;
import android.widget.Toast;

public class NzbmService extends Service
{
	public Nzbget nzbget;
	public NZBGetClient client = null;
	public boolean running = true;

	private Config config;
	private Thread thread;
	private Receiver receiver;
	private int sleepTime = 5000;
	// notifs
	private NotificationManager mNotificationManager;
	private Builder mNotifyBuilder;
	private int NOTIFICATION = 16031980;
	
	private PowerManager _powerManagement = null;
	private PowerManager.WakeLock _wakeLock = null;
	private WifiManager.WifiLock _wifiLock = null;
	
	@SuppressWarnings("deprecation")
	private int wifiSleepPolicy = Settings.System.WIFI_SLEEP_POLICY_DEFAULT;
	
	@Override
    public void onCreate() 
    {
		receiver = new Receiver(this);
		registerReceiver(receiver, new IntentFilter("com.greatlittleapps.nzbm.service.nzbm_stop"));
		
		//
		wifiSleepPolicy = this.getWifiPolicy();
		
        createNotification();
        
        // Start native nzbget
        nzbget = new Nzbget();
        nzbget.start();
        
        // Handle nzbget native configuration file
        config = new Config(Paths.config);
        
        // 
        client = new NZBGetClient( 
        		"127.0.0.1", config.getControlPort(), 
        		config.getControlUsername(), config.getControlPassword() );
        
        // Start main thread
        thread = new Thread( new Runnable()
		{
        	@Override
			public void run() 
			{
				Utility.log( "NzbmService thread started" );
				while( running )
				{
					if( client != null )
					{
						Status status;
						if( ( status = client.getStatus() ) != null )
						{
							boolean sleeping = status.isServerStandBy() || status.getDownloadRate() <= 0;
							
							if( sleeping )
							{
								keepOnStop();
								sleepTime = 5000;
							}
							else
							{
								keepOnStart();
								sleepTime = 1000;
							}
							
							String msg = "Waiting for nzb ...";
							if( !sleeping )
							{
								String remainingTime = 
										Utility.formatTimeLeft((status.getRemainingSizeMB()*1024)/Math.max(1,(status.getDownloadRate()/1024)));
			        			String remaining = status.getRemainingSizeMB()+" MB";	
								String downloadRate = (int)(status.getDownloadRate()/1024)+" KB/s";
								msg = remaining + " @ " + downloadRate + " (" + remainingTime + ")";
							}
							
							mNotifyBuilder.setContentText( msg );
						    mNotificationManager.notify(
						            NOTIFICATION,
						            mNotifyBuilder.build());
						}
					}
					try 
	        		{
	        			Thread.sleep( sleepTime );
	        		} 
	        		catch (InterruptedException e) {}
				}
				Utility.log( "NzbmService thread stopped" );
			}
		});
        thread.start();
        
        Toast.makeText( this, "nzbget service started", Toast.LENGTH_SHORT ).show();
    }
	
	public boolean isRunning()
	{
		return nzbget.isRunning();
	}
	
    @Override
    public void onDestroy() 
    {
    	this.unregisterReceiver(receiver);
    	keepOnStop();
    	running = false;
    	nzbget.stop();
        Toast.makeText( this, "nzbget service stopped", Toast.LENGTH_SHORT ).show();
        super.onDestroy();
        Utility.log("NZBMService.onDestroy");
    }
    
    @Override
    public IBinder onBind(Intent intent) 
    {
        return mBinder;
    }
	
	@Override
    public int onStartCommand( Intent intent, int flags, int startId ) 
    {
        Utility.log( "Received start id " + startId + ": " + intent );
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return START_STICKY;
    }

	public class LocalBinder extends Binder 
    {
    	public NzbmService getService() 
        {
            return NzbmService.this;
        }
    }

    private final IBinder mBinder = new LocalBinder();
   
    /**
     * Show a notification while this service is running.
     */
    @SuppressLint("NewApi")
	private void createNotification() 
    {
    	CharSequence text = "nzbget service is running";
    	
    	mNotificationManager =
    	        (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    	
    	mNotifyBuilder =
    	        new NotificationCompat.Builder(this)
    	        .setSmallIcon(R.drawable.ic_launcher)
    	        .setContentTitle("nzbm")
    	        .setContentText(text)
    	        .setOngoing(true);

    	Intent resultIntent = new Intent(this, MainActivity.class);
    	resultIntent.setAction(Intent.ACTION_MAIN);
        resultIntent.addCategory(Intent.CATEGORY_LAUNCHER);

    	TaskStackBuilder stackBuilder = TaskStackBuilder.create(this);
    	stackBuilder.addParentStack(MainActivity.class);
    	stackBuilder.addNextIntent(resultIntent);
    	PendingIntent resultPendingIntent = stackBuilder.getPendingIntent(0,PendingIntent.FLAG_UPDATE_CURRENT);
    	mNotifyBuilder.setContentIntent(resultPendingIntent);
    	
    	
    	Intent stopIntent = new Intent("com.greatlittleapps.nzbm.service.nzbm_stop");
    	PendingIntent pIntent = PendingIntent.getBroadcast(this, 0, stopIntent, 0);
    	mNotifyBuilder.addAction( R.drawable.ic_media_stop, "Stop", pIntent );
    	
    	startForeground( NOTIFICATION, mNotifyBuilder.build() );
    }
    
    @SuppressWarnings("deprecation")
	private int getWifiPolicy()
    {
    	if(Build.VERSION.SDK_INT < 17)
        {
	    	return Settings.System.getInt( 
					getContentResolver(), 
					Settings.System.WIFI_SLEEP_POLICY, 
					Settings.System.WIFI_SLEEP_POLICY_DEFAULT );
        }
    	return 0;
    }
    
    @SuppressWarnings("deprecation")
	private void setWifiPolicy( int policy )
    {
    	if(Build.VERSION.SDK_INT < 17)
        {
	    	Settings.System.putInt( 
					getContentResolver(),
					Settings.System.WIFI_SLEEP_POLICY, 
					policy );
        }
    }
    
	@SuppressLint("InlinedApi")
	@SuppressWarnings("deprecation")
	private void keepOnStart() 
 	{
		if( Build.VERSION.SDK_INT < 17 && getWifiPolicy() != Settings.System.WIFI_SLEEP_POLICY_NEVER )
        {
			setWifiPolicy( Settings.System.WIFI_SLEEP_POLICY_NEVER );
        }
		
		if (_powerManagement == null) 
    		_powerManagement = (PowerManager) getSystemService(Context.POWER_SERVICE);
    	
    	if (_wakeLock == null) 
    		_wakeLock = _powerManagement.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, "powerLock" );

        _wakeLock.acquire();
        
        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        if (wifiManager != null) 
        {
        	int lock = WifiManager.WIFI_MODE_FULL;
        	if( Build.VERSION.SDK_INT >= 12 )
        		lock |= WifiManager.WIFI_MODE_FULL_HIGH_PERF;
        	_wifiLock = wifiManager.createWifiLock( lock, "wifiLock" );
        	_wifiLock.acquire();
        }
	}

	private void keepOnStop() 
	{
		if( Build.VERSION.SDK_INT < 17 && getWifiPolicy() != wifiSleepPolicy )
        {
			setWifiPolicy( wifiSleepPolicy );
		}
		
		if ((_wifiLock != null) && (_wifiLock.isHeld())) 
			_wifiLock.release();
    	  
		if ((_wakeLock != null) && (_wakeLock.isHeld())) 
			_wakeLock.release();
	}
	
	public class Receiver extends BroadcastReceiver
	{
		NzbmService service;
		
		public Receiver(NzbmService srv)
		{
			service = srv;
		}
		
		@Override
		public void onReceive(Context context, Intent intent) 
		{
			service.running = false;
			service.stopSelf();
		}
	}
}


