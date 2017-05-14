#ifndef JORGE_NETWORK
#define JORGE_NETWORK

void *get_in_addr(struct sockaddr *sa);

void sendall(int conn_fd, char *buffer, int *len, int isLast);
#endif
