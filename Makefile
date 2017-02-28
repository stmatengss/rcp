all:rcp rcp_debug rcp_rc

CFLAG:=-O1 -g

LIBS:=-lrdmacm -libverbs

test: test.c
	$(CC) -o $@ $< $(LIBS)

rcp_debug: rcp.c
	$(CC) -DDEBUG -o $@ $< $(LIBS)

rcp: rcp.c
	$(CC) -o $@ $< $(LIBS)


rcp_rc: rcp_rc.c
	$(CC) -o $@ $< $(LIBS)
