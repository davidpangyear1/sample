======================client2.0======================
gcc tcp_client2.0.c -o tcp_client2.0.out && ./tcp_client2.0.out

======================server2.0======================
gcc tcp_server2.0.c -o tcp_server2.0.out -lpthread && ./tcp_server2.0.out

======================server side======================
gcc tcp_server.c -o tcp_server.out && ./tcp_server.out

======================client side======================
gcc tcp_client.c -o tcp_client.out && ./tcp_client.out
