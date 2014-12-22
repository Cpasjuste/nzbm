package com.greatlittleapps.nzbm.activity;

import com.greatlittleapps.utility.Utility;
import com.greatlittleapps.utility.UtilityMessage;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

public class Receiver extends NZBMServiceActivity
{
	private UtilityMessage dialog;
	private String nzbPath;
	
	@Override
    public void onCreate( Bundle savedInstanceState ) 
    {
        super.onCreate( savedInstanceState );
        dialog = new UtilityMessage( this );
        
        if( getIntent().getDataString() != null )
        {
        	nzbPath = getIntent().getData().getPath();
        	Utility.log( "nzbPath="+nzbPath );
        	this.startService();
        }
        else
        {
        	finish();
        }
    }

	@Override
	public void onDestroy()
	{
		if( dialog != null )
		{
			dialog.Dispose();
		}
		super.onDestroy();
	}
	
	@Override
    protected void onServiceBindStart()
	{
    	dialog.show( "Please wait while connecting to nzbget" );
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
		Receiver.this.runOnUiThread( new Runnable()
		{
			@Override
			public void run() 
			{
				dialogConfirmAddNzb( nzbPath );
			}
		});
	}
	
	private void dialogConfirmAddNzb( final String nzbPath )
    {
    	new AlertDialog.Builder( Receiver.this )
    	.setTitle( "Confirm" )
        .setMessage( "Add nzb to download queue ?\n\n" + nzbPath )
        .setPositiveButton( "Add", new DialogInterface.OnClickListener() 
        {
            public void onClick(DialogInterface dialog, int whichButton) 
            {
            	if( nzbPath.startsWith( "http") 
                		|| nzbPath.startsWith( "https")
                		|| nzbPath.startsWith( "ftp")
                		|| nzbPath.startsWith( "sftp") )
            	{
            		nzbget.addNZBURL( nzbPath );
            	}
            	else
            	{
            		nzbget.addNZB( nzbPath );
            	}
            	finish();
            }
        })
        .setNegativeButton( "Cancel", new DialogInterface.OnClickListener() 
        {
            public void onClick(DialogInterface dialog, int whichButton)
            {
            	finish();
            }
        })
        .create().show();
    }
}


