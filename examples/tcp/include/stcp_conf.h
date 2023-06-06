#pragma once

#define NAIVE_CHECKSUM
//#define INT32_CHECKSUM

//#define STCP_DEBUG

//#define ONLY_CHECKSUM

#define CONNECTION_IN_L1

//Size of the TCP shadow buffer (per-connection, in bytes)
#define STCP_SHADOW_BUFFER_SIZE (1024*1024*1024)

#define STCP_MAX_CONNECTIONS 256
#define STCP_HASH_TABLE_SIZE 128

#define NUM_OOO_SEGMENTS 256