package com.greatlittleapps.nzbm.activity;

import java.io.File;
import java.lang.reflect.Method;

import com.greatlittleapps.utility.Preferences;
import com.greatlittleapps.utility.Utility;
import com.greatlittleapps.utility.UtilityDecompressRaw;
import com.greatlittleapps.utility.UtilityMessage;
import com.greatlittleapps.billing.utils.IabHelper;
import com.greatlittleapps.billing.utils.IabResult;
import com.greatlittleapps.billing.utils.Inventory;
import com.greatlittleapps.billing.utils.Purchase;
import com.greatlittleapps.nzbm.Config;
import com.greatlittleapps.nzbm.Paths;
import com.greatlittleapps.nzbm.R;
import com.greatlittleapps.nzbm.views.ServerView;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;

import com.google.android.gms.ads.*;

public class MainActivity extends NZBMServiceActivity 
{
	private String SKU = "com.greatlittleapps.nzbm.support";
	
	private Config conf;
	private UtilityMessage dialog;
	private ServerView serverView;
	private IabHelper mHelper;
	private AdView adView;
	private int launchCount;
	private int launchCountTrigger = 10;
	private boolean showAdds = true;
	
	@Override
    public void onCreate( Bundle savedInstanceState )
    {
		super.onCreate(savedInstanceState);
		setContentView( R.layout.webview );

		serverView = (ServerView)this.findViewById(R.id.ServerView);
		dialog = new UtilityMessage( MainActivity.this );
		Paths.unrar = this.getFilesDir().getAbsolutePath();
        if( new File( Paths.webui ).exists() 
        		&& new File( Paths.unrar ).exists()
        		&& new File ( Paths.config ).exists() )
        {
        	conf = new Config( Paths.config );
        	Utility.logVisible( "NZBM DATA VERSIONCODE: " + conf.getVersionCode() );
        	if( conf.getVersionCode() != 140 ) {
        		Utility.logVisible( "data not up to date !" );
        		extractData();
        	} else {
        		startService();
        	}
        }
        else
        {
        	Utility.log( "missing webui directory or unrar binary..." );
        	extractData();
        }
       
        // IAP
        String base64EncodedPublicKey = 
        		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkh1dlB0LPZ1zGXXnmbM3K5V8jZ58VBFDmkQX77gq/LycA1S2aCX74rA1I871ZIVfqfGbBMGMUB6QnOHzV7zZhxazl/Jj8PIPR3S4psJGZDeCnbAI5Fm93IULnhpdO7sgiH68Q0iFlYL2IZQfIGy+zqtgkKq4SUf26Ypx/LfPg199znqI5XOrxtwaFxqawYSQEBik101HIDeWiT2fmfF1i/KyCG57oJuu61J84bbnsbfwi4OCw1nDLChiLfNAcXvSlNuSEtwpNcrP4fQb7KgxUynoL5XOE2iGvSJAB/3vAOQP6zzlhh+1FnO4QTx8EIry+k/IuhXJTmFbwIsiOJtFFwIDAQAB";
    	mHelper = new IabHelper(this, base64EncodedPublicKey);
    	mHelper.startSetup(new IabHelper.OnIabSetupFinishedListener() 
    	{
    	   	  public void onIabSetupFinished(IabResult result) 
    	   	  {
    	   		  if (!result.isSuccess()) 
    	   		  {
    	   			  Utility.log( "In-app billing setup failed: " + result);
    	   			  loadAdd();
    	   		  } 
    	   		  else 
    	   		  {             
    	   			  Utility.log( "In-app billing setup success");
    	   			  mHelper.queryInventoryAsync(mReceivedInventoryListener);
    	   		  }
    	   	  }
    	});
    }

	@Override
	public void onPause() 
	{
		if(adView!=null)
			adView.pause();
		super.onPause();
	}

	@Override
	public void onResume() 
	{
		super.onResume();
		if(adView!=null)
			adView.resume();
	}
	
	@Override
	public void onDestroy() 
	{
		if(adView!=null)
			adView.destroy();
		adView = null;
		
		if(dialog!=null)
			dialog.Dispose();
		dialog = null;
		
		if (mHelper != null) 
			mHelper.dispose();
		mHelper = null;
		
	    super.onDestroy();
	}
	
	@Override  
	protected void onActivityResult( int requestCode, int resultCode, Intent intent )
	{
		if( requestCode == ServerView.FILECHOOSER_RESULTCODE )
	   	{  
			if ( this.serverView == null || this.serverView.uploadMessage == null ) 
				return;  
					 
			Uri result = (intent == null) || (resultCode != Activity.RESULT_OK) ? null : intent.getData();
			this.serverView.uploadMessage.onReceiveValue( result );  
			this.serverView.uploadMessage  = null; 
	   	}
		else if (!mHelper.handleActivityResult(requestCode, resultCode, intent)) 
		{     
			super.onActivityResult(requestCode, resultCode, intent);
		}
	}
	
	private void extractData()
    {
		Utility.delete( new File( Paths.webui ) );
		
    	dialog.show( "Please wait while extracting data..." );
    	UtilityDecompressRaw d = new UtilityDecompressRaw( this )
    	{
    		@Override
    		public void OnTerminate( boolean success )
    		{
    			dialog.hide();
    			if( success )
    			{
    				try {
						chmod(new File(Paths.unrar+"/unrar"), 755);
					} catch (Exception e) {
						e.printStackTrace();
					}
    					
    				runOnUiThread( new Runnable( )
    				{
    					@Override
    					public void run() 
    					{
    						conf = new Config( Paths.config );
    						startService();
    					}	
    				});
    			}
    			else
    				dialog.showMessageErrorExit( "Sorry, a fatal error occured while extracting data" );
    		}
    	};
    	d.add( "webui", Paths.webui+"/" );
    	d.add( "unrar", Paths.unrar+"/" );
    	d.process();
    }
	
	public int chmod(File path, int mode) throws Exception 
	{
		Class<?> fileUtils = Class.forName("android.os.FileUtils");
		Method setPermissions = fileUtils.getMethod("setPermissions", String.class, int.class, int.class, int.class);
		return (Integer) setPermissions.invoke(null, path.getAbsolutePath(), mode, -1, -1);
	}

	
	@Override
	protected void onServiceBindStart()
	{
		dialog.show( "Please wait while starting nzbget service" );
		super.onServiceBindStart();
	}
	
	@Override
	protected void onServiceBindEnd( final boolean success, final String info )
	{
		super.onServiceBindEnd( success, info );
		
		dialog.hide();
		if( !success )
		{
			dialog.showMessageError( info );
			return;
		}
	
		MainActivity.this.runOnUiThread( new Runnable()
		{
			@Override
			public void run() 
			{
				if( serverView != null )
				{
					serverView.load( 
							MainActivity.this, 
							"127.0.0.1", 
							conf.getControlPort(), 
							conf.getControlUsername(), 
							conf.getControlPassword() );
				}
			}
		});
	}
	
	/**************
	/* IAP + ADS   
	/**************/
	IabHelper.OnIabPurchaseFinishedListener mPurchaseFinishedListener 
		= new IabHelper.OnIabPurchaseFinishedListener() 
	{
		public void onIabPurchaseFinished(IabResult result, Purchase purchase) 
		{
			if (result.isFailure()) 
			{
				Utility.loge("onIabPurchaseFinished: Support SKU not purchased: " + result.getMessage() );
			}      
			else if (purchase.getSku().equals(SKU))
			{
				mHelper.queryInventoryAsync(mReceivedInventoryListener);
			} 
		}
	};

	IabHelper.QueryInventoryFinishedListener mReceivedInventoryListener 
		= new IabHelper.QueryInventoryFinishedListener() 
	{
		public void onQueryInventoryFinished(IabResult result, Inventory inventory) 
		{
			if( result.isSuccess() && inventory.hasPurchase(SKU))
			{
				showAdds = false;
				unloadAdd();
			}
			else
			{
				Utility.loge("onQueryInventoryFinished: Support SKU not purchased: " + result.getMessage() );
				loadAdd();
				launchCount = Preferences.getInt(MainActivity.this, "lc", 0) + 1;
				if(launchCount>=launchCountTrigger)
				{
					Preferences.setInt(MainActivity.this, "lc", 0);
					loadSupportMe();
				}
				else
				{
					Preferences.setInt(MainActivity.this, "lc", launchCount);
				}
			}
	    }
	};
	
	private void loadSupportMe()
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle( "Support My Work !" );
		builder.setMessage( "If you like this application, you can support my work and donate !\n\n" +
				"This donation will also remove advertissement." );
		builder.setPositiveButton( "Donate", new DialogInterface.OnClickListener() 
		{
			public void onClick(DialogInterface dialog, int id) 
			{
				mHelper.launchPurchaseFlow(MainActivity.this, SKU, 10001, mPurchaseFinishedListener, "");
			}
		});
		builder.setNegativeButton( "No Way!", new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface dialog, int id) {}
		});
		builder.create().show();
	}
	
	private void loadAdd()
	{
		if(!showAdds)
			return;
		
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
