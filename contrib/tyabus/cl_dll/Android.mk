LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := client
#ifeq ($(XASH_SDL),1)
#APP_PLATFORM := android-12
#LOCAL_SHARED_LIBRARIES += SDL2
#LOCAL_CFLAGS += -DXASH_SDL
#else
APP_PLATFORM := android-8
#endif

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libclient_hardfp
endif

INCLUDES =  -I../common -I. -I../game_shared -I../pm_shared -I../engine -I../dlls -I../utils/false_vgui/include
DEFINES = -fsigned-char -Wno-write-strings -DLINUX -D_LINUX -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DCLIENT_WEAPONS -DCLIENT_DLL -w -D_snprintf=snprintf

LOCAL_C_INCLUDES := $(LOCAL_PATH)/. \
		 $(LOCAL_PATH)/../common \
		 $(LOCAL_PATH)/../engine \
		 $(LOCAL_PATH)/../game_shared \
		 $(LOCAL_PATH)/../dlls \
		 $(LOCAL_PATH)/../pm_shared

LOCAL_CFLAGS += $(DEFINES) $(INCLUDES)

LOCAL_SRC_FILES := cdll_int.cpp \
	  ../game_shared/common.cpp \
	  entity.cpp \
	  hud.cpp \
	  hud_draw.cpp \
	  hud_msg.cpp \
	  hud_redraw.cpp \
	  hud_sbar.cpp \
	  hud_update.cpp \
	  in_camera.cpp \
	  input.cpp \
	  input_xash3d.cpp \
	  input_mouse.cpp \
	  message.cpp \
	  parsemsg.cpp \
	  ../pm_shared/pm_debug.c \
	  ../pm_shared/pm_math.c \
	  ../pm_shared/pm_shared.c \
	  saytext.cpp \
	  scoreboard.cpp \
	  ../game_shared/stringlib.cpp \
	  StudioModelRenderer.cpp \
	  text_message.cpp \
	  util.cpp \
	  view.cpp \

include $(BUILD_SHARED_LIBRARY)
