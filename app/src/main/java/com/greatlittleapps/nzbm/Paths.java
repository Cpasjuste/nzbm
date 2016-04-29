package com.greatlittleapps.nzbm;

import android.content.Context;
import android.os.Environment;

public class Paths
{
	public String nzbget;
	public String data;
	public String download;
	public String webui;
	public String config;
	public String unrar;
	
	public Paths(Context ctx)
	{
		nzbget = data = ctx.getFilesDir().getAbsolutePath();
		unrar = ctx.getApplicationInfo().nativeLibraryDir + "/libunrar.so";
		download = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Download";
		webui = nzbget + "/webui";
		config = nzbget + "/nzbget.conf";
	}
}
