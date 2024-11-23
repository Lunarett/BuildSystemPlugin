#pragma once

#if UE_EDITOR && UE_BUILD_DEVELOPMENT

#define LOG(Msg) UE_LOG(LogTemp, Display, TEXT("Build System Plugin - %s - Log: %s"), *FString(__FUNCTION__), *FString(Msg))
#define LOG_WARN(Msg) UE_LOG(LogTemp, Warning, TEXT("Build System Plugin - %s - Warning: %s"), *FString(__FUNCTION__), *FString(Msg))
#define LOG_ERROR(Msg) UE_LOG(LogTemp, Error, TEXT("Build System Plugin - %s - Error: %s"), *FString(__FUNCTION__), *FString(Msg))

#else

#define LOG(Msg)
#define LOG_WARN(Msg)
#define LOG_ERROR(Msg)

#endif
