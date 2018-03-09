CC = g++
NAME = odin_server
LIBS= -lpthread -lcrypto -lssl -I/app/hadoop-2.5.2/include/ -lhdfs `pkg-config --libs glib-2.0` `pkg-config --cflags glib-2.0`

$(NAME):
	$(CC) server.cpp ../common/common.cpp ../common/confoperation.cpp ../common/writelog.cpp $(CPPFLAGS) -o $(NAME) $(LIBS) -O2
.PHONY: clean
clean:
	-rm $(NAME)  *.gz *.log *.tmp
