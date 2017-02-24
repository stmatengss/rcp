#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

static char *port = "7471";

struct rdma_cm_id *listen_id, *id;
struct ibv_mr *mr, *send_mr;
int send_flags;
uint8_t send_msg[16];
uint8_t recv_msg[16];

static int run(void)
{

		struct rdma_addrinfo hints, *res;
		struct ibv_qp_init_attr init_attr;
		struct ibv_qp_attr qp_attr;
		struct ibv_wc wc;
		int ret;

		memset(&hints, 0, sizeof hints);
		hints.ai_flags = RAI_PASSIVE;
		hints.ai_port_space = RDMA_PS_TCP;
		ret = rdma_getaddrinfo(NULL, port, &hints, &res);
		if (ret) {

				printf("rdma_getaddrinfo: %s\n", gai_strerror(ret));
				return ret;
		}
}


int main() {
		run();
		return 0;
}
