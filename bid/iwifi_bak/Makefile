CPPFLAGS += -DNDEBUG
CFLAGS = -O2
CXXFLAGS = $(CFLAGS)
CC = g++
adx = iwifi
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
		main.o				\
		iwifi_response.o	\

		
LIBS= -lfcgi -lcrypto -lhiredis -pthread -lrt `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) $(CXXFLAGS)

main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
iwifi_response.o:
	$(CC) -c iwifi_response.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
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
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
getlocation.o:
	$(CC) -c ../../common/getlocation.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/odin_$(adx)
.PHONY: clean
clean:
	rm $(adx).cgi $(OBJECTS)

#end of file
