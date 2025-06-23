#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

static volatile int running = 1;
static int server_socket = -1;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
    if (server_socket != -1) {
        close(server_socket);
    }
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

int handle_client_connection(int client_socket) {
    printf("New client connected\n");
    
    // Initialize NETCONF session for this client
    struct nc_session *session = NULL;
    int ret = nc_accept_ssh(client_socket, NULL, NULL, &session);
    if (ret != NC_MSG_HELLO) {
        fprintf(stderr, "Failed to accept NETCONF session: %s\n", nc_strerror(ret));
        close(client_socket);
        return ERROR_SESSION;
    }
    
    printf("NETCONF session established with client\n");
    
    // Handle NETCONF messages
    while (running) {
        struct nc_msg *msg = NULL;
        ret = nc_recv_msg(session, 1000, &msg);
        
        if (ret == NC_MSG_RPC) {
            // Handle RPC message
            printf("Received RPC message from client\n");
            
            // Extract tracing data from the message
            // This is a simplified implementation
            tracing_data_t tracing;
            memset(&tracing, 0, sizeof(tracing));
            
            // In a real implementation, you would parse the XML message
            // to extract traceid and spanid
            printf("Processing tracing data...\n");
            
            // Send response
            char response[512];
            snprintf(response, sizeof(response),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<rpc-reply xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" message-id=\"1\">\n"
                "  <ok/>\n"
                "</rpc-reply>\n");
            
            ret = nc_send_reply(session, response, 1000);
            if (ret != NC_MSG_REPLY) {
                fprintf(stderr, "Failed to send response: %s\n", nc_strerror(ret));
                break;
            }
            
            printf("Response sent to client\n");
            
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
    
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("Starting NETCONF Server on port %d\n", port);
    
    // Initialize logging
    init_logging();
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create server socket
    server_socket = setup_server_socket(port);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to setup server socket\n");
        cleanup_logging();
        return 1;
    }
    
    printf("Server listening on port %d\n", port);
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
        
        // Handle client in a new thread (simplified - in production use pthread)
        handle_client_connection(client_socket);
    }
    
    // Cleanup
    if (server_socket != -1) {
        close(server_socket);
    }
    
    cleanup_logging();
    printf("Server stopped\n");
    
    return 0;
} 