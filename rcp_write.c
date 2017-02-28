#include <stdio.h>
#include <fcntl.h>
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
#include <getopt.h>

#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <rdma/rsocket.h>


char* m_port = "7471";
char* m_server_ip = "127.0.0.1";
int m_ret = 0;
char* m_file_name;
char* m_file_name_dst;

#define CPE(ret) if(ret) {\
		printf("error code: %d, %s\n", ret, gai_strerror(ret));\
		printf("ERROR: %s", strerror(errno)); \
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
#define MESSAGE_LEN 160 
#define ITER_NUM 100
#define MAX_WR_NUM 100 

struct m_conn_block {
		char file_name[64];
		long file_size;
		uint64_t	buf_va;
		uint32_t	buf_rkey;
};

struct m_ctl_block {
		struct rdma_addrinfo *res, *id;
		struct rdma_cm_id *lis_id;
		struct rdma_cm_id* m_id;
		struct ibv_mr *send_mr, *recv_mr;
	   	struct ibv_qp *qp;
		struct ibv_cq *cq;
		struct rdma_event_channel *m_cm_ch;
		struct rdma_conn_param *m_conn_param;
		struct rdma_cm_event *m_cm_event; 
//		uint8_t send_msg[MESSAGE_LEN];
//		uint8_t recv_msg[MESSAGE_LEN];
};

int m_is_server = 0;
//uint8_t m_send_msg[MESSAGE_LEN];
//uint8_t m_recv_msg[MESSAGE_LEN];
char m_send_msg[MESSAGE_LEN] = "Hello World\n";
char m_recv_msg[MESSAGE_LEN];
char *m_file_whole;

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
		m_hint->ai_port_space = RDMA_PS_IB;
//		m_hint->ai_port_space = RDMA_PS_TCP;
		m_hint->ai_qp_type = IBV_QPT_UC;
//		m_hint->ai_qp_type = IBV_QPT_RC;
		if(m_is_server) {
				m_hint->ai_flags = RAI_PASSIVE;
//				m_hint->ai_flags = RAI_NUMERICHOST;
		} else {
//				m_hint->ai_flags = RAI_NUMERICHOST;
				m_hint->ai_flags = AI_NUMERICHOST;
		}	
}

void m_qp_attr_init(struct ibv_qp_init_attr *m_init_attr) {
		PRINT_FUNC
		FILL_P(m_init_attr);
		m_init_attr->cap.max_send_wr = m_init_attr->cap.max_recv_wr = MAX_WR_NUM;
		m_init_attr->cap.max_send_sge = m_init_attr->cap.max_send_sge = 1;
		m_init_attr->cap.max_inline_data = MESSAGE_LEN;
		m_init_attr->sq_sig_all = 1;
//		m_init_attr->qp_type = IBV_QPT_RC;
		m_init_attr->qp_type = IBV_QPT_UC;
}

long m_file_size(FILE *m_fd) {
		fseek(m_fd, 0L, SEEK_END);
		long sz = ftell(m_fd);
		fseek(m_fd, 0L, SEEK_SET);
		return sz;
}

void m_run_server() {
		
//		 fd = open(m_file_name, O_RDONLY, 0644);
		FILE *m_fd = fopen(m_file_name, "r");
		if (m_fd == NULL) {
				printf("open failed! exit!\n");
				exit(-1);
		}
		
		long m_fd_sz = m_file_size(m_fd);
		
		m_file_whole = (char *)malloc(m_fd_sz + 1);
		fread(m_file_whole, 1, m_fd_sz, m_fd);

		fclose(m_fd);

		struct m_ctl_block* m_ctl_block = (struct m_ctl_block*)malloc(sizeof(struct m_ctl_block));

//		struct rdma_cm_id* m_id;
		struct rdma_addrinfo m_hint;
		struct ibv_qp_init_attr m_init_attr;
		struct ibv_qp_attr m_attr;
		struct ibv_wc m_wc;
		struct m_conn_block m_conn_block = {

		};
//		m_fill_string(m_send_msg);

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
		m_ctl_block->m_cm_ch = rdma_create_event_channel();

		PRINT_LINE
		CPE(rdma_migrate_id(m_ctl_block->lis_id, m_ctl_block->m_cm_ch));

//#define FIXME

#ifdef FIXME
		PRINT_LINE //FIXME
		CPE(rdma_get_request(m_ctl_block->lis_id, &(m_ctl_block->m_id)) );
#endif
		PRINT_LINE
//	/*
		CPE(rdma_get_cm_event(m_ctl_block->m_cm_ch, &m_ctl_block->m_cm_event) );
		if (m_ctl_block->m_cm_event->event != RDMA_CM_EVENT_CONNECT_REQUEST) {
				printf("event error\n");
				exit(1);
		}

		PRINT_LINE
		m_ctl_block->m_id = m_ctl_block->m_cm_event->id;

		PRINT_LINE
		rdma_ack_cm_event(m_ctl_block->m_cm_event);
//*/
		PRINT_LINE
		rdma_create_qp(m_ctl_block->m_id, m_ctl_block->lis_id->pd, &m_init_attr);

		PRINT_LINE
		m_get_qp_info(m_ctl_block->m_id->qp);

		PRINT_LINE
		m_ctl_block->send_mr = rdma_reg_msgs(m_ctl_block->m_id, m_send_msg, MESSAGE_LEN);
		
//		PRINT_LINE
//		m_ctl_block->recv_mr = rdma_reg_msgs(m_ctl_block->m_id, m_recv_msg, MESSAGE_LEN);

		PRINT_LINE
		m_conn_block.file_size = m_fd_sz;
		int m_str_len = strlen(m_file_name_dst);
		memcpy(m_conn_block.file_name, m_file_name_dst, m_str_len);		
		m_conn_block.file_name[m_str_len] = '\0';
//		m_conn_block.file_name = m_file_name_dst;

		m_ctl_block->m_conn_param = (struct rdma_conn_param *)malloc(sizeof(struct rdma_conn_param));
		PRINT_LINE
		m_ctl_block->m_conn_param->private_data = &m_conn_block; 

		PRINT_LINE
		m_ctl_block->m_conn_param->private_data_len = sizeof (m_conn_block);

		PRINT_LINE
		CPE(rdma_accept(m_ctl_block->m_id, m_ctl_block->m_conn_param));				

		PRINT_LINE
		CPE(rdma_get_cm_event(m_ctl_block->m_cm_ch, &(m_ctl_block->m_cm_event)) );

		PRINT_LINE
		rdma_ack_cm_event(m_ctl_block->m_cm_event);

		int counter = 0;
		while(counter < m_fd_sz) {
				int msg_size = MESSAGE_LEN;
//				int msg_size = counter + MESSAGE_LEN >= m_fd_sz ? m_fd_sz - counter : MESSAGE_LEN;
				printf("msg size: %d\n", msg_size);

				char m_buffer[MESSAGE_LEN];
				memcpy(m_buffer, m_file_whole + counter, msg_size);
//				printf("msg: %s\n", m_buffer);
//				m_buffer[MESSAGE_LEN] = '\0';
//				printf("send messge: %s\n", m_buffer);
				CPE(rdma_post_send(m_ctl_block->m_id, NULL, m_file_whole + counter, msg_size, 
						m_ctl_block->send_mr, IBV_SEND_INLINE));
				counter += MESSAGE_LEN;
		}

//#define ITER_TEST

#ifdef ITER_TEST
		int i;
		for (i = 0; i < ITER_NUM; i ++ ) {
				PRINT_LINE
				CPE(rdma_post_send(m_ctl_block->m_id, NULL, m_send_msg, MESSAGE_LEN, 
						m_ctl_block->send_mr, IBV_SEND_INLINE));

		}
#endif
		
}

void m_run_client() {


		struct m_ctl_block* m_ctl_block = (struct m_ctl_block*)malloc(sizeof(struct m_ctl_block));

		struct rdma_addrinfo m_hint;
		struct ibv_qp_init_attr m_init_attr;
		struct ibv_qp_attr m_attr;
		struct ibv_wc m_wc; 
		struct m_conn_block m_conn_block;

		m_addrinfo_init(&m_hint);

		PRINT_LINE
		m_ctl_block->m_cm_ch = rdma_create_event_channel();
		
		PRINT_LINE
		CPE(rdma_getaddrinfo(m_server_ip, m_port, &m_hint, &(m_ctl_block->res)) );

		PRINT_LINE
		m_qp_attr_init(&m_init_attr);

		PRINT_LINE
		m_init_attr.qp_context = m_ctl_block->m_id;
		CPE(rdma_create_ep(&(m_ctl_block->m_id), m_ctl_block->res, NULL, &m_init_attr) );

		PRINT_LINE
		CPE(rdma_migrate_id(m_ctl_block->m_id, m_ctl_block->m_cm_ch));

//		PRINT_LINE
//		m_ctl_block->send_mr = rdma_reg_msgs(m_ctl_block->m_id, m_send_msg, MESSAGE_LEN);

		m_file_whole = (char *)malloc(ITER_NUM * MESSAGE_LEN + 100);
		
		PRINT_LINE
//		m_ctl_block->recv_mr = rdma_reg_msgs(m_ctl_block->m_id, m_recv_msg, MESSAGE_LEN);
		m_ctl_block->recv_mr = rdma_reg_msgs(m_ctl_block->m_id, m_file_whole, ITER_NUM * MESSAGE_LEN);

		int i;
		for (i = 0; i < ITER_NUM; i ++ ) {
				PRINT_LINE
//				CPE(rdma_post_recv(m_ctl_block->m_id, NULL, m_recv_msg, 
				CPE(rdma_post_recv(m_ctl_block->m_id, NULL, m_file_whole + i * MESSAGE_LEN, 
						MESSAGE_LEN, m_ctl_block->recv_mr ));
		}

		PRINT_LINE
		m_ctl_block->m_conn_param = (struct rdma_conn_param *)malloc(sizeof(struct rdma_conn_param));

		PRINT_LINE
		CPE(rdma_connect(m_ctl_block->m_id, NULL));
//		CPE(rdma_connect(m_ctl_block->m_id, m_ctl_block->m_conn_param));

		PRINT_LINE
		CPE(rdma_get_cm_event(m_ctl_block->m_cm_ch, &(m_ctl_block->m_cm_event)) );
		memcpy(&m_conn_block, m_ctl_block->m_cm_event->param.conn.private_data,
					   	sizeof(m_conn_block));		
		printf("file size is %ld\n", m_conn_block.file_size);
		printf("file name is %s\n", m_conn_block.file_name);

		long m_fd_sz = m_conn_block.file_size;
		m_file_name_dst = m_conn_block.file_name;
		FILE *m_fd = fopen(m_file_name_dst, "wr");
		if (m_fd == NULL) {
				printf("open failed! EXIT!\n");
				exit(-1);
		}

		PRINT_LINE
		rdma_ack_cm_event(m_ctl_block->m_cm_event);

//		m_file_whole = (char *)malloc(m_fd_sz + 1);
/*
		int counter = 0;
		while(counter < m_fd_sz) {
				int msg_size = counter + MESSAGE_LEN >= m_fd_sz ? m_fd_sz - counter : MESSAGE_LEN;
				printf("msg size: %d\n", msg_size);

				while(rdma_get_recv_comp(m_ctl_block->m_id, &m_wc) == 0);

				printf("msg: %s\n", m_recv_msg);

				memcpy(m_file_whole + counter, m_recv_msg, msg_size);
				counter += msg_size;
		}
*/
		fwrite(m_file_whole, 1, m_fd_sz, m_fd);
		fclose(m_fd);
#ifdef ITER_TEST
		for (i = 0; i < ITER_NUM; i ++ ) {
				PRINT_LINE
				while(rdma_get_recv_comp(m_ctl_block->m_id, &m_wc) == 0);

				PRINT_LINE
				printf("message: %s\n", m_recv_msg);

		}
#endif
}

int main(int argc, char** argv) {
#ifdef DEBUG
		test();
#endif
		int op;

		while((op = getopt(argc, argv, "sci:p:f:d:")) != -1) {
				switch (op) {
						case 'f':
								m_file_name = optarg;
								break;
						case 's':
								m_is_server = 1;
								break;
						case 'c':
								m_is_server = 0;
								break;
						case 'i':
								m_server_ip = optarg;
								break;
						case 'p':
								m_port = optarg;
								break;
						case 'd':
								m_file_name_dst = optarg;
								break;
						default:
								printf("usage: ./rcp [-s/c] [-i ip_addr] [-p port]");
								exit(1);
				}
		}

		if (m_is_server) {
				printf("Server start:\n");
				m_run_server();
				printf("Server end:\n");
		} else {
				printf("Client start:\n");
				m_run_client();
				printf("Client end:\n");
		}
		return 0;
}
