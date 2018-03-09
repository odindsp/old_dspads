CC = g++
NAME = odin_client_16
LIBS= -lpthread -lcrypto -lssl `pkg-config --libs glib-2.0` `pkg-config --cflags glib-2.0`

$(NAME):
	$(CC) client.cpp ../common/common.cpp ../common/confoperation.cpp ../common/writelog.cpp ../common/selfsemphore.cpp $(CPPFLAGS) -o $(NAME) $(LIBS) -O2
.PHONY: clean
clean:
	-rm $(NAME) *.log *.ini 
