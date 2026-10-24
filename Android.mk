# File: Android.mk
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ocgcore
LOCAL_MODULE_FILENAME := libocgcore
LOCAL_SRC_FILES := clone.cpp \
                clone/persistence/api.cpp \
                clone/persistence/graph.cpp \
                clone/persistence/objects.cpp \
                clone/persistence/field.cpp \
                clone/persistence/vm.cpp \
                clone/persistence/values.cpp \
                clone/persistence/image.cpp \
                clone/random.cpp \
                clone/checkpoint/api.cpp \
                clone/checkpoint/capture.cpp \
                clone/checkpoint/restore.cpp \
                clone/memory/buffer.cpp \
                clone/memory/object_pool.cpp \
                clone/lua/region.cpp \
                clone/profiling/metrics.cpp \
                clone/graph/duel.cpp \
                clone/graph/objects.cpp \
                clone/graph/references.cpp \
                clone/graph/field.cpp \
                clone/graph/vm.cpp \
                clone/lua/heap.cpp \
                clone/lua/vm.cpp \
                identity/reidentify.cpp \
                card.cpp \
				duel.cpp \
				effect.cpp \
				field.cpp \
				interpreter.cpp \
				libcard.cpp \
				libdebug.cpp \
				libduel.cpp \
				libeffect.cpp \
				libgroup.cpp \
				ocgapi.cpp \
				operations.cpp \
				playerop.cpp \
				processor.cpp \
				processor_visit.cpp \
				scriptlib.cpp

LOCAL_CFLAGS   := -pedantic -Wextra -fvisibility=hidden -DOCGCORE_EXPORT_FUNCTIONS -DNDEBUG
LOCAL_CPPFLAGS := -fexceptions -frtti
LOCAL_C_INCLUDES += $(BOOST_ROOT)
ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_LDFLAGS += "-Wl,-z,max-page-size=16384"
LOCAL_LDFLAGS += "-Wl,-z,common-page-size=16384"
endif
LOCAL_STATIC_LIBRARIES += liblua
LOCAL_LDLIBS += -ldl

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(LOCAL_PATH))
$(call import-module,lua)

