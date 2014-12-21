package com.greatlittleapps.nzbm.service;

import com.greatlittleapps.nzbm.Paths;
import com.greatlittleapps.utility.Utility;


public class Nzbget 
{
	private boolean running = false;
	private Thread thread = null;
	
	public boolean start()
	{
		if( running || thread != null )
		{
			Utility.loge( "could not start nzbget, already running" );
			return false;
		}
		
		thread = new Thread( new Runnable()
		{
			public void run()
			{
				Utility.log( "nzbget thread running" );
				running = true;

				// Run !
				String[] args = { 
						"none", "-n", "-s", "-c", Paths.config };
				int ret = main( args );
	
				// Stopped
				running = false;
				Utility.log( "nzbget thread returned (status="+ret+")" );
			}
		});
		
		Utility.log( "starting nzbget thread" );
		thread.start();
		return true;
	}

	public void stop()
	{
		Utility.log( "waiting for nzbget thread" );
		shutdown();
		try 
		{
			thread.join();
			thread = null;
		} 
		catch (InterruptedException e) 
		{
			e.printStackTrace();
		}
	}
	
	public boolean isRunning()
	{
		return running;
	}
	
	
	public String getVersion()
	{
		return version();
	}
	public boolean addNZB( String path )
	{
		return append( path, "", false );
	}
	public boolean addNZBURL( String path )
	{
		return appendurl( path, "", false );
	}

	// Native functions
	private native int main( String[] pArgs );
	private native void shutdown();
	private native String version();
	private native boolean append( String path, String category, boolean addToTop );
	private native boolean appendurl( String path, String category, boolean addToTop );
	
	static 
	{
        System.loadLibrary( "nzbget" );
    }
}

