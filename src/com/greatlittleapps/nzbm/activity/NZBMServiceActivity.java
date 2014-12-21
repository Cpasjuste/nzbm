package com.greatlittleapps.nzbm.activity;

import com.greatlittleapps.nzbm.service.Nzbget;
import com.greatlittleapps.nzbm.service.NzbmService;
import com.greatlittleapps.utility.Utility;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;

public class NZBMServiceActivity extends Activity 
{
	public NzbmService nzbservice;
	public Nzbget nzbget;
	public boolean isBound = false;
	
	@Override
    public void onCreate( Bundle savedInstanceState ) 
    {
        super.onCreate(savedInstanceState);
    }
	@Override
    public void onDestroy()
    {
		this.unbindService();
    	super.onDestroy();
    }
	
	protected void onServiceBindStart()
	{
		Utility.log( "onBindStart" );
	}
	protected void onServiceBindEnd( final boolean success, final String info )
	{
		Utility.log( "onBindEnd: "+success+" ("+info+")" );
		if( success )
		{
			this.onServiceStarted();
			nzbget = nzbservice.nzbget;
		}
	}
	
	protected void onServiceStopped()
	{
		Utility.log( "onServiceStopped" );
	}
	protected void onServiceStarted()
	{
		Utility.log( "onServiceStarted" );
	}
	
	public void startService()
	{
		startService( new Intent( NZBMServiceActivity.this, NzbmService.class ) );
		this.bindService();
	}
	
	public void stopService()
	{
		stopService( new Intent( NZBMServiceActivity.this, NzbmService.class ) );
		this.unbindService();
		this.onServiceStopped();
	}
	
	public void bindService() 
    {
    	Utility.log( "bind" );
    	
    	this.onServiceBindStart();
    	
    	if( isBound && nzbservice != null )
    	{
    		this.onServiceBindEnd( true, "service already bound" );
    		return;
    	}
    	
    	isBound = false;
    	if( bindService( new Intent( NZBMServiceActivity.this, NzbmService.class ), connection, Context.BIND_AUTO_CREATE ) )
    	{
    		isBound = true;
    		this.connect();
    	}
	    else
	    	this.onServiceBindEnd( false, "could not bind service" );

        Utility.log( "bindService: " + isBound );	
    }
    
	public void unbindService() 
    {
    	Utility.log( "unbind" );
        if( this.isBound ) 
        {
        	this.unbindService( connection );
        	this.isBound = false;
        }
    }
	
    private void connect()
    {
    	Thread t = new Thread( new Runnable()
		{
			@Override
			public void run() 
			{
				int retry = 1;
				boolean running = false;
				
				while( retry < 11 )
				{
					Utility.log( "waiting for nzbget server..." );
					
					try 
					{
						Thread.sleep( 1000 );
					} 
					catch (InterruptedException e) 
					{
						e.printStackTrace();
					}

					if( nzbservice != null && nzbservice.isRunning() )
					{
						running = true;
						break;
					}
					retry++;
				}
				
				if( running )
					onServiceBindEnd( true, "connected to service" );
				else
					onServiceBindEnd( false, "could not connect to service" );
			}
		});
		t.start();
    }
    
    private ServiceConnection connection = new ServiceConnection() 
    {
        public void onServiceConnected(ComponentName className, IBinder service) 
        {
        	nzbservice = ((NzbmService.LocalBinder)service).getService();
        }

        public void onServiceDisconnected(ComponentName className)
        {
        	nzbservice = null;
        }
    };
}

