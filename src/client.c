#include "common.h"

int main(int argc, char *argv[]) {
    netconf_config_t config;
    nc_session *session = NULL;
    tracing_data_t tracing;
    
    // Initialize default configuration
    strcpy(config.host, DEFAULT_HOST);
    config.port = DEFAULT_PORT;
    strcpy(config.username, DEFAULT_USERNAME);
    strcpy(config.password, DEFAULT_PASSWORD);
    strcpy(config.private_key_path, DEFAULT_PRIVATE_KEY);
    strcpy(config.public_key_path, DEFAULT_PUBLIC_KEY);
    
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
    
    printf("NETCONF Client - Tracing Data Sender\n");
    printf("Connecting to %s:%d as %s\n", config.host, config.port, config.username);
    
    // Initialize logging
    init_logging();
    
    // Generate tracing data
    if (generate_tracing_data(&tracing) != SUCCESS) {
        fprintf(stderr, "Failed to generate tracing data\n");
        cleanup_logging();
        return 1;
    }
    
    printf("Generated tracing data:\n");
    print_tracing_data(&tracing);
    
    // Create NETCONF session
    if (create_netconf_session(&session, &config) != SUCCESS) {
        fprintf(stderr, "Failed to create NETCONF session\n");
        cleanup_logging();
        return 1;
    }
    
    // Send tracing data
    if (send_tracing_data(session, &tracing) != SUCCESS) {
        fprintf(stderr, "Failed to send tracing data\n");
        cleanup_netconf_session(session);
        cleanup_logging();
        return 1;
    }
    
    // Receive response
    tracing_data_t received_tracing;
    if (receive_tracing_data(session, &received_tracing) != SUCCESS) {
        fprintf(stderr, "Failed to receive response\n");
        cleanup_netconf_session(session);
        cleanup_logging();
        return 1;
    }
    
    printf("Successfully sent tracing data to server\n");
    
    // Cleanup
    cleanup_netconf_session(session);
    cleanup_logging();
    
    printf("Client completed successfully\n");
    return 0;
} 