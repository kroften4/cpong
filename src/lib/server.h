/*
 * Backlog for `listen` call
 */
#define SERVER_BACKLOG 10

/*
 * Start a TCP server
 *
 * Returns server socket fd on success, and -1 on error
 *
 * Also spams errors if any occur into console
 */
int start_server(char *port);

/*
 * Start a TCP server and call `on_connection` in a new thread on connection
 * 
 * Returns -1 on error
 */
int server(char *port, void on_connection(int connfd));

