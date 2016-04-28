LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE := nzbget

SRC_CONNECT := \
	daemon/connect/Connection.cpp \
	daemon/connect/TlsSocket.cpp \
	daemon/connect/WebDownloader.cpp

SRC_EXTENSION := \
	daemon/extension/ScanScript.cpp \
	daemon/extension/FeedScript.cpp \
	daemon/extension/QueueScript.cpp \
	daemon/extension/ScriptConfig.cpp \
	daemon/extension/NzbScript.cpp \
	daemon/extension/PostScript.cpp \
	daemon/extension/SchedulerScript.cpp
	
SRC_FEED := \
	daemon/feed/FeedCoordinator.cpp \
	daemon/feed/FeedFile.cpp \
	daemon/feed/FeedFilter.cpp \
	daemon/feed/FeedInfo.cpp

SRC_FRONTEND := \
	daemon/frontend/ColoredFrontend.cpp \
	daemon/frontend/LoggableFrontend.cpp \
	daemon/frontend/Frontend.cpp

SRC_MAIN := \
	daemon/main/Maintenance.cpp \
	daemon/main/nzbget.cpp \
	daemon/main/Options.cpp \
	daemon/main/Scheduler.cpp \
	daemon/main/StackTrace.cpp \
	daemon/main/CommandLineParser.cpp \
	daemon/main/DiskService.cpp

SRC_NNTP := \
	daemon/nntp/ArticleDownloader.cpp \
	daemon/nntp/Decoder.cpp \
	daemon/nntp/NntpConnection.cpp \
	daemon/nntp/StatMeter.cpp \
	daemon/nntp/ArticleWriter.cpp \
	daemon/nntp/NewsServer.cpp \
	daemon/nntp/ServerPool.cpp

SRC_POSTPROCESS := \
	daemon/postprocess/ParRenamer.cpp \
	daemon/postprocess/DupeMatcher.cpp \
	daemon/postprocess/PrePostProcessor.cpp \
	daemon/postprocess/Unpack.cpp \
	daemon/postprocess/ParCoordinator.cpp \
	daemon/postprocess/ParChecker.cpp \
	daemon/postprocess/ParParser.cpp \
	daemon/postprocess/Cleanup.cpp

SRC_QUEUE := \
	daemon/queue/QueueCoordinator.cpp \
	daemon/queue/UrlCoordinator.cpp \
	daemon/queue/DiskState.cpp \
	daemon/queue/HistoryCoordinator.cpp \
	daemon/queue/NzbFile.cpp \
	daemon/queue/QueueEditor.cpp \
	daemon/queue/Scanner.cpp \
	daemon/queue/DownloadInfo.cpp \
	daemon/queue/DupeCoordinator.cpp

SRC_REMOTE := \
	daemon/remote/BinRpc.cpp \
	daemon/remote/RemoteServer.cpp \
	daemon/remote/XmlRpc.cpp \
	daemon/remote/RemoteClient.cpp \
	daemon/remote/WebServer.cpp

SRC_UTIL := \
	daemon/util/Log.cpp \
	daemon/util/Observer.cpp \
	daemon/util/Script.cpp \
	daemon/util/Thread.cpp \
	daemon/util/Util.cpp \
	daemon/util/FileSystem.cpp \
	daemon/util/NString.cpp \
	daemon/util/Service.cpp

SRC_PAR2 := \
	lib/par2/commandline.cpp \
	lib/par2/descriptionpacket.cpp \
	lib/par2/md5.cpp \
	lib/par2/parheaders.cpp \
	lib/par2/crc.cpp \
	lib/par2/diskfile.cpp \
	lib/par2/par2creatorsourcefile.cpp \
	lib/par2/recoverypacket.cpp \
	lib/par2/creatorpacket.cpp \
	lib/par2/filechecksummer.cpp \
	lib/par2/par2fileformat.cpp \
	lib/par2/reedsolomon.cpp \
	lib/par2/criticalpacket.cpp \
	lib/par2/galois.cpp \
	lib/par2/par2repairer.cpp \
	lib/par2/verificationhashtable.cpp \
	lib/par2/datablock.cpp \
	lib/par2/mainpacket.cpp \
	lib/par2/par2repairersourcefile.cpp \
	lib/par2/verificationpacket.cpp

LOCAL_SRC_FILES := android/Android.cpp \
		$(SRC_CONNECT) $(SRC_EXTENSION) $(SRC_FEED) $(SRC_FRONTEND) \
		$(SRC_MAIN) $(SRC_NNTP) $(SRC_POSTPROCESS) $(SRC_QUEUE) \
		$(SRC_REMOTE) $(SRC_UTIL) $(SRC_PAR2)
		 
LOCAL_LDLIBS := -llog -lz

LOCAL_STATIC_LIBRARIES := libssl_static libcrypto_static xml2

LOCAL_CFLAGS += -DHAVE_PTHREADS

LOCAL_CFLAGS += -O3 -DHAVE_CONFIG_H -DANDROID \
		-I$(LOCAL_PATH)/android \
		-I$(LOCAL_PATH)/daemon/connect \
		-I$(LOCAL_PATH)/daemon/extension \
		-I$(LOCAL_PATH)/daemon/feed \
		-I$(LOCAL_PATH)/daemon/frontend \
		-I$(LOCAL_PATH)/daemon/main \
		-I$(LOCAL_PATH)/daemon/nntp \
		-I$(LOCAL_PATH)/daemon/postprocess \
		-I$(LOCAL_PATH)/daemon/queue \
		-I$(LOCAL_PATH)/daemon/remote \
		-I$(LOCAL_PATH)/daemon/util \
		-I$(LOCAL_PATH)/lib/par2 \
		-I$(LOCAL_PATH)/../ \
		-I$(LOCAL_PATH)/../openssl/include \
		-I$(LOCAL_PATH)/../libxml2/include

include $(BUILD_SHARED_LIBRARY)
