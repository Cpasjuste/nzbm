package com.greatlittleapps.nzbm;

import android.os.Environment;

public class Paths
{
	public static final String nzbget =
			Environment.getExternalStorageDirectory().getAbsolutePath() + "/nzbget";
	
	public static final String data =
			Environment.getExternalStorageDirectory().getAbsolutePath() + "/nzbget";
	
	public static final String download =
			Environment.getExternalStorageDirectory().getAbsolutePath() + "/Download";
	
	public static final String webui =
			Environment.getExternalStorageDirectory().getAbsolutePath() + "/nzbget/webui";
	
	public static final String config = 
			Environment.getExternalStorageDirectory().getAbsolutePath() + "/nzbget/webui/nzbget.conf";
	
	public static String unrar = "";
}
