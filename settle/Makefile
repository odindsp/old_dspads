CC = g++

OBJECTS = \
	  json.o		\
	  json_util.o		\
	  setlog.o		\
	  tcp.o			\
	  util.o		\
	  url_endecode.o	\
	  settle.o		\
	  confoperation.o	\
	  redisimpclk.o	\
	  selfsemphore.o	\
	  init_context.o        \
	  main.o

LIBS= -lfcgi -lcrypto -lhiredis -pthread -ldl `pkg-config --libs glib-2.0` 

settle : $(OBJECTS)
	$(CC) -o settle.cgi $(OBJECTS) $(LIBS) -O2
main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
settle.o:
	$(CC) -c settle.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tcp.o:
	$(CC) -c ../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
util.o:
	$(CC) -c ../common/util.cpp $(CPPFLAGS) $(PKGOCNFIG) -O2
url_endecode.o:
	$(CC) -c ../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json_util.o:
	$(CC) -c ../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json.o:
	gcc -c ../common/json.c $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	$(CC) -c ../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` -O2 
selfsemphore.o:
	$(CC) -c ../common/selfsemphore.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
init_context.o:
	$(CC) -c ../common/init_context.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
redisimpclk.o:
	$(CC) -c ../common/redisimpclk.cpp $(CPPFLAGS) $(PKGCONFIG) -O2

install:
	mv -f settle.cgi /usr/bin/odin_settle

.PHONY: clean
clean:
	-rm $(OBJECTS)

#end of file

