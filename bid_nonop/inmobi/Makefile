#If you want make debug version, you must add the param to "make", the example as follow:
#
#    make BUILD=-DDEBUG SOFLAG=-lbidfilter_d
#	 make "CPPFLAGS += -DDEBUG"
#
#if don't need debug version,just release version, then just use make

CC = g++
adx = inmobi
prefix = odin

OBJECTS = tcp.o			\
	  json_util.o		\
	  json.o		\
	  setlog.o		\
	  $(adx)_response.o	\
	  main.o                \
	  util.o                \
	  confoperation.o		\
	  	  	  url_endecode.o	\
	  bid_filter_type.o

LIBS= -lrt -lfcgi -lcrypto -lhiredis `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) -O2
main.o:
	$(CC) -c main.cpp $(PKGCONFIG) -O2
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(PKGCONFIG) $(BUILD) -O2
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(PKGCONFIG) $(BUILD) -O2
util.o:
	$(CC) -c ../../common/util.cpp $(PKGCONFIG) $(BUILD) -O2
json.o:
	gcc -c ../../common/json.c $(PKGCONFIG) -O2
bid_filter_type.o:
	$(CC) -c ../../common/bid_filter_type.cpp $(PKGCONFIG) $(BUILD) -O2
confoperation.o:
	$(CC) -c ../../common/confoperation.cpp $(PKGCONFIG) $(BUILD) `pkg-config --cflags glib-2.0` -O2
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
$(adx)_response.o:
	$(CC) -c $(adx)_response.cpp $(PKGCONFIG) -O2
.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/$(prefix)_$(adx)_mini
.PHONY: clean
clean:
	rm -f $(OBJECTS)

#end of file
