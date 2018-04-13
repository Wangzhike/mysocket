.PHONY: all
all: tcpcli tcpserv

tcpcli: tcpcli.o str_cli.o ./rdwt/readn.o ./rdwt/writen.o ./err_handlers/err_handlers.o
	cc -o tcpcli tcpcli.o str_cli.o ./rdwt/readn.o ./rdwt/writen.o ./err_handlers/err_handlers.o

tcpserv: tcpserv.o str_echo.o ./rdwt/readn.o ./rdwt/writen.o ./err_handlers/err_handlers.o signal.o
	cc -o tcpserv tcpserv.o str_echo.o ./rdwt/readn.o ./rdwt/writen.o ./err_handlers/err_handlers.o signal.o

tcpcli.o: tcpcli.c myecho.h

tcpserv.o: tcpserv.c myecho.h

str_cli.o: str_cli.c myecho.h

str_echo.o: str_echo.c myecho.h

writen.o: ./rdwt/writen.c myecho.h

readn.o: ./rdwt/readn.c myecho.h

err_handlers.o: ./err_handlers/err_handlers.c

signal.o: signal.c myecho.h

.PHONY: clean
clean:
	rm -rf *.o ./rdwt/*.o ./err_handlers/err_handlers.o
