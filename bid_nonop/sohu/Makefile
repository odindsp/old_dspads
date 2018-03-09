#If you want make debug version, you must add the param to "make", the example as follow:
#
#    make BUILD=-DDEBUG SOFLAG=-lbidfilter_d
#
#if don't need debug version,just release version, then just use make

CC = g++
adx = sohu

OBJECTS = confoperation.o	\
	  $(adx)_response.o	\
	  sohuRTB.pb.o		\
	  util.o            \
	  json_util.o		\
	  json.o		\
	  setlog.o              \
	  main.o \
	  url_endecode.o	\
	  tcp.o

LIBS= -lfcgi -lcrypto -lhiredis -pthread -lprotobuf -lrt `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) -O2

main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
$(adx)_response.o:
	$(CC) -c $(adx)_response.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	 $(CC) -c ../../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` -O2
sohuRTB.pb.o:
	$(CC) -c sohuRTB.pb.cc $(CPPFLAGS) $(PKGCONFIG) -O2
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json_util.o:
	$(CC) -c ../../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json.o:
	gcc -c ../../common/json.c $(CPPFLAGS) $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG)
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
util.o:
	$(CC) -c ../../common/util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
	
.PHONY: install
install:
	mv -f $(adx).cgi /usr/bin/odin_$(adx)_mini
.PHONY: clean
clean:
	rm -f $(OBJECTS)

#end of file
