#pragma once
/**
 * @file fs.h
 * @brief Filesystem Services
 */
#ifndef FS_H
#define FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds/services/fs.h>

/// The data of a user savedata archive's lowpath.
typedef struct
{
	u32 mediatype; 	///< The mediatype of the FS_Path.
	u32 lowid; 		///< The lower word of the saveid.
	u32 highid; 	///< The upper word of the saveid.
} FS_UserSaveData_LowPathData;

/// Attribute flags extended.
enum
{
	FS_ATTRIBUTE_NONE = 0,  ///< None.
};

extern FS_Archive saveArchive;

Result FS_ReadFile(const char* path, void* dst, u64 maxSize, FS_Archive archive, u32* bytesRead);
Result FS_WriteFile(const char* path, const void* src, u64 size, FS_Archive archive, u32* bytesWritten);
Result FS_DeleteFile(const char* path, FS_Archive archive);
Result FS_CommitArchive(FS_Archive archive);

/**
 * @brief Initializes the filesystem service for a selected title.
 */
Result FSCIA_Init(u64 titleid, FS_MediaType mediatype);

/**
 * @brief Initializes the filesystem service for a selected title without SDMC fallback.
 */
Result FSCIA_InitSaveOnly(u64 titleid, FS_MediaType mediatype);

/**
 * @brief Exits the filesystem service for a selected title.
 */
Result FSCIA_Exit(void);

/**
 * @brief Initializes the filesystem service for the current title (*hax).
 */
Result FS_Init(void);

/**
 * @brief Exits the filesystem service.
 */
Result FS_Exit(void);

#ifdef __cplusplus
}
#endif

#endif // FS_H
