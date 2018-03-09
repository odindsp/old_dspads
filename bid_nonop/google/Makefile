CC = g++
adx=google
OBJECTS = realtime-bidding.pb.o	\
	  $(adx)_response.o	\
          main.o                \
          confoperation.o       \
          json_util.o		\
	  json.o		\
          util.o                \
	  url_endecode.o	\
          setlog.o              \
          tcp.o

LIBS= -lrt -lcrypto -lfcgi  -pthread -lprotobuf `pkg-config --libs glib-2.0` -lhiredis
$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) -O2
main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
$(adx)_response.o:
	$(CC) -c $(adx)_response.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
realtime-bidding.pb.o:
	$(CC) -c realtime-bidding.pb.cc $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	$(CC) -c ../../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0`
util.o:
	$(CC) -c ../../common/util.cpp $(CPPFLAGS) $(PKGCONFIG)
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json.o:
	gcc -c ../../common/json.c $(CPPFLAGS) $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG)
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/odin_$(adx)_mini
.PHONY: clean
clean:
	rm $(OBJECTS)
#end of file
