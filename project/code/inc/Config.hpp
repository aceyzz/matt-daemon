#pragma once

// Test mode flag
#define TEST_MODE 1

// Config pour Tintin_reporter
#define MD_LOG_DIR "/var/log/matt-daemon/"
#define MD_LOG_DIR_MODE 0755
#define MD_LOG_FILE "matt-daemon.log"
#define MD_LOG_FILE_MODE 0644
#define MD_LOG_MAX_SIZE 1048576  // 1 MB
#define MD_LOG_BACKUP_COUNT 5