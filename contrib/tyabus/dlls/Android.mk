LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := server

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libserver_hardfp
endif

LOCAL_CFLAGS += -D_LINUX -DCLIENT_WEAPONS -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf \
	-fno-exceptions -fsigned-char -w

LOCAL_CPPFLAGS := $(LOCAL_CFLAGS)

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
		    $(LOCAL_PATH)/. \
		    $(LOCAL_PATH)/wpn_shared \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../public \
		    $(LOCAL_PATH)/../pm_shared \
		    $(LOCAL_PATH)/../game_shared

LOCAL_SRC_FILES := client.cpp \
	  ../game_shared/common.cpp \
	  crc.cpp \
	  dll_int.cpp \
	  game.cpp \
	  globals.cpp \
	  physics.cpp \
	  pr_cmds.cpp \
	  pr_edict.cpp \
	  pr_exec.cpp \
	  pr_message.cpp \
	  pr_move.cpp \
	  pr_phys.cpp \
	  pr_save.cpp \
	  pr_world.cpp \
	  saverestore.cpp \
	  ../game_shared/stringlib.cpp \
	  util.cpp \
	   ../pm_shared/pm_debug.c \
	   ../pm_shared/pm_math.c \
	   ../pm_shared/pm_shared.c

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
