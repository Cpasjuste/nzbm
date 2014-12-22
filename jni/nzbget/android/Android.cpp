#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <fstream>
#include <stdarg.h>
#include <unistd.h>

#include "nzbget.h"
#include "Log.h"
#include "Options.h"
#include "QueueCoordinator.h"
#include "UrlCoordinator.h"
#include "QueueEditor.h"
#include "PrePostProcessor.h"
#include "Scanner.h"
#include "Util.h"

const char* svn_version(void)
{
	const char* SVN_Version = "508";
	return SVN_Version;
}

extern int main( int argc, char *argv[], char *argp[] );
extern void ExitProc();
extern bool g_bReloading;
extern Options* g_pOptions;
extern Scanner* g_pScanner;

extern "C"
{
    JNIEXPORT int JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_main( JNIEnv *env, jobject thiz, jobjectArray strArray );
    JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_pause( JNIEnv *env, jobject thiz );
    JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_resume( JNIEnv *env, jobject thiz );
    JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_shutdown( JNIEnv *env, jobject thiz );
    JNIEXPORT jboolean JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_append( JNIEnv *env, jobject thiz, jstring path, jstring category, bool addToTop );
    JNIEXPORT jstring JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_version( JNIEnv *env, jobject thiz );
};

JNIEXPORT int JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_main( JNIEnv *env, jobject thiz, jobjectArray strArray )
{
	int i;

	jsize len = env->GetArrayLength(strArray);

	const char *argv[len];
	for( i=0; i<len; i++ )
	{
		jstring str = (jstring)env->GetObjectArrayElement(strArray,i);
		argv[i] = env->GetStringUTFChars( str, 0 );
	}

	int ret = main( i, (char **)argv, (char **)"" );

	g_bReloading = true;

	for( i=0; i<len; i++ )
		env->ReleaseStringUTFChars( (jstring)env->GetObjectArrayElement(strArray,i), argv[i] );

	return ret;
}

JNIEXPORT jstring JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_version( JNIEnv *env, jobject thiz )
{
	return env->NewStringUTF( Util::VersionRevision() );
}

JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_pause( JNIEnv *env, jobject thiz )
{
	if( g_pOptions )
		g_pOptions->SetPauseDownload( true );
}

JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_resume( JNIEnv *env, jobject thiz )
{
	if( g_pOptions )
		g_pOptions->SetPauseDownload( false );
}

JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_shutdown( JNIEnv *env, jobject thiz )
{
	ExitProc();
}

JNIEXPORT jboolean JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_append( JNIEnv *env, jobject thiz, jstring path, jstring category, bool bAddTop )
{
	bool success = false;
	const char *szNZBFilename = env->GetStringUTFChars( path, 0 );
	const char *szCategory = env->GetStringUTFChars( category, 0 );

	debug("FileName=%s", szNZBFilename);
	
	if (!strncasecmp(szNZBFilename, "http://", 6) || !strncasecmp(szNZBFilename, "https://", 7))
	{
		// add url
		NZBInfo* pNZBInfo = new NZBInfo();
		pNZBInfo->SetKind(NZBInfo::nkUrl);
		pNZBInfo->SetURL(szNZBFilename);
		pNZBInfo->SetFilename(szNZBFilename);
		pNZBInfo->SetCategory(szCategory);
		pNZBInfo->SetPriority(0);
		pNZBInfo->SetAddUrlPaused(false);
		pNZBInfo->SetDupeKey("");
		pNZBInfo->SetDupeScore(0);
		pNZBInfo->SetDupeMode(dmScore);
		int iNZBID = pNZBInfo->GetID();

		char szNicename[1024];
		pNZBInfo->MakeNiceUrlName(szNZBFilename, szNZBFilename, szNicename, sizeof(szNicename));
		info("Queue %s", szNicename);

		DownloadQueue* pDownloadQueue = DownloadQueue::Lock();
		pDownloadQueue->GetQueue()->Add(pNZBInfo, bAddTop);
		pDownloadQueue->Save();
		DownloadQueue::Unlock();

		success = true;
	}
	else
	{
		g_pScanner->AddExternalFile(szNZBFilename, szCategory, 0, "", 0, dmScore, NULL, bAddTop, false, NULL, NULL, NULL, 0, NULL);
		return true;
	}
	return success;
}


