======================server2.0======================
gcc tcp_server_2.c -o tcp_server2.0.out -lpthread
./tcp_server2.0.out

This class has not been tested thoroughly.

======================server side======================
gcc tcp_server.c -o tcp_server.out && ./tcp_server.out

======================client side======================
gcc tcp_client.c -o tcp_client.out && ./tcp_client.out
