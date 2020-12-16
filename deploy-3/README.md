# Description:
Program that simulates a server-client interaction in which the server holds a file system and the client makes requests to it using a socket. <br />
The server has multiple threads of execution using fine locking and can hold more than 1000 threads. <br />
Sockets are created in /tmp directory

## How to run
Execute the following command:
```
make
./tecnicofs <number_of_threads> <socket_name>
cd client
make
./tecnicofs-client <inputfile> <server_socket_path>
```

