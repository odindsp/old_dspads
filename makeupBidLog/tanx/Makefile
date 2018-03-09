CC = g++
adx=tanx
OBJECTS = selfsemphore.o	\
	  confoperation.o	\
	  bid_filter_type.o	\
	  bid_filter_dump.o		\
	  redisoperation.o      \
	  getlocation.o         \
	  json.o		\
	  util.o		\
	  json_util.o		\
	  setlog.o		\
	  tcp.o			\
	  bid_filter.o	        \
	  tanx_adapter.o	\
	  url_endecode.o	\
	  main.o                \
          DecodeDeviceId.o	\
          tanx-bidding.pb.o	\
		  geohash.o

LIBS= -lfcgi -lcrypto -lhiredis -pthread -lprotobuf -lrt `pkg-config --libs glib-2.0`

tanx.cgi : $(OBJECTS)
	$(CC) -o tanx.cgi $(OBJECTS) $(LIBS) -O2

main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tanx_adapter.o:
	$(CC) -c ../../bid/tanx/tanx_adapter.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
bid_filter.o:
	$(CC) -c ../../common/bid_filter.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
bid_filter_type.o:
	 $(CC) -c ../../common/bid_filter_type.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
bid_filter_dump.o:
	 $(CC) -c ../../common/bid_filter_dump.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
redisoperation.o:
	$(CC) -c ../../common/redisoperation.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
selfsemphore.o:
	$(CC) -c ../../common/selfsemphore.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
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
getlocation.o:
	$(CC) -c ../../common/getlocation.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tanx-bidding.pb.o:
	$(CC) -c ../../bid/tanx/tanx-bidding.pb.cc $(CPPFLAGS) $(PKGCONFIG) -O2
DecodeDeviceId.o:
	$(CC) -c ../../common/tanx/DecodeDeviceId.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
geohash.o:
	$(CC) -c ../../common/geohash.cpp $(CPPFLAGS) $(PKGCONFIG) -O2

.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/odin_$(adx)	
.PHONY: clean
clean:
	rm  $(OBJECTS)

#end of file
