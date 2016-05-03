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

const char* code_revision(void)
{
	return PACKAGE_STRING;
}

int lockf(int fd, int cmd, off_t len) {};
int getdtablesize(void) {};

extern int main( int argc, char *argv[], char *argp[] );
extern void ExitProc();

extern "C"
{
    JNIEXPORT int JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_main( JNIEnv *env, jobject thiz, jobjectArray strArray );
    JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_pause( JNIEnv *env, jobject thiz );
    JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_resume( JNIEnv *env, jobject thiz );
    JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_shutdown( JNIEnv *env, jobject thiz );
    JNIEXPORT jboolean JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_append( JNIEnv *env, jobject thiz, jstring path, jstring category, bool addToTop );
    JNIEXPORT jstring JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_version( JNIEnv *env, jobject thiz );
};

// fix Script.cpp: EnvironmentStrings::InitFromCurrentProcess
const char *fake_argp[1] = { "none" };

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

	// run nzbget
	int ret = main( i, (char **)argv, (char **)fake_argp );

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
	if( g_Options )
		g_Options->SetPauseDownload( true );
}

JNIEXPORT void JNICALL Java_com_greatlittleapps_nzbm_service_Nzbget_resume( JNIEnv *env, jobject thiz )
{
	if( g_Options )
		g_Options->SetPauseDownload( false );
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
	
	if (!strncasecmp(szNZBFilename, "http://", 6)
			|| !strncasecmp(szNZBFilename, "https://", 7))
	{
		// add url
		std::unique_ptr<NzbInfo> nzbInfo = std::make_unique<NzbInfo>();
		nzbInfo->SetKind(NzbInfo::nkUrl);
		nzbInfo->SetUrl(szNZBFilename);
		nzbInfo->SetFilename(szNZBFilename);
		nzbInfo->SetCategory(szCategory);
		nzbInfo->SetPriority(0);
		nzbInfo->SetAddUrlPaused(false);
		nzbInfo->SetDupeKey("");
		nzbInfo->SetDupeScore(0);
		nzbInfo->SetDupeMode(dmScore);
		//nzbInfo->GetParameters()->CopyFrom(&Params);
		int nzbId = nzbInfo->GetId();

		info("Queue %s", *nzbInfo->MakeNiceUrlName(szNZBFilename, szNZBFilename));

		GuardedDownloadQueue downloadQueue = DownloadQueue::Guard();
		downloadQueue->GetQueue()->Add(std::move(nzbInfo), false);
		downloadQueue->Save();

		success = true;
	}
	else
	{
		g_Scanner->AddExternalFile(szNZBFilename, szCategory, 0, "", 0, dmScore, nullptr, bAddTop, false,
			nullptr, szNZBFilename, NULL, 0, NULL);
		return true;
	}
	return success;
}


