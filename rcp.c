#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <netdb.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <rdma/rsocket.h>


char* m_port = "7471";
char* m_server_ip = "127.0.0.1";
int m_ret = 0;

#define CPE(ret) if(ret) {\
		printf("error code: %d, %s\n", ret, gai_strerror(ret));\
		exit(1);\
} \

#define CPNE(ret) if(!ret) {\
		printf("error code: res is NULL\n"); \
		exit(1); \
}

#define FILL(name) memset(&name, 0, sizeof(name));
#define FILL_P(name) memset(name, 0, sizeof(*name));
#define PRINT_LINE do { \
	printf("line: %d\n", __LINE__); \
} while(0); \

#define PRINT_FUNC do { \
	printf("func: %s\n", __FUNCTION__); \
} while(0); \

#define BACK_LOG 10
#define MESSAGE_LEN 256

struct m_ctl_block {
		struct rdma_addrinfo *res, *id;
		struct rdma_cm_id *lis_id;
		struct rdma_cm_id* m_id;
		struct ibv_mr *send_mr, *recv_mr;
	   	struct ibv_qp *qp;
		struct ibv_cq *cq;
//		uint8_t send_msg[MESSAGE_LEN];
//		uint8_t recv_msg[MESSAGE_LEN];
};

int m_is_server = 0;
//uint8_t m_send_msg[MESSAGE_LEN];
//uint8_t m_recv_msg[MESSAGE_LEN];
char m_send_msg[MESSAGE_LEN];
char m_recv_msg[MESSAGE_LEN];

void m_fill_string(char *str) {
		int i;
		for (i = 0; i < MESSAGE_LEN; i ++ ) {
				str[i] = 'a' + i % 26;
		}
}

void m_get_device_info() {
		PRINT_FUNC
		struct ibv_device **m_dev_list;
		int m_dev_num, i;

		m_dev_list = ibv_get_device_list(&m_dev_num);
		CPNE(m_dev_list);

		for (i = 0; i < m_dev_num; i ++ ) {

				struct ibv_device_attr m_device_attr;
				struct ibv_context *m_ctx;

				m_ctx = ibv_open_device(m_dev_list[i]);
				CPNE(m_ctx);

				CPE(ibv_query_device(m_ctx, &m_device_attr));

				printf("RDMA device[%d] name: %s\n", i, 
								ibv_get_device_name(m_dev_list[i]));
				printf("Name: %s\n", m_dev_list[i]->name);
				printf("Device name: %s\n", m_dev_list[i]->dev_name);
				printf("GUID: %016llx\n", (unsigned long long)ibv_get_device_guid(m_dev_list[i]));
				printf("Node Type: %d\n", m_dev_list[i]->node_type);
				printf("Transport Type: %d\n", m_dev_list[i]->transport_type);

				printf("Fw: %s\n", m_device_attr.fw_ver);
				printf("Max_mr_size: %" PRIu64 "\n", m_device_attr.max_mr_size);
				printf("Page_size_cap: %" PRIu64 "\n", m_device_attr.page_size_cap);
				printf("Max_qp: %d\n", m_device_attr.max_sge);

		}
}

void m_get_qp_info(struct ibv_qp *m_qp) {
		struct ibv_qp_attr m_attr;
		struct ibv_qp_init_attr m_init_attr;
		ibv_query_qp(m_qp, &m_attr, 0, &m_init_attr);

		printf("Max_inline_data: %d\n", m_init_attr.cap.max_inline_data);

}

void test() {

		m_get_device_info();
}

void m_addrinfo_init(struct rdma_addrinfo *m_hint) {
		PRINT_FUNC
		FILL_P(m_hint);
		m_hint->ai_port_space = RDMA_PS_TCP;
		if(m_is_server) {
				m_hint->ai_flags = RAI_PASSIVE;
		} else {
				m_hint->ai_flags = RAI_NUMERICHOST;
		}	
}

void m_qp_attr_init(struct ibv_qp_init_attr *m_init_attr) {
		PRINT_FUNC
		FILL_P(m_init_attr);
		m_init_attr->cap.max_send_wr = m_init_attr->cap.max_recv_wr = 1;
		m_init_attr->cap.max_send_sge = m_init_attr->cap.max_send_sge = 1;
		m_init_attr->cap.max_inline_data = MESSAGE_LEN;
		m_init_attr->sq_sig_all = 1;
}

void m_run_server() {

		struct m_ctl_block* m_ctl_block = (struct m_ctl_block*)malloc(sizeof(struct m_ctl_block));

//		struct rdma_cm_id* m_id;
		struct rdma_addrinfo m_hint;
		struct ibv_qp_init_attr m_init_attr;
		struct ibv_qp_attr m_attr;
		struct ibv_wc m_wc;

		m_fill_string(m_send_msg);

		m_addrinfo_init(&m_hint);
//		memset(&m_hint, 0, sizeof m_hint);
//		m_hint.ai_flags = RAI_PASSIVE;
//		m_hint.ai_port_space = RDMA_PS_TCP

		PRINT_LINE

		CPE(rdma_getaddrinfo(m_server_ip, m_port, &m_hint, &(m_ctl_block->res)) );
//		CPE(rdma_getaddrinfo(NULL, m_port, &m_hint, &res) );
	
		PRINT_LINE	
		m_qp_attr_init(&m_init_attr);
		CPE(rdma_create_ep(&(m_ctl_block->lis_id), m_ctl_block->res, NULL, &m_init_attr) );

		PRINT_LINE	
		rdma_freeaddrinfo(m_ctl_block->res);

		PRINT_LINE
		CPE(rdma_listen(m_ctl_block->lis_id, BACK_LOG));
//		CPE(rdma_listen(m_ctl_block->lis_id, &(m_ctl_block->id)));
		
		PRINT_LINE
		CPE(rdma_get_request(m_ctl_block->lis_id, &(m_ctl_block->m_id)) );

		PRINT_LINE
		m_get_qp_info(m_ctl_block->m_id->qp);

		PRINT_LINE
		m_ctl_block->send_mr = rdma_reg_msgs(m_ctl_block->m_id, m_send_msg, MESSAGE_LEN);
		
		PRINT_LINE
		m_ctl_block->recv_mr = rdma_reg_msgs(m_ctl_block->m_id, m_recv_msg, MESSAGE_LEN);

		PRINT_LINE
		CPE(rdma_accept(m_ctl_block->m_id, NULL));				

		PRINT_LINE
		CPE(rdma_post_send(m_ctl_block->m_id, NULL, m_send_msg, MESSAGE_LEN, 
								m_ctl_block->send_mr, IBV_SEND_INLINE));
}

void m_run_client() {
		struct m_ctl_block* m_ctl_block = (struct m_ctl_block*)malloc(sizeof(struct m_ctl_block));

		struct rdma_addrinfo m_hint;
		struct ibv_qp_init_attr m_init_attr;
		struct ibv_qp_attr m_attr;
		struct ibv_wc m_wc; 

		m_addrinfo_init(&m_hint);
		
		PRINT_LINE
		CPE(rdma_getaddrinfo(m_server_ip, m_port, &m_hint, &(m_ctl_block->res)) );

		PRINT_LINE
		m_qp_attr_init(&m_init_attr);

		PRINT_LINE
		m_init_attr.qp_context = m_ctl_block->m_id;
		CPE(rdma_create_ep(&(m_ctl_block->m_id), m_ctl_block->res, NULL, &m_init_attr) );

		PRINT_LINE
		m_ctl_block->send_mr = rdma_reg_msgs(m_ctl_block->m_id, m_send_msg, MESSAGE_LEN);
		
		PRINT_LINE
		m_ctl_block->recv_mr = rdma_reg_msgs(m_ctl_block->m_id, m_recv_msg, MESSAGE_LEN);

		PRINT_LINE
		CPE(rdma_post_recv(m_ctl_block->m_id, NULL, m_recv_msg, MESSAGE_LEN, m_ctl_block->recv_mr ));

		PRINT_LINE
		CPE(rdma_connect(m_ctl_block->m_id, NULL));

		PRINT_LINE
		while(rdma_get_recv_comp(m_ctl_block->m_id, &m_wc) == 0);

		PRINT_LINE
		printf("message: %s\n", m_recv_msg);
}

int main(int argc, char** argv) {
#ifdef DEBUG
		test();
#endif
		if (argc == 1) {
				m_is_server = 1;
				printf("Server start:\n");
				m_run_server();
				printf("Server end:\n");
		} else {
				m_server_ip = argv[1];
				printf("Client start:\n");
				m_run_client();
				printf("Client end:\n");
		}
		return 0;
}
