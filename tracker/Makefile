CC = g++
CFLAGS = -O2
OBJECTS = main.o			\
	tracker.o			\
	setlog.o			\
	rdkafka_operator.o            	\
	tcp.o				\
	util.o				\
	json_util.o			\
	confoperation.o       		\
	redisimpclk.o        		\
        selfsemphore.o        		\
        init_context.o        		\
	url_endecode.o			\
	json.o

LIBS= -lcrypto -lfcgi -lhiredis -pthread -lrt -ldl -lz -lrdkafka -lrdkafka++ `pkg-config --libs glib-2.0`

tracker : $(OBJECTS)
	$(CC) -o tracker.cgi $(OBJECTS) $(LIBS) $(CFLAGS)
main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
tracker.o:
	$(CC) -c tracker.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
setlog.o:
	$(CC) -c ../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
util.o:
	$(CC) -c ../common/util.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
tcp.o:
	$(CC) -c ../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
rdkafka_operator.o:
	$(CC) -c ../common/rdkafka_operator.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
json_util.o:
	$(CC) -c ../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
json.o:
	gcc -c ../common/json.c $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
url_endecode.o:
	$(CC) -c ../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
confoperation.o:
	$(CC) -c ../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` $(CFLAGS)
selfsemphore.o:
	$(CC) -c ../common/selfsemphore.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
init_context.o:
	$(CC) -c ../common/init_context.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)
redisimpclk.o:
	$(CC) -c ../common/redisimpclk.cpp $(CPPFLAGS) $(PKGCONFIG) $(CFLAGS)

install:
	mv -f tracker.cgi /usr/bin/odin_tracker

.PHONY: clean
clean:
	-rm $(OBJECTS)

#end of file


