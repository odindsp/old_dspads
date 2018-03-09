CC = g++
adx=momo
OBJECTS = confoperation.o	\
	  json.o		\
	  util.o		\
	  json_util.o		\
	  setlog.o		\
	  tcp.o			\
	  $(adx)_adapter.o	\
	  url_endecode.o	\
	  main.o                \
          momortb12.pb.o	\


LIBS= -lfcgi -lcrypto -lhiredis -pthread -lprotobuf -lrt `pkg-config --libs glib-2.0`

$(adx).cgi : $(OBJECTS)
	$(CC) -o $(adx).cgi $(OBJECTS) $(LIBS) -O2

main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
$(adx)_adapter.o:
	$(CC) -c $(adx)_adapter.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
confoperation.o:
	 $(CC) -c ../../common/confoperation.cpp $(CPPFLAGS) $(PKGCONFIG) `pkg-config --cflags glib-2.0` -O2
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
momortb12.pb.o:
	$(CC) -c momortb12.pb.cc $(CPPFLAGS) $(PKGCONFIG) -O2
setlog.o:
	$(CC) -c ../../common/setlog.cpp $(CPPFLAGS) $(PKGCONFIG)

.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/odin_$(adx)_mini
.PHONY: clean
clean:
	rm  $(OBJECTS)

#end of file
