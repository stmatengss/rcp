#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#define CPE(ret) if(ret) {\
		printf("error code: %d, %s\n", ret, gai_strerror(ret));\
		printf("ERROR: %s", strerror(errno)); \
		exit(1);\
} \

#define PRINT_LINE do { \
	printf("line: %d\n", __LINE__); \
} while(0); \

char* m_file_name_dst = "hello world";
long m_fd_sz = 100;
int m_str_len;
int m_is_server = 1;

void m_tell_inf(int argc, char **argv) {
		m_str_len = strlen(m_file_name_dst);
		if (m_is_server) {


				int listenfd = 0, connfd = 0;
				struct sockaddr_in server_addr;

				listenfd = socket(AF_INET, SOCK_STREAM, 0);
				memset(&server_addr, '0', sizeof(server_addr));

				server_addr.sin_family = AF_INET;
				server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				server_addr.sin_port = htons(5000);

				PRINT_LINE
				CPE(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) );

				PRINT_LINE
				listen(listenfd, 10);

				PRINT_LINE
				connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

				PRINT_LINE
				write(connfd, &m_fd_sz, sizeof(long));

				close(connfd);

				PRINT_LINE
				connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

				PRINT_LINE
				write(connfd, &m_str_len, sizeof(int));

				close(connfd);

				PRINT_LINE
				connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

				PRINT_LINE
				write(connfd, m_file_name_dst, m_str_len);

				close(connfd);

		} else {
				int sockfd = 0, n = 0;
				struct sockaddr_in server_addr;

				sockfd = socket(AF_INET, SOCK_STREAM, 0);
				if (sockfd < 0) {
						printf("sock build error!\n");
				}

				memset(&server_addr, '0', sizeof(server_addr));

				server_addr.sin_family = AF_INET;
				server_addr.sin_port = htonl(5000);

				PRINT_LINE
				printf("%s\n", argv[1]);
				server_addr.sin_addr.s_addr = inet_addr(argv[1]);
//				int ret = inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
//				if (ret <= 0) {
//						printf("sock inet_pton error!\n");
//				}

				PRINT_LINE
				CPE(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) );

				PRINT_LINE
				read(sockfd, &m_fd_sz, sizeof(long));
				printf("file size: %ld\n", m_fd_sz);

				PRINT_LINE
				read(sockfd, &m_str_len, sizeof(int));
				printf("m_str_len: %d\n", m_str_len);

				read(sockfd, &m_file_name_dst, m_str_len);
				printf("m_file_name_dst: %s\n", m_file_name_dst);
		}
		return;
}

int main(int argc, char **argv) {
		if (argc >= 2) {
				m_is_server = 0;
		}
		m_tell_inf(argc, argv);	
}
