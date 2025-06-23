#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

// NETCONF includes
#include <libnetconf2/netconf.h>
#include <libnetconf2/session.h>
#include <libnetconf2/messages.h>
#include <libnetconf2/log.h>
#include <libyang/libyang.h>

// Tracing structure
typedef struct {
    char traceid[33];  // 32 hex chars + null terminator
    char spanid[17];   // 16 hex chars + null terminator
} tracing_data_t;

// Configuration structure
typedef struct {
    char host[256];
    int port;
    char username[64];
    char password[64];
    char private_key_path[256];
    char public_key_path[256];
} netconf_config_t;

// Function declarations
void init_logging(void);
void cleanup_logging(void);
int generate_tracing_data(tracing_data_t *tracing);
void print_tracing_data(const tracing_data_t *tracing);
int create_netconf_session(nc_session **session, const netconf_config_t *config);
void cleanup_netconf_session(nc_session *session);
int send_tracing_data(nc_session *session, const tracing_data_t *tracing);
int receive_tracing_data(nc_session *session, tracing_data_t *tracing);
int create_self_signed_cert(const char *cert_path, const char *key_path);
int setup_ssh_keys(const char *private_key_path, const char *public_key_path);

// Default configuration
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 830
#define DEFAULT_USERNAME "admin"
#define DEFAULT_PASSWORD "admin123"
#define DEFAULT_PRIVATE_KEY "config/id_rsa"
#define DEFAULT_PUBLIC_KEY "config/id_rsa.pub"

// Error codes
#define SUCCESS 0
#define ERROR_INIT -1
#define ERROR_SESSION -2
#define ERROR_SEND -3
#define ERROR_RECEIVE -4
#define ERROR_CONFIG -5

#endif // COMMON_H 