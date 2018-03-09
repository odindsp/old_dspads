CC = g++
adx=iqiyi_mini
OBJECTS = confoperation.o	\
	  json.o		\
	  util.o		\
	  json_util.o		\
	  setlog.o		\
	  tcp.o			\
	  iqiyi_adapter.o	\
	  url_endecode.o	\
	  main.o                


LIBS= -lfcgi -lcrypto -lhiredis -pthread -lprotobuf -lmaxminddb -lrt `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) -O2

main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
iqiyi_adapter.o:
	$(CC) -c iqiyi_adapter.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	 $(CC) -c ../../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` -O2
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
util.o:
	$(CC) -c ../../common/util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json.o:
	gcc -c ../../common/json.c $(CPPFLAGS) $(PKGCONFIG) -O2
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/odin_$(adx)	
.PHONY: clean
clean:
	rm  $(OBJECTS)

#end of file
