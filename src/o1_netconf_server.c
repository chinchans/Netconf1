#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

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

static volatile int running = 1;
static int server_socket = -1;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
    if (server_socket != -1) {
        close(server_socket);
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
    
    printf("NETCONF initialized for O1 interface server\n");
}

void cleanup_netconf() {
    printf("NETCONF cleaned up\n");
}

int setup_server_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to create socket");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sock);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return -1;
    }
    
    if (listen(sock, 5) < 0) {
        perror("Listen failed");
        close(sock);
        return -1;
    }
    
    return sock;
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

int parse_o1_get_config(const char *xml_data, o1_interface_data_t *o1_data) {
    if (!xml_data || !o1_data) {
        return -1;
    }
    
    // Simple XML parsing for get-config
    char *interface_start = strstr(xml_data, "<name>");
    if (interface_start) {
        interface_start += 6; // Skip "<name>"
        char *interface_end = strstr(interface_start, "</name>");
        if (interface_end) {
            int len = interface_end - interface_start;
            if (len < sizeof(o1_data->interface_name)) {
                strncpy(o1_data->interface_name, interface_start, len);
                o1_data->interface_name[len] = '\0';
                strcpy(o1_data->operation, "get");
                return 0;
            }
        }
    }
    
    return -1;
}

int parse_o1_edit_config(const char *xml_data, o1_interface_data_t *o1_data) {
    if (!xml_data || !o1_data) {
        return -1;
    }
    
    // Simple XML parsing for edit-config
    char *interface_start = strstr(xml_data, "<name>");
    char *status_start = strstr(xml_data, "<status>");
    char *traceid_start = strstr(xml_data, "<traceid>");
    char *spanid_start = strstr(xml_data, "<spanid>");
    
    if (interface_start && status_start && traceid_start && spanid_start) {
        // Parse interface name
        interface_start += 6;
        char *interface_end = strstr(interface_start, "</name>");
        if (interface_end) {
            int len = interface_end - interface_start;
            if (len < sizeof(o1_data->interface_name)) {
                strncpy(o1_data->interface_name, interface_start, len);
                o1_data->interface_name[len] = '\0';
            }
        }
        
        // Parse status
        status_start += 8;
        char *status_end = strstr(status_start, "</status>");
        if (status_end) {
            int len = status_end - status_start;
            if (len < sizeof(o1_data->status)) {
                strncpy(o1_data->status, status_start, len);
                o1_data->status[len] = '\0';
            }
        }
        
        // Parse traceid
        traceid_start += 9;
        char *traceid_end = strstr(traceid_start, "</traceid>");
        if (traceid_end) {
            int len = traceid_end - traceid_start;
            if (len < sizeof(o1_data->traceid)) {
                strncpy(o1_data->traceid, traceid_start, len);
                o1_data->traceid[len] = '\0';
            }
        }
        
        // Parse spanid
        spanid_start += 8;
        char *spanid_end = strstr(spanid_start, "</spanid>");
        if (spanid_end) {
            int len = spanid_end - spanid_start;
            if (len < sizeof(o1_data->spanid)) {
                strncpy(o1_data->spanid, spanid_start, len);
                o1_data->spanid[len] = '\0';
            }
        }
        
        strcpy(o1_data->operation, "edit");
        return 0;
    }
    
    return -1;
}

int handle_netconf_message(struct nc_session *session, const char *xml_data) {
    if (!session || !xml_data) {
        return -1;
    }
    
    o1_interface_data_t o1_data;
    memset(&o1_data, 0, sizeof(o1_data));
    
    // Determine message type and parse accordingly
    if (strstr(xml_data, "<get-config>")) {
        printf("Received get-config request\n");
        if (parse_o1_get_config(xml_data, &o1_data) == 0) {
            print_o1_data(&o1_data);
            
            // Send get-config response
            char response[2048];
            snprintf(response, sizeof(response),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<rpc-reply xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" message-id=\"1\">\n"
                "  <data>\n"
                "    <o1-interface xmlns=\"urn:example:o1-interface\">\n"
                "      <name>%s</name>\n"
                "      <status>up</status>\n"
                "      <tracing>\n"
                "        <traceid>%s</traceid>\n"
                "        <spanid>%s</spanid>\n"
                "      </tracing>\n"
                "    </o1-interface>\n"
                "  </data>\n"
                "</rpc-reply>\n",
                o1_data.interface_name, o1_data.traceid, o1_data.spanid);
            
            int ret = nc_send_reply(session, response, 1000);
            if (ret != NC_MSG_REPLY) {
                fprintf(stderr, "Failed to send get-config response: %s\n", nc_strerror(ret));
                return -1;
            }
            
            printf("Sent get-config response\n");
        }
        
    } else if (strstr(xml_data, "<edit-config>")) {
        printf("Received edit-config request\n");
        if (parse_o1_edit_config(xml_data, &o1_data) == 0) {
            print_o1_data(&o1_data);
            
            // Process the O1 interface configuration
            printf("Processing O1 interface configuration for %s\n", o1_data.interface_name);
            
            // Send edit-config response
            char response[512];
            snprintf(response, sizeof(response),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<rpc-reply xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" message-id=\"2\">\n"
                "  <ok/>\n"
                "</rpc-reply>\n");
            
            int ret = nc_send_reply(session, response, 1000);
            if (ret != NC_MSG_REPLY) {
                fprintf(stderr, "Failed to send edit-config response: %s\n", nc_strerror(ret));
                return -1;
            }
            
            printf("Sent edit-config response\n");
        }
        
    } else {
        printf("Unknown NETCONF message type\n");
        return -1;
    }
    
    return 0;
}

int handle_client_connection(int client_socket) {
    printf("New client connected\n");
    
    // Initialize NETCONF session for this client
    struct nc_session *session = NULL;
    int ret = nc_accept_ssh(client_socket, NULL, NULL, &session);
    if (ret != NC_MSG_HELLO) {
        fprintf(stderr, "Failed to accept NETCONF session: %s\n", nc_strerror(ret));
        close(client_socket);
        return -1;
    }
    
    printf("NETCONF session established with client\n");
    
    // Handle NETCONF messages
    while (running) {
        struct nc_msg *msg = NULL;
        ret = nc_recv_msg(session, 1000, &msg);
        
        if (ret == NC_MSG_RPC) {
            // Handle RPC message
            printf("Received RPC message from client\n");
            
            // Extract XML data from the message
            // In a real implementation, you would use proper XML parsing
            char xml_data[4096];
            // This is a simplified approach - in production, use proper XML parsing
            printf("Processing NETCONF RPC message\n");
            
            // Handle the message
            handle_netconf_message(session, xml_data);
            
        } else if (ret == NC_MSG_CLOSE) {
            printf("Client closed connection\n");
            break;
        } else if (ret == NC_MSG_ERROR) {
            fprintf(stderr, "Received error message from client\n");
            break;
        } else if (ret == NC_MSG_WOULDBLOCK) {
            // Timeout, continue
            continue;
        } else {
            fprintf(stderr, "Unexpected message type: %d\n", ret);
            break;
        }
        
        nc_msg_free(msg);
    }
    
    // Cleanup
    if (session) {
        nc_session_free(session, NULL);
    }
    close(client_socket);
    
    return 0;
}

int main(int argc, char *argv[]) {
    int port = 830;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("O1 Interface NETCONF Server\n");
    printf("Starting server on port %d\n", port);
    
    // Initialize NETCONF
    init_netconf();
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create server socket
    server_socket = setup_server_socket(port);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to setup server socket\n");
        cleanup_netconf();
        return 1;
    }
    
    printf("O1 NETCONF server listening on port %d\n", port);
    printf("Press Ctrl+C to stop the server\n");
    
    // Main server loop
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                perror("Accept failed");
            }
            continue;
        }
        
        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Handle client (in a real application, you'd use threads)
        handle_client_connection(client_socket);
    }
    
    // Cleanup
    if (server_socket != -1) {
        close(server_socket);
    }
    
    cleanup_netconf();
    printf("O1 NETCONF server stopped\n");
    
    return 0;
} 