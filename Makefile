all:rcp rcp_debug

CFLAG:=-O1 -g

test: test.c
	$(CC) -o $@ $< -lrdmacm -libverbs

rcp_debug: rcp.c
	$(CC) -DDEBUG -o $@ $< -lrdmacm -libverbs

rcp: rcp.c
	$(CC) -o $@ $< -lrdmacm -libverbs
