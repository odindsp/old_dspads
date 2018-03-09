CC = g++
NAME = odin_server
LIBS= -lpthread `pkg-config --libs glib-2.0` `pkg-config --cflags glib-2.0`

$(NAME):
	$(CC) server.cpp common/confoperation.cpp common/writelog.cpp -o $(NAME) $(LIBS) -O2
.PHONY: clean
clean:
	-rm $(NAME)  *.log 
