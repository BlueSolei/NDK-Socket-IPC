//
// Created by sfridman on 4/8/2019.
//

#ifndef MYBOOSTAPP_ANDROIDLOG_H
#define MYBOOSTAPP_ANDROIDLOG_H

#include <android/log.h>

#ifndef ANDROID_LOG_TAG
#error "Please define ANDROID_LOG_TAG as your logging tag for this compilation unit"
#endif

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, ANDROID_LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, ANDROID_LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, ANDROID_LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, ANDROID_LOG_TAG, __VA_ARGS__))

#endif //MYBOOSTAPP_ANDROIDLOG_H
