package com.greatlittleapps.nzbm.views;

import com.greatlittleapps.utility.Utility;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.util.AttributeSet;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class ServerView extends WebView
{
	public final static int FILECHOOSER_RESULTCODE=1;  
	public ValueCallback<Uri> uploadMessage;  
	
	private Activity activity;
	 
	public ServerView( Context context ) 
	{
		super( context );
		init();
	}
	
	public ServerView( Context context, AttributeSet attrs ) 
	{
		super( context, attrs );
		init();
	}
	
	public ServerView( Context context, AttributeSet attrs, int defStyle ) 
	{
		super( context, attrs, defStyle );
		init();
	}
	
	@SuppressLint({ "SetJavaScriptEnabled", "NewApi" })
	public void init()
	{
		if( isInEditMode() )
			return;
		
		Utility.log( "ServerView init!" );


		WebSettings settings = getSettings();
		settings.setJavaScriptEnabled(true);
		settings.setJavaScriptCanOpenWindowsAutomatically(true);
		settings.setDomStorageEnabled(true);

		// Prevent scaling
		this.setInitialScale( 100 );
		if( Build.VERSION.SDK_INT > 11 )
		{
			settings.setBuiltInZoomControls(true);
			settings.setDisplayZoomControls(false);
		}
		else
		{
			settings.setBuiltInZoomControls(false);
			settings.setSupportZoom(false);
		}

		setWebChromeClient( new WebChromeClient()
		{
			// http://m0s-programming.blogspot.fr/2011/02/file-upload-in-through-webview-on.html
			// The undocumented magic method override
			// Eclipse will swear at you if you try to put @Override here

			// android 4.1+
			public void openFileChooser(ValueCallback<Uri> uploadMsg, String acceptType, String capture)
			{
				Utility.log( "openFileChooser" );
				//activity.fileChooserOpen = true;
				uploadMessage = uploadMsg;
				Intent i = new Intent( Intent.ACTION_GET_CONTENT );
				i.addCategory( Intent.CATEGORY_OPENABLE );
				i.setType( "*/*" );
				activity.startActivityForResult( Intent.createChooser( i,"File Chooser" ), FILECHOOSER_RESULTCODE );
			}

			// android 3.0+
		    public void openFileChooser( ValueCallback<Uri> uploadMsg, String acceptType )
		    {
		    	openFileChooser( uploadMsg, "", "" );
		    }

		    // android < 3.0
		    @SuppressWarnings("unused")
			public void openFileChooser( ValueCallback<Uri> uploadMsg )
		    {
		        openFileChooser( uploadMsg, "" );
		    }

			public void onProgressChanged( WebView view, int progress )
			{
				//Utility.logd( this.getClass(), "Loading..." + progress );
			}
		});

		setWebViewClient( new WebViewClient()
		{
			public void onReceivedError( WebView view, int errorCode, String description, String failingUrl )
			{
			     Utility.log( "Oh no! " + description );
			}
		});
	}
	
	public void load( Activity pActivity, String ip, String port, String username, String passwd )
	{
		Utility.log( "loading server page" );
		activity = pActivity;
		loadUrl( "http://"+ip+":"+port+"/"+username+":"+passwd+"/" );
	}
}
