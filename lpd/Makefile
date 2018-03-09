CC = g++

OBJECTS = \
	  setlog.o		\
	  tcp.o			\
	  json.o		\
	  json_util.o		\
	  util.o		\
	  lpd.o		\
	  confoperation.o	\
	  url_endecode.o  \
	  main.o

LIBS= -lfcgi -lcrypto -lhiredis -pthread -ldl `pkg-config --libs glib-2.0` 

lpd : $(OBJECTS)
	$(CC) -o lpd.cgi $(CPPFLAGS) $(OBJECTS) $(LIBS) -O2
main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
lpd.o:
	$(CC) -c lpd.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tcp.o:
	$(CC) -c ../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
util.o:
	$(CC) -c ../common/util.cpp $(CPPFLAGS) $(PKGOCNFIG) -O2
json_util.o:
	$(CC) -c ../common/json_util.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
json.o:
	gcc -c ../common/json.c $(CPPFLAGS) $(PKGCONFIG) -O2
url_endecode.o:
	$(CC) -c ../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	$(CC) -c ../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` -O2 
	
.PHONY: install
install:
	mv -f lpd.cgi /usr/bin/odin_lpd

.PHONY: clean
clean:
	rm lpd.cgi $(OBJECTS)

#end of file

