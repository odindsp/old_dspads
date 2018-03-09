CC = g++
adx=tanx
OBJECTS = tanx-bidding.pb.o	\
	  setlog.o		\
	  tanx_adapter.o	\
	  main.o                \
          confoperation.o       \
          json_util.o		\
	  json.o		\
	   url_endecode.o	\
          util.o                \
          tcp.o			\

LIBS= -lrt -lcrypto -lfcgi -lhiredis -pthread -lprotobuf `pkg-config --libs glib-2.0`
$(adx).cgi : $(OBJECTS)
	$(CC) -o tanx.cgi $(OBJECTS) $(LIBS) -O2
main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tanx_adapter.o:
	$(CC) -c tanx_adapter.cpp $(CPPFLAGS) $(PKGCONFIG) -O2 
tanx-bidding.pb.o:
	$(CC) -c tanx-bidding.pb.cc $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	$(CC) -c ../../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0`
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG)
util.o:
	$(CC) -c ../../common/util.cpp $(CPPFLAGS) $(PKGCONFIG)
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json.o:
	gcc -c ../../common/json.c $(CPPFLAGS) $(PKGCONFIG) -O2
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/odin_$(adx)_mini
.PHONY: clean
clean:
	rm $(OBJECTS)
#end of file
