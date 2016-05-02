LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE := nzbget

SRC_PATH := ../nzbget/

SRC_CONNECT := \
	$(SRC_PATH)daemon/connect/Connection.cpp \
	$(SRC_PATH)daemon/connect/TlsSocket.cpp \
	$(SRC_PATH)daemon/connect/WebDownloader.cpp

SRC_EXTENSION := \
	$(SRC_PATH)daemon/extension/ScanScript.cpp \
	$(SRC_PATH)daemon/extension/FeedScript.cpp \
	$(SRC_PATH)daemon/extension/QueueScript.cpp \
	$(SRC_PATH)daemon/extension/ScriptConfig.cpp \
	$(SRC_PATH)daemon/extension/NzbScript.cpp \
	$(SRC_PATH)daemon/extension/PostScript.cpp \
	$(SRC_PATH)daemon/extension/SchedulerScript.cpp
	
SRC_FEED := \
	$(SRC_PATH)daemon/feed/FeedCoordinator.cpp \
	$(SRC_PATH)daemon/feed/FeedFile.cpp \
	$(SRC_PATH)daemon/feed/FeedFilter.cpp \
	$(SRC_PATH)daemon/feed/FeedInfo.cpp

SRC_FRONTEND := \
	$(SRC_PATH)daemon/frontend/ColoredFrontend.cpp \
	$(SRC_PATH)daemon/frontend/LoggableFrontend.cpp \
	$(SRC_PATH)daemon/frontend/Frontend.cpp

SRC_MAIN := \
	$(SRC_PATH)daemon/main/Maintenance.cpp \
	$(SRC_PATH)daemon/main/nzbget.cpp \
	$(SRC_PATH)daemon/main/Options.cpp \
	$(SRC_PATH)daemon/main/Scheduler.cpp \
	$(SRC_PATH)daemon/main/StackTrace.cpp \
	$(SRC_PATH)daemon/main/CommandLineParser.cpp \
	$(SRC_PATH)daemon/main/DiskService.cpp

SRC_NNTP := \
	$(SRC_PATH)daemon/nntp/ArticleDownloader.cpp \
	$(SRC_PATH)daemon/nntp/Decoder.cpp \
	$(SRC_PATH)daemon/nntp/NntpConnection.cpp \
	$(SRC_PATH)daemon/nntp/StatMeter.cpp \
	$(SRC_PATH)daemon/nntp/ArticleWriter.cpp \
	$(SRC_PATH)daemon/nntp/NewsServer.cpp \
	$(SRC_PATH)daemon/nntp/ServerPool.cpp

SRC_POSTPROCESS := \
	$(SRC_PATH)daemon/postprocess/ParRenamer.cpp \
	$(SRC_PATH)daemon/postprocess/DupeMatcher.cpp \
	$(SRC_PATH)daemon/postprocess/PrePostProcessor.cpp \
	$(SRC_PATH)daemon/postprocess/Unpack.cpp \
	$(SRC_PATH)daemon/postprocess/ParCoordinator.cpp \
	$(SRC_PATH)daemon/postprocess/ParChecker.cpp \
	$(SRC_PATH)daemon/postprocess/ParParser.cpp \
	$(SRC_PATH)daemon/postprocess/Cleanup.cpp

SRC_QUEUE := \
	$(SRC_PATH)daemon/queue/QueueCoordinator.cpp \
	$(SRC_PATH)daemon/queue/UrlCoordinator.cpp \
	$(SRC_PATH)daemon/queue/DiskState.cpp \
	$(SRC_PATH)daemon/queue/HistoryCoordinator.cpp \
	$(SRC_PATH)daemon/queue/NzbFile.cpp \
	$(SRC_PATH)daemon/queue/QueueEditor.cpp \
	$(SRC_PATH)daemon/queue/Scanner.cpp \
	$(SRC_PATH)daemon/queue/DownloadInfo.cpp \
	$(SRC_PATH)daemon/queue/DupeCoordinator.cpp

SRC_REMOTE := \
	$(SRC_PATH)daemon/remote/BinRpc.cpp \
	$(SRC_PATH)daemon/remote/RemoteServer.cpp \
	$(SRC_PATH)daemon/remote/XmlRpc.cpp \
	$(SRC_PATH)daemon/remote/RemoteClient.cpp \
	$(SRC_PATH)daemon/remote/WebServer.cpp

SRC_UTIL := \
	$(SRC_PATH)daemon/util/Log.cpp \
	$(SRC_PATH)daemon/util/Observer.cpp \
	$(SRC_PATH)daemon/util/Script.cpp \
	$(SRC_PATH)daemon/util/Thread.cpp \
	$(SRC_PATH)daemon/util/Util.cpp \
	$(SRC_PATH)daemon/util/FileSystem.cpp \
	$(SRC_PATH)daemon/util/NString.cpp \
	$(SRC_PATH)daemon/util/Service.cpp

SRC_PAR2 := \
	$(SRC_PATH)lib/par2/commandline.cpp \
	$(SRC_PATH)lib/par2/descriptionpacket.cpp \
	$(SRC_PATH)lib/par2/md5.cpp \
	$(SRC_PATH)lib/par2/parheaders.cpp \
	$(SRC_PATH)lib/par2/crc.cpp \
	$(SRC_PATH)lib/par2/diskfile.cpp \
	$(SRC_PATH)lib/par2/par2creatorsourcefile.cpp \
	$(SRC_PATH)lib/par2/recoverypacket.cpp \
	$(SRC_PATH)lib/par2/creatorpacket.cpp \
	$(SRC_PATH)lib/par2/filechecksummer.cpp \
	$(SRC_PATH)lib/par2/par2fileformat.cpp \
	$(SRC_PATH)lib/par2/reedsolomon.cpp \
	$(SRC_PATH)lib/par2/criticalpacket.cpp \
	$(SRC_PATH)lib/par2/galois.cpp \
	$(SRC_PATH)lib/par2/par2repairer.cpp \
	$(SRC_PATH)lib/par2/verificationhashtable.cpp \
	$(SRC_PATH)lib/par2/datablock.cpp \
	$(SRC_PATH)lib/par2/mainpacket.cpp \
	$(SRC_PATH)lib/par2/par2repairersourcefile.cpp \
	$(SRC_PATH)lib/par2/verificationpacket.cpp

LOCAL_SRC_FILES := Android.cpp \
		$(SRC_CONNECT) $(SRC_EXTENSION) $(SRC_FEED) $(SRC_FRONTEND) \
		$(SRC_MAIN) $(SRC_NNTP) $(SRC_POSTPROCESS) $(SRC_QUEUE) \
		$(SRC_REMOTE) $(SRC_UTIL) $(SRC_PAR2)
		 
LOCAL_LDLIBS := -llog -lz

LOCAL_STATIC_LIBRARIES := libopenssl_static libopencrypto_static libxml2

LOCAL_CFLAGS += -DHAVE_PTHREADS

LOCAL_CFLAGS += -O3 -DHAVE_CONFIG_H -DANDROID \
		-I$(LOCAL_PATH) \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/connect \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/extension \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/feed \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/frontend \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/main \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/nntp \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/postprocess \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/queue \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/remote \
		-I$(LOCAL_PATH)/$(SRC_PATH)daemon/util \
		-I$(LOCAL_PATH)/$(SRC_PATH)lib/par2 \
		-I$(LOCAL_PATH)/$(SRC_PATH)../ \
		-I$(LOCAL_PATH)/$(SRC_PATH)../openssl/openssl-1.0.2/include \
		-I$(LOCAL_PATH)/$(SRC_PATH)../libxml2/include

include $(BUILD_SHARED_LIBRARY)
