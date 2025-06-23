#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// NETCONF includes
#include <libnetconf2/netconf.h>
#include <libnetconf2/session.h>
#include <libnetconf2/messages.h>
#include <libnetconf2/log.h>
#include <libyang/libyang.h>

// O1 interface structures
typedef struct {
    char traceid[33];      // 32 hex chars + null terminator
    char spanid[17];       // 16 hex chars + null terminator
    char interface_name[64];
    char operation[32];    // "get", "set", "delete"
    char status[32];       // "up", "down", "error"
} o1_interface_data_t;

typedef struct {
    char host[256];
    int port;
    char username[64];
    char password[64];
    char private_key_path[256];
} o1_config_t;

// Global variables
static volatile int running = 1;
static struct nc_session *session = NULL;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
    if (session) {
        nc_session_free(session, NULL);
    }
}

void init_netconf() {
    // Initialize libnetconf2
    int ret = nc_init();
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize libnetconf2\n");
        exit(1);
    }
    
    // Set logging level
    nc_verbosity(NC_VERB_VERBOSE);
    
    printf("NETCONF initialized for O1 interface\n");
}

void cleanup_netconf() {
    if (session) {
        nc_session_free(session, NULL);
    }
    printf("NETCONF cleaned up\n");
}

int generate_tracing_data(o1_interface_data_t *o1_data) {
    if (!o1_data) {
        return -1;
    }
    
    // Generate random traceid (32 hex characters)
    unsigned char traceid_bytes[16];
    if (RAND_bytes(traceid_bytes, 16) != 1) {
        fprintf(stderr, "Failed to generate random traceid\n");
        return -1;
    }
    
    for (int i = 0; i < 16; i++) {
        sprintf(&o1_data->traceid[i * 2], "%02x", traceid_bytes[i]);
    }
    o1_data->traceid[32] = '\0';
    
    // Generate random spanid (16 hex characters)
    unsigned char spanid_bytes[8];
    if (RAND_bytes(spanid_bytes, 8) != 1) {
        fprintf(stderr, "Failed to generate random spanid\n");
        return -1;
    }
    
    for (int i = 0; i < 8; i++) {
        sprintf(&o1_data->spanid[i * 2], "%02x", spanid_bytes[i]);
    }
    o1_data->spanid[16] = '\0';
    
    // Set default O1 interface data
    strcpy(o1_data->interface_name, "eth0");
    strcpy(o1_data->operation, "get");
    strcpy(o1_data->status, "up");
    
    return 0;
}

void print_o1_data(const o1_interface_data_t *o1_data) {
    if (!o1_data) {
        printf("O1 data: NULL\n");
        return;
    }
    
    printf("O1 Interface Data:\n");
    printf("  TraceID:       %s\n", o1_data->traceid);
    printf("  SpanID:        %s\n", o1_data->spanid);
    printf("  Interface:     %s\n", o1_data->interface_name);
    printf("  Operation:     %s\n", o1_data->operation);
    printf("  Status:        %s\n", o1_data->status);
}

int create_netconf_session(const o1_config_t *config) {
    if (!config) {
        return -1;
    }
    
    int ret;
    
    // Create SSH session
    ret = nc_connect_ssh(config->host, config->port, config->username, 
                        config->private_key_path, config->password, &session);
    if (ret != NC_MSG_HELLO) {
        fprintf(stderr, "Failed to connect to NETCONF server: %s\n", nc_strerror(ret));
        return -1;
    }
    
    printf("NETCONF session established successfully\n");
    return 0;
}

int send_o1_get_config(const o1_interface_data_t *o1_data) {
    if (!session || !o1_data) {
        return -1;
    }
    
    // Create NETCONF get-config message
    char xml_msg[2048];
    snprintf(xml_msg, sizeof(xml_msg),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" message-id=\"1\">\n"
        "  <get-config>\n"
        "    <source>\n"
        "      <running/>\n"
        "    </source>\n"
        "    <filter type=\"subtree\">\n"
        "      <o1-interface xmlns=\"urn:example:o1-interface\">\n"
        "        <name>%s</name>\n"
        "      </o1-interface>\n"
        "    </filter>\n"
        "  </get-config>\n"
        "</rpc>\n",
        o1_data->interface_name);
    
    // Send the message
    int ret = nc_send_rpc(session, xml_msg, 1000, NULL);
    if (ret != NC_MSG_RPC) {
        fprintf(stderr, "Failed to send get-config: %s\n", nc_strerror(ret));
        return -1;
    }
    
    printf("O1 get-config sent for interface %s\n", o1_data->interface_name);
    return 0;
}

int send_o1_edit_config(const o1_interface_data_t *o1_data) {
    if (!session || !o1_data) {
        return -1;
    }
    
    // Create NETCONF edit-config message
    char xml_msg[2048];
    snprintf(xml_msg, sizeof(xml_msg),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" message-id=\"2\">\n"
        "  <edit-config>\n"
        "    <target>\n"
        "      <running/>\n"
        "    </target>\n"
        "    <config>\n"
        "      <o1-interface xmlns=\"urn:example:o1-interface\">\n"
        "        <name>%s</name>\n"
        "        <status>%s</status>\n"
        "        <tracing>\n"
        "          <traceid>%s</traceid>\n"
        "          <spanid>%s</spanid>\n"
        "        </tracing>\n"
        "      </o1-interface>\n"
        "    </config>\n"
        "  </edit-config>\n"
        "</rpc>\n",
        o1_data->interface_name, o1_data->status, 
        o1_data->traceid, o1_data->spanid);
    
    // Send the message
    int ret = nc_send_rpc(session, xml_msg, 1000, NULL);
    if (ret != NC_MSG_RPC) {
        fprintf(stderr, "Failed to send edit-config: %s\n", nc_strerror(ret));
        return -1;
    }
    
    printf("O1 edit-config sent for interface %s\n", o1_data->interface_name);
    return 0;
}

int receive_netconf_response() {
    if (!session) {
        return -1;
    }
    
    // Receive response
    struct nc_msg *msg = NULL;
    int ret = nc_recv_reply(session, NULL, 1000, &msg);
    if (ret != NC_MSG_REPLY) {
        fprintf(stderr, "Failed to receive response: %s\n", nc_strerror(ret));
        return -1;
    }
    
    printf("Received NETCONF response\n");
    
    // In a real implementation, you would parse the XML response
    // to extract the O1 interface data
    
    nc_msg_free(msg);
    return 0;
}

int main(int argc, char *argv[]) {
    o1_config_t config;
    o1_interface_data_t o1_data;
    
    // Set default configuration
    strcpy(config.host, "127.0.0.1");
    config.port = 830;
    strcpy(config.username, "admin");
    strcpy(config.password, "admin123");
    strcpy(config.private_key_path, "config/id_rsa");
    
    // Parse command line arguments
    if (argc > 1) {
        strcpy(config.host, argv[1]);
    }
    if (argc > 2) {
        config.port = atoi(argv[2]);
    }
    if (argc > 3) {
        strcpy(config.username, argv[3]);
    }
    if (argc > 4) {
        strcpy(config.password, argv[4]);
    }
    
    printf("O1 Interface NETCONF Client\n");
    printf("Connecting to %s:%d as %s\n", config.host, config.port, config.username);
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize NETCONF
    init_netconf();
    
    // Generate O1 interface data
    if (generate_tracing_data(&o1_data) != 0) {
        fprintf(stderr, "Failed to generate O1 data\n");
        cleanup_netconf();
        return 1;
    }
    
    printf("Generated O1 interface data:\n");
    print_o1_data(&o1_data);
    
    // Create NETCONF session
    if (create_netconf_session(&config) != 0) {
        fprintf(stderr, "Failed to create NETCONF session\n");
        cleanup_netconf();
        return 1;
    }
    
    // Send O1 get-config operation
    if (send_o1_get_config(&o1_data) != 0) {
        fprintf(stderr, "Failed to send get-config\n");
        cleanup_netconf();
        return 1;
    }
    
    // Receive response
    if (receive_netconf_response() != 0) {
        fprintf(stderr, "Failed to receive get-config response\n");
        cleanup_netconf();
        return 1;
    }
    
    // Send O1 edit-config operation
    if (send_o1_edit_config(&o1_data) != 0) {
        fprintf(stderr, "Failed to send edit-config\n");
        cleanup_netconf();
        return 1;
    }
    
    // Receive response
    if (receive_netconf_response() != 0) {
        fprintf(stderr, "Failed to receive edit-config response\n");
        cleanup_netconf();
        return 1;
    }
    
    printf("Successfully completed O1 interface operations via NETCONF\n");
    
    // Cleanup
    cleanup_netconf();
    
    printf("O1 NETCONF client completed successfully\n");
    return 0;
} 