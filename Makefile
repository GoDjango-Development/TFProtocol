cflags = -D_FILE_OFFSET_BITS=64 -I ./include -I /usr/include/mysql -Wall \
	-I /usr/include/postgresql -Wno-parentheses -Wno-pointer-sign
libs = -lpthread -lcrypto -lrt -ldl -lsqlite3 -lmysqlclient -lpq

#Release profile

obj = release/obj/main.o release/obj/net.o release/obj/init.o release/obj/log.o \
	release/obj/tfproto.o release/obj/sig.o release/obj/cmd.o release/obj/util.o \
	release/obj/ntfy.o release/obj/mempool.o release/obj/crypto.o \
	release/obj/xs1.o release/obj/xs1utils.o release/obj/xmods.o \
	release/obj/xs_ime.o release/obj/xs_imeopc1.o release/obj/udp_keep.o \
	release/obj/xs_ntmex.o release/obj/xs_sqlite.o release/obj/xs_ace.o \
	release/obj/xs_mysql.o release/obj/xs_postgresql.o \
	release/obj/xs_gateway.o release/obj/trp.o release/obj/xs_rpcproxy.o \
	release/obj/core.o

CCX = gcc -o
CC = gcc -c

release: $(obj)
	$(CCX) release/tfd $(obj) $(ldflags) $(libs)

release/obj/main.o: src/main.c
	$(CC) src/main.c -o release/obj/main.o $(cflags) 

release/obj/net.o: src/net.c include/net.h
	$(CC) src/net.c -o release/obj/net.o $(cflags) 

release/obj/init.o: src/init.c include/init.h
	$(CC) src/init.c -o release/obj/init.o $(cflags) 

release/obj/log.o: src/log.c include/log.h
	$(CC) src/log.c -o release/obj/log.o $(cflags) 

release/obj/tfproto.o: src/tfproto.c include/tfproto.h
	$(CC) src/tfproto.c -o release/obj/tfproto.o $(cflags)
	
release/obj/sig.o: src/sig.c include/sig.h
	$(CC) src/sig.c -o release/obj/sig.o $(cflags)

release/obj/cmd.o: src/cmd.c include/cmd.h
	$(CC) src/cmd.c -o release/obj/cmd.o $(cflags) 

release/obj/util.o: src/util.c include/util.h
	$(CC) src/util.c -o release/obj/util.o $(cflags) 

release/obj/ntfy.o: src/ntfy.c include/ntfy.h
	$(CC) src/ntfy.c -o release/obj/ntfy.o $(cflags)

release/obj/mempool.o: src/mempool.c include/mempool.h
	$(CC) src/mempool.c -o release/obj/mempool.o $(cflags) 
	
release/obj/crypto.o: src/crypto.c include/xs1/xs1.h
	$(CC) src/crypto.c -o release/obj/crypto.o $(cflags)

release/obj/xs1.o: src/xs1/xs1.c include/xs1/xs1.h
	$(CC) src/xs1/xs1.c -o release/obj/xs1.o $(cflags) 

release/obj/xs1utils.o: src/xs1/xs1utils.c include/xs1/xs1utils.h
	$(CC) src/xs1/xs1utils.c -o release/obj/xs1utils.o $(cflags) 

release/obj/xmods.o: src/xmods.c include/xmods.h
	$(CC) src/xmods.c -o release/obj/xmods.o $(cflags) 

release/obj/xs_ime.o: src/xs_ime/xs_ime.c include/xs_ime/xs_ime.h
	$(CC) src/xs_ime/xs_ime.c -o release/obj/xs_ime.o $(cflags) 

release/obj/xs_imeopc1.o: src/xs_ime/xs_imeopc1.c include/xs_ime/xs_imeopc1.h
	$(CC) src/xs_ime/xs_imeopc1.c -o release/obj/xs_imeopc1.o $(cflags) 

release/obj/udp_keep.o: src/udp_keep.c include/udp_keep.h
	$(CC) src/udp_keep.c -o release/obj/udp_keep.o $(cflags) 

release/obj/xs_ntmex.o: src/xs_ntmex/xs_ntmex.c include/xs_ntmex/xs_ntmex.h
	$(CC) src/xs_ntmex/xs_ntmex.c -o release/obj/xs_ntmex.o $(cflags) 

release/obj/xs_sqlite.o: src/xs_sqlite/xs_sqlite.c src/xs_sqlite/xs_sqlite.h
	$(CC) src/xs_sqlite/xs_sqlite.c -o release/obj/xs_sqlite.o $(cflags) 

release/obj/xs_mysql.o: src/xs_mysql/xs_mysql.c src/xs_mysql/xs_mysql.h
	$(CC) src/xs_mysql/xs_mysql.c -o release/obj/xs_mysql.o $(cflags)  

release/obj/xs_postgresql.o: src/xs_postgresql/xs_postgresql.c src/xs_postgresql/xs_postgresql.h
	$(CC) src/xs_postgresql/xs_postgresql.c -o release/obj/xs_postgresql.o $(cflags)  
	
release/obj/xs_ace.o: src/xs_ace/xs_ace.c include/xs_ace/xs_ace.h
	$(CC) src/xs_ace/xs_ace.c -o release/obj/xs_ace.o $(cflags)

release/obj/xs_gateway.o: src/xs_gateway/xs_gateway.c include/xs_gateway/xs_gateway.h
	$(CC) src/xs_gateway/xs_gateway.c -o release/obj/xs_gateway.o $(cflags)

release/obj/trp.o: src/trp.c include/trp.h
	$(CC) src/trp.c -o release/obj/trp.o $(cflags)

release/obj/xs_rpcproxy.o: src/xs_rpcproxy/xs_rpcproxy.c include/xs_rpcproxy/xs_rpcproxy.h
	$(CC) src/xs_rpcproxy/xs_rpcproxy.c -o release/obj/xs_rpcproxy.o $(cflags)
		
release/obj/core.o: src/core.c include/core.h
	$(CC) src/core.c -o release/obj/core.o $(cflags)

# Debug profile

dbg = debug/obj/main.o debug/obj/net.o debug/obj/init.o debug/obj/log.o \
	debug/obj/tfproto.o debug/obj/sig.o debug/obj/cmd.o debug/obj/util.o \
	debug/obj/ntfy.o debug/obj/mempool.o debug/obj/crypto.o \
	debug/obj/xs1.o debug/obj/xs1utils.o debug/obj/xmods.o \
	debug/obj/xs_ime.o debug/obj/xs_imeopc1.o debug/obj/udp_keep.o \
	debug/obj/xs_ntmex.o debug/obj/xs_sqlite.o debug/obj/xs_ace.o \
	debug/obj/xs_mysql.o debug/obj/xs_postgresql.o debug/obj/xs_gateway.o \
	debug/obj/trp.o debug/obj/xs_rpcproxy.o debug/obj/core.o

CCGX = gcc -g -DDEBUG -o
CCG = gcc -g -DDEBUG -c
	
debug: $(dbg)
	$(CCGX) debug/tfd $(dbg) $(ldflags) $(libs)

debug/obj/main.o: src/main.c
	$(CCG) src/main.c -o debug/obj/main.o $(cflags) 

debug/obj/net.o: src/net.c include/net.h
	$(CCG) src/net.c -o debug/obj/net.o $(cflags) 

debug/obj/init.o: src/init.c include/init.h
	$(CCG) src/init.c -o debug/obj/init.o $(cflags) 

debug/obj/log.o: src/log.c include/log.h
	$(CCG) src/log.c -o debug/obj/log.o $(cflags) 

debug/obj/tfproto.o: src/tfproto.c include/tfproto.h
	$(CCG) src/tfproto.c -o debug/obj/tfproto.o $(cflags)
	
debug/obj/sig.o: src/sig.c src include/sig.h
	$(CCG) src/sig.c -o debug/obj/sig.o $(cflags)

debug/obj/cmd.o: src/cmd.c include/cmd.h
	$(CCG) src/cmd.c -o debug/obj/cmd.o $(cflags) 

debug/obj/util.o: src/util.c include/util.h
	$(CCG) src/util.c -o debug/obj/util.o $(cflags) 

debug/obj/ntfy.o: src/ntfy.c include/ntfy.h
	$(CCG) src/ntfy.c -o debug/obj/ntfy.o $(cflags)

debug/obj/mempool.o: src/mempool.c include/mempool.h
	$(CCG) src/mempool.c -o debug/obj/mempool.o $(cflags) 
	
debug/obj/crypto.o: src/crypto.c include/crypto.h
	$(CCG) src/crypto.c -o debug/obj/crypto.o $(cflags)

debug/obj/xs1.o: src/xs1/xs1.c include/xs1/xs1.h
	$(CCG) src/xs1/xs1.c -o debug/obj/xs1.o $(cflags) 

debug/obj/xs1utils.o: src/xs1/xs1utils.c include/xs1/xs1utils.h
	$(CCG) src/xs1/xs1utils.c -o debug/obj/xs1utils.o $(cflags) 

debug/obj/xmods.o: src/xmods.c include/xmods.h
	$(CCG) src/xmods.c -o debug/obj/xmods.o $(cflags) 

debug/obj/xs_ime.o: src/xs_ime/xs_ime.c include/xs_ime/xs_ime.h
	$(CCG) src/xs_ime/xs_ime.c -o debug/obj/xs_ime.o $(cflags) 
	
debug/obj/xs_imeopc1.o: src/xs_ime/xs_imeopc1.c include/xs_ime/xs_imeopc1.h
	$(CCG) src/xs_ime/xs_imeopc1.c -o debug/obj/xs_imeopc1.o $(cflags) 
	
debug/obj/udp_keep.o: src/udp_keep.c include/udp_keep.h
	$(CCG) src/udp_keep.c -o debug/obj/udp_keep.o $(cflags) 

debug/obj/xs_ntmex.o: src/xs_ntmex/xs_ntmex.c include/xs_ntmex/xs_ntmex.h
	$(CCG) src/xs_ntmex/xs_ntmex.c -o debug/obj/xs_ntmex.o $(cflags) 

debug/obj/xs_sqlite.o: src/xs_sqlite/xs_sqlite.c src/xs_sqlite/xs_sqlite.h
	$(CCG) src/xs_sqlite/xs_sqlite.c -o debug/obj/xs_sqlite.o $(cflags) 

debug/obj/xs_mysql.o: src/xs_mysql/xs_mysql.c src/xs_mysql/xs_mysql.h
	$(CCG) src/xs_mysql/xs_mysql.c -o debug/obj/xs_mysql.o $(cflags)

debug/obj/xs_postgresql.o: src/xs_postgresql/xs_postgresql.c src/xs_postgresql/xs_postgresql.h
	$(CCG) src/xs_postgresql/xs_postgresql.c -o debug/obj/xs_postgresql.o $(cflags)
		
debug/obj/xs_ace.o: src/xs_ace/xs_ace.c include/xs_ace/xs_ace.h
	$(CCG) src/xs_ace/xs_ace.c -o debug/obj/xs_ace.o $(cflags) 

debug/obj/xs_gateway.o: src/xs_gateway/xs_gateway.c include/xs_gateway/xs_gateway.h
	$(CCG) src/xs_gateway/xs_gateway.c -o debug/obj/xs_gateway.o $(cflags) 

debug/obj/trp.o: src/trp.c include/trp.h
	$(CCG) src/trp.c -o debug/obj/trp.o $(cflags) 

debug/obj/xs_rpcproxy.o: src/xs_rpcproxy/xs_rpcproxy.c include/xs_rpcproxy/xs_rpcproxy.h
	$(CCG) src/xs_rpcproxy/xs_rpcproxy.c -o debug/obj/xs_rpcproxy.o $(cflags) 		

debug/obj/core.o: src/core.c include/core.h
	$(CCG) src/core.c -o debug/obj/core.o $(cflags)	
	
#Others rules

prepare:
	mkdir -p debug release debug/obj release/obj

help:
	echo "usage: \nmake prepare -> Make all preparation before making the release or the debug binary\nmake release -> Make the release binary to be used in production\nmake debug -> Make the debug binary which have verbose output usefull for development environment\nmake clean -> Make the cleanup erasing all binaries"

clean:
	rm -R -f $(obj) $(dbg) release/obj/* debug/obj/* release/tfd debug/tfd
