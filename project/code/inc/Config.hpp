#pragma once

// Config pour Tintin_reporter
#define MD_LOG_DIR "/var/log/matt_daemon/"
#define MD_LOG_DIR_MODE 0755
#define MD_LOG_FILE "matt_daemon.log"
#define MD_LOG_FILE_MODE 0644
#define MD_LOG_MAX_SIZE 1048576  // 1 MB
#define MD_LOG_BACKUP_COUNT 5

// Lock file
#define MD_LOCK_FILE "/var/lock/matt_daemon.lock"
#define MD_LOCK_FILE_MODE 0644

// Server
#define SRV_PORT 4242
#define SRV_ADDR "0.0.0.0"
#define SRV_MAX_CLIENTS 3
#define SRV_SO_REUSEADDR 1
#define SRV_SO_REUSEPORT 1
#define SRV_TCP_KEEPALIVE 1
#define SRV_BUFFER_SIZE 1024
#define SRV_TIMEOUT_USEC 100000 // 100 ms
#define SRV_CMD_QUIT "quit"