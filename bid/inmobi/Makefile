#If you want make debug version, you must add the param to "make", the example as follow:
#
#    make BUILD=-DDEBUG SOFLAG=-lbidfilter_d
#	 make "CPPFLAGS += -DDEBUG"
#
#if don't need debug version,just release version, then just use make

CPPFLAGS += -DNDEBUG -DDETAILLOG
CFLAGS = -O2
CXXFLAGS = $(CFLAGS)
CC = g++
adx = inmobi
prefix = odin

OBJECTS = selfsemphore.o	\
		confoperation.o		\
		bid_filter_type.o	\
		bid_filter_dump.o	\
		redisoperation.o	\
		getlocation.o		\
		json.o				\
		util.o				\
		json_util.o			\
		setlog.o			\
		tcp.o				\
		bid_filter.o		\
		filter_by_deviceid_wblist.o	\
		url_endecode.o		\
		geohash.o			\
		server_log.o		\
		bid_flow_detail.o	\
		main.o				\
		$(adx)_response.o	\
		rdkafka_operator.o	\

		
LIBS= -lfcgi -lcrypto -lhiredis -pthread -lrt -lrdkafka -lrdkafka++ -lz `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) $(CXXFLAGS)

main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
$(adx)_response.o:
	$(CC) -c $(adx)_response.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
geohash.o:
	$(CC) -c ../../common/geohash.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
filter_by_deviceid_wblist.o:
	$(CC) -c ../../common/filter_by_deviceid_wblist.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
bid_filter.o:
	$(CC) -c ../../common/bid_filter.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
bid_filter_type.o:
	 $(CC) -c ../../common/bid_filter_type.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
bid_filter_dump.o:
	 $(CC) -c ../../common/bid_filter_dump.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
redisoperation.o:
	$(CC) -c ../../common/redisoperation.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
selfsemphore.o:
	$(CC) -c ../../common/selfsemphore.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
confoperation.o:
	 $(CC) -c ../../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` $(CXXFLAGS)
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
util.o:
	$(CC) -c ../../common/util.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
json.o:
	gcc -c ../../common/json.c $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
rdkafka_operator.o:
	$(CC) -c ../../common/rdkafka_operator.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
server_log.o:
	$(CC) -c ../../common/server_log.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
bid_flow_detail.o:
	$(CC) -c ../../common/bid_flow_detail.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
getlocation.o:
	$(CC) -c ../../common/getlocation.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
	
.PHONY: install
install:
	mv -f $(adx).cgi /usr/bin/$(prefix)_$(adx)
.PHONY: clean
clean:
	rm -f $(adx).cgi $(OBJECTS)

#end of file
