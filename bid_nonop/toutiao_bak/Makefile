CC = g++
adx=toutiao
OBJECTS = toutiao_ssp_api.pb.o	\
	  toutiao_adapter.o	\
          main.o                \
          confoperation.o       \
          json_util.o		\
	  json.o		\
          util.o                \
          setlog.o              \
		  	  	  url_endecode.o	\
          tcp.o

LIBS= -lrt -lfcgi  -pthread -lprotobuf `pkg-config --libs glib-2.0` -lhiredis
$(adx).cgi : $(OBJECTS)
	$(CC) -o toutiao.cgi $(OBJECTS) $(LIBS) -O2
main.o:
	$(CC) -c main.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
toutiao_adapter.o:
	$(CC) -c toutiao_adapter.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
toutiao_ssp_api.pb.o:
	$(CC) -c toutiao_ssp_api.pb.cc $(CPPFLAGS) $(PKGCONFIG) -O2
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
url_endecode.o:
	$(CC) -c ../../common/url_endecode.cpp $(CPPFLAGS) $(PKGCONFIG) -O2
tcp.o:
	$(CC) -c ../../common/tcp.cpp $(CPPFLAGS) $(PKGCONFIG) -O2

.PHONY: install
install:
	mv -f  $(adx).cgi /usr/bin/dsp_$(adx)_mini
.PHONY: clean
clean:
	rm $(OBJECTS)
#end of file
