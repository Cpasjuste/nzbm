package com.greatlittleapps.nzbm.views;

import com.malothetoad.nzbgetrpc.NZBGetClient;
import com.malothetoad.nzbgetrpc.Status;
import com.greatlittleapps.nzbm.R;
import com.greatlittleapps.utility.Utility;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class DownloadIconView extends RelativeLayout 
{
	Status status;
	
	TextView rate;
	ImageView bg;
	ImageView anim;
	
	Drawable bg_downloading;
	Drawable bg_paused;
	
	Drawable anim_downloading;
	Drawable anim_paused;
	
	public DownloadIconView(Context context) 
	{
		super(context);
		init(context);
	}
	
	public DownloadIconView(Context context, AttributeSet attrs) 
	{
		super(context, attrs);
		init(context);
	}
	
	public DownloadIconView(Context context, AttributeSet attrs, int defStyle) 
	{
		super(context, attrs, defStyle);
		init(context);
	}
	
	
	public void update( Status s )
	{
		status = s;
		
		/*
		Utility.log( "isDownloadPaused:"+status.isDownloadPaused()
				+"|isDownload2Paused:"+status.isDownload2Paused() 
				+"|isServerPaused:"+status.isServerPaused() 
				+"|isServerStandBy:"+status.isServerStandBy()
				);
		*/
	
		if( status.isServerStandBy() )
		{
			rate.setText( "..." );
			if( anim.isShown() )
			{
				Utility.log( "status == paused" );
				anim.clearAnimation();
				anim.setVisibility( View.GONE );
				bg.setImageDrawable( bg_paused );
			}
		}
		else if( status.isDownloadPaused() || status.isDownload2Paused() || status.isServerPaused() )
		{
			rate.setText( ""+(int)(status.getDownloadRate()/1024) );
			if( !bg.getDrawable().equals( bg_paused ) )
			{
				Utility.log( "status == pausing" );
				bg.setImageDrawable( bg_paused );
				anim.setImageDrawable( anim_paused );
				rotate( anim );
			}
		}
		else
		{
			rate.setText( ""+(int)(status.getDownloadRate()/1024) );
			if( !bg.getDrawable().equals( bg_downloading ) )
			{
				Utility.log( "status == downloading" );
				bg.setImageDrawable( bg_downloading );
				anim.setImageDrawable( anim_downloading );
				anim.setVisibility( View.VISIBLE );
				rotate( anim );
			}
		}
	}
	
	void init( Context context )
	{
		LayoutInflater inflater = (LayoutInflater)context.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
		View v = inflater.inflate( R.layout.menu_icon, null );
	
		bg_downloading = getResources().getDrawable( R.drawable.download_green );
		bg_paused = getResources().getDrawable( R.drawable.download_orange );
		anim_downloading = getResources().getDrawable( R.drawable.download_anim_green );
		anim_paused = getResources().getDrawable( R.drawable.download_anim_orange );
		
		bg = (ImageView)v.findViewById( R.id.download_bg );
		anim = (ImageView)v.findViewById( R.id.download_anim );
		anim.setVisibility( View.GONE );
		rate = (TextView)v.findViewById( R.id.rate );
		rate.setText( "..." );
	
        addView( v );
        setOnClickListener( new OnClickListener() 
        {
            @Override
            public void onClick(View view) 
            {
            	if( status != null )
            	{
        			new Thread( new Runnable()
        			{
        				public void run() 
        				{
        					NZBGetClient nzbget = new NZBGetClient( "mydedibox.fr", "20887", "nzbget", "g4rknydm" );
        					if( status.isDownloadPaused() )
        					{
        						Utility.log( "resume" );
        						nzbget.resume();
        					}
        					else
        					{
        						Utility.log( "pause" );
        						nzbget.pause();
        					}
        				}
        			}).start();
            	}
            }
        });
	}
	
	void rotate( View v )
	{
		RotateAnimation r = new RotateAnimation( 0, 359, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f );
		r.setInterpolator( new LinearInterpolator() );
		r.setDuration( 1000 );
		r.setRepeatCount(Animation.INFINITE);
		r.setRepeatMode( Animation.INFINITE );
		r.setFillBefore( true );
		v.setAnimation( r );
	}
}
