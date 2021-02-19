/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.5-dev */

#ifndef PB_CHIP_RPC_MAIN_IPV6_ONLY_RPC_PB_H_INCLUDED
#define PB_CHIP_RPC_MAIN_IPV6_ONLY_RPC_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _chip_rpc_PingTestConfig {
    uint32_t interval_sec;
    char ipv6addr_of_counterparty[46];
} chip_rpc_PingTestConfig;

typedef struct _chip_rpc_PingTestResponse {
    char message[256];
} chip_rpc_PingTestResponse;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define chip_rpc_PingTestConfig_init_default     {0, ""}
#define chip_rpc_PingTestResponse_init_default   {""}
#define chip_rpc_PingTestConfig_init_zero        {0, ""}
#define chip_rpc_PingTestResponse_init_zero      {""}

/* Field tags (for use in manual encoding/decoding) */
#define chip_rpc_PingTestConfig_interval_sec_tag 1
#define chip_rpc_PingTestConfig_ipv6addr_of_counterparty_tag 2
#define chip_rpc_PingTestResponse_message_tag    1

/* Struct field encoding specification for nanopb */
#define chip_rpc_PingTestConfig_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UINT32,   interval_sec,      1) \
X(a, STATIC,   SINGULAR, STRING,   ipv6addr_of_counterparty,   2)
#define chip_rpc_PingTestConfig_CALLBACK NULL
#define chip_rpc_PingTestConfig_DEFAULT NULL

#define chip_rpc_PingTestResponse_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, STRING,   message,           1)
#define chip_rpc_PingTestResponse_CALLBACK NULL
#define chip_rpc_PingTestResponse_DEFAULT NULL

extern const pb_msgdesc_t chip_rpc_PingTestConfig_msg;
extern const pb_msgdesc_t chip_rpc_PingTestResponse_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define chip_rpc_PingTestConfig_fields &chip_rpc_PingTestConfig_msg
#define chip_rpc_PingTestResponse_fields &chip_rpc_PingTestResponse_msg

/* Maximum encoded size of messages (where known) */
#define chip_rpc_PingTestConfig_size             53
#define chip_rpc_PingTestResponse_size           258

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif