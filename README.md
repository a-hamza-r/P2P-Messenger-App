# P2P-Messenger-App
A command line based messenger app where you can add friends and chat with friends 

Functionality implemented: 
1) server initialization with configuration file
2) user registration "r"
3) login "l"
4) quit the program "exit" 
5) logout from the server "logout"
6) client invitation command "i"
7) client invitation acceptance command
8) handling SIGINT signal
9) readme file

The server is in the messenger_server folder. Client is in the messenger_client folder. 
to run the server, compile using the command "g++ -o server messenger_server.cpp" in the respective folder. To run the client, use the command "g++ -o client -pthread messenger_client.cpp" in the respective folder. I have implemented POSIX threads at the client side while I/O multiplexing with "select" at the server side. The server and client only work at localhost. The reason is that I could not find the global IP addresses of server or client. Hence, server is not available globally and outside entity cannot connect. The reason might be NAT settings. 

In case the client side does not show any prompt on the terminal to follow, it is still implemented properly. So, if you type any supported commands like "i" for invitation, "ia" for accepting, it will work properly. I have implemented the "i" (to invite someone) command such that you first type "i" and then type the "friend username" and then the message. Same case for "ia" command. 