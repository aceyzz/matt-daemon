#pragma once

// Test mode flag
#define TEST_MODE 0

// Config pour Tintin_reporter
#define MD_LOG_DIR "/var/log/matt-daemon/"
#define MD_LOG_DIR_MODE 0755
#define MD_LOG_FILE "matt-daemon.log"
#define MD_LOG_FILE_MODE 0644
#define MD_LOG_MAX_SIZE 1048576  // 1 MB
#define MD_LOG_BACKUP_COUNT 5

// Server
#define SRV_PORT 4242
#define SRV_MAX_CONNECTIONS 3
#define SRV_BUFFER_SIZE 1024
#define SRV_TIMEOUT_SEC 5
