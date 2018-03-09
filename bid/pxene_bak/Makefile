CPPFLAGS += -DNDEBUG
CFLAGS = -O2
CXXFLAGS = $(CFLAGS)
CC = g++
adx=pxene
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
		pxene_response.o	\

		
LIBS= -lbidfilter -lfcgi -lcrypto -lhiredis -pthread `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) $(CXXFLAGS)

main.o:
	$(CC) -c main.cpp $(PKGCONFIG) $(CXXFLAGS)
filter_by_deviceid_wblist.o:
	$(CC) -c ../../common/filter_by_deviceid_wblist.cpp $(CPPFLAGS) $(PKGCONFIG) $(CXXFLAGS)
pxene_response.o:
	$(CC) -c pxene_response.cpp $(PKGCONFIG) $(CXXFLAGS)
confoperation.o:
	$(CC) -c ../../common/confoperation.cpp $(PKGCONFIG)  `pkg-config --cflags glib-2.0` $(CXXFLAGS)
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(PKGCONFIG) $(CXXFLAGS)
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(PKGCONFIG) $(CXXFLAGS)
util.o:
	$(CC) -c ../../common/util.cpp $(PKGCONFIG) $(CXXFLAGS)
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(PKGCONFIG) $(CXXFLAGS)
json.o:
	gcc -c ../../common/json.c $(PKGCONFIG) $(CXXFLAGS)
.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/dsp_$(adx)
.PHONY: clean
clean:
	rm -f $(OBJECTS)

#end of file
