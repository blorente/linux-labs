LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= chat.c ## Lista de ficheros .c del proyecto
LOCAL_SHARED_LIBRARIES :=
LOCAL_CFLAGS :=
LOCAL_MODULE := chat
## Nombre del ejecutable
include $(BUILD_EXECUTABLE) ## Incluye reglas para compilar ejecutable
