#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <cstring>

using namespace std;

#define BUFSIZE 50
#define PACKETSIZE sizeof(MSG)
#define NUMMEMBERSMSG 4

typedef struct MSG {
	int l_username;
	int l_password;
	int l_command;
	int l_status;
    char username[BUFSIZE];
    char password[BUFSIZE];
    char command[BUFSIZE];
    char status[BUFSIZE];

    MSG(char *command, char *username, char *password, char *status) {
    	strcpy(this->username, username);
		strcpy(this->password, password);
		strcpy(this->command, command);
		strcpy(this->status, status);
		this->l_username = strlen(username);
		this->l_password = strlen(password);
		this->l_command = strlen(command);
		this->l_status = strlen(status);
    }
    MSG(string command, string username, string password, string status, int a) {
    	strcpy(this->username, username.c_str());
		strcpy(this->password, password.c_str());
		strcpy(this->command, command.c_str());
		strcpy(this->status, status.c_str());
		this->l_username = username.length();
		this->l_password = password.length();
		this->l_command = command.length();
		this->l_status = status.length();
    }
    MSG() {}
}MSG;

typedef struct Location {
	struct sockaddr_in addr;

	Location(struct sockaddr_in addr) : addr(addr) {}
	Location() {}
}Loc;

typedef struct args {
	int port;
	int serv_sock;
	char logged_in_user[BUFSIZE];
}args;

map<string, string> config_info;
map<string, Loc> online_friends;

void serialize(MSG* msg, char *data)
{
	char str[NUMMEMBERSMSG*BUFSIZE];
	strcpy(str, msg->command);
	strcat(str, msg->username);
	strcat(str, msg->password);
	strcat(str, msg->status);

	int *q = (int*)data;    
    *q = msg->l_command; q++;
    *q = msg->l_username; q++;    
    *q = msg->l_password; q++;    
    *q = msg->l_status; q++;    

    char *p = (char*)q;
    int i = 0;
    while (i < NUMMEMBERSMSG*BUFSIZE)
    {
        *p = str[i];
        p++; i++;
    }
}

void deserialize(char *data, MSG* msg, char *arr)
{
	int *q = (int*)data;    
    msg->l_command = *q; q++;    
    msg->l_username = *q; q++;    
    msg->l_password = *q; q++;
    msg->l_status = *q; q++;

    char *p = (char*)q;
    int i = 0;
    while (i < NUMMEMBERSMSG*BUFSIZE)
    {
        arr[i] = *p;
        p++; i++;
    }
    memcpy(msg->command, &arr[0], msg->l_command);
    msg->command[msg->l_command] = '\0';

    memcpy(msg->username, &arr[msg->l_command], msg->l_username);
    msg->username[msg->l_username] = '\0';

    memcpy(msg->password, &arr[msg->l_command+msg->l_username], msg->l_password);
    msg->password[msg->l_password] = '\0';

    memcpy(msg->status, &arr[msg->l_command+msg->l_username+msg->l_password], msg->l_status);
    msg->status[msg->l_status] = '\0';
}


int parse_string(string line, string delim, vector<string> *vec) {
	size_t pos = 0;
	string token;
	int i = 0;
	while ((pos = line.find(delim)) != string::npos) {
		token = line.substr(0, pos);
		vec->push_back(token);
		i++;
		line.erase(0, pos+delim.length());
	}
	vec->push_back(line);
	return ++i;
}

void read_configuration_info(const char *config_info_file) {
	int len = 0, i = 0;
	string line;
	ifstream file;
	file.open(config_info_file);
	vector<string> vec;
	while (getline(file, line)) {
		parse_string(line, ": ", &vec);
		config_info.insert({vec[i], vec[i+1]});
		i += 2;
	}
	file.close();
}

void *process_connection(void *arg) {
    ssize_t n;
    char buf[BUFSIZE];

    int sockfd = *((int *)arg);
    free(arg);
	MSG* res = new MSG;
	char arr[PACKETSIZE];

    pthread_detach(pthread_self());
    
    n = read(sockfd, buf, BUFSIZE);
    if (n != 0) {
		deserialize(buf, res, arr);
		cout << res->username << " >> " << res->status << endl;
    }
    
    close(sockfd);
    return(NULL);
}

void* handle_peers(void* arg) {
	struct sockaddr_in addr, recv_addr;
	int sock_fd, client_sock;
	socklen_t length;
	args* args_ = (args *)arg;
	int port = args_->port;
	int serv_sock = args_->serv_sock;
	char logged_in_user[BUFSIZE]; 
	strcpy(logged_in_user, args_->logged_in_user);
	free(arg);
	char service[20];
	pthread_t tid;

    pthread_detach(pthread_self());

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Can not get the socket");
		return(NULL);
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;

	addr.sin_port = htons((short)port);

	length = sizeof(addr);

	if (bind(sock_fd, (struct sockaddr *)&addr, length) < 0) {
		perror(": bind");
		return(NULL);
	}

	if (getsockname(sock_fd, (struct sockaddr *)&addr, (socklen_t *)&length) < 0) {
		perror(": can't get name");
		return(NULL);
	}

	char data[PACKETSIZE];
	MSG* res = new MSG("save_loc", logged_in_user, "127.0.0.1", to_string(port), 1);

	serialize(res, data);
	// cout << port << endl;
	// cout << ntohs(addr.sin_port) << endl;
	write(serv_sock, data, PACKETSIZE);

	printf("\nListening at ip = %s, port = %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	if (listen(sock_fd, 50) < 0) {
		perror(": bind");
		return(NULL);
	}

	length = sizeof(recv_addr);

	while (1) {
		if ((client_sock = accept(sock_fd, (struct sockaddr *)&recv_addr, &length)) < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				perror(": accept");
				return(NULL);
			}
		}

		int *sock_ptr = new int;
		*sock_ptr = client_sock;

	    if (pthread_create(&tid, NULL, &process_connection, (void*) sock_ptr) != 0) {
	    	perror(": pthread");
	    	continue;
	    }
	}
}

void* handle_invitations(void* arg) {
		int sock_fd = *((int *)arg);
		free(arg);
	    pthread_detach(pthread_self());
	    char data[PACKETSIZE], arr[NUMMEMBERSMSG*BUFSIZE];
	    MSG* res = new MSG;
		
	while (1) {
		int read_num = read(sock_fd, data, PACKETSIZE);
		if (read_num != 0) {
			deserialize(data, res, arr);
			if (strcmp(res->command, "i") == 0) {
				cout << "\n\nYou got a friend invite from " << res->username << endl;
				cout << "Message: " << res->status << endl;
				cout << "Type ia and enter to accept: ";
			} else if (strcmp(res->command, "ia") == 0) {
				cout << "\n\nYour invite got accepted by " << res->username << ".\n";
				cout << "Message: " << res->status << endl;
				char username[BUFSIZE];
				strcpy(username, res->username);

				struct sockaddr_in sa;

				read_num = read(sock_fd, data, PACKETSIZE);
				res = new MSG;
				deserialize(data, res, arr);
				inet_pton(AF_INET, res->password, &(sa.sin_addr));
				sa.sin_port = htons((short)stoi(res->status));
				Loc* l = new Loc(sa);
				map<string, Loc>::iterator i = online_friends.find(username);
				if (i == online_friends.end()) {
					online_friends.insert({username, *l});
				}
				// cout << "size: " << online_friends.size() << endl; 
			}
		}
	}
}

void handle_con_server(int sock_fd) {
	char username[BUFSIZE], password[BUFSIZE], buf[BUFSIZE], status[BUFSIZE];
	char data[PACKETSIZE], arr[NUMMEMBERSMSG*BUFSIZE], data1[PACKETSIZE], arr1[NUMMEMBERSMSG*BUFSIZE];
	int read_num;
	bool registered = false, logged_in = false;
	pthread_t tid;
	char logged_in_user[BUFSIZE];
	printf("Type h for available options\n");

	while (fgets(buf, BUFSIZE, stdin)) {
		buf[strlen(buf)-1] = '\0';
		if (strcmp(buf, "r") == 0) {
			printf("Enter a username: ");
			fgets(username, BUFSIZE, stdin);
			username[strlen(username)-1] = '\0';
			printf("Enter a password: ");
			fgets(password, BUFSIZE, stdin);
			// getline(cin, password);
			password[strlen(password)-1] = '\0';
			strcpy(status, "no");
			MSG* reg = new MSG(buf, username, password, status);

			serialize(reg, data);

			write(sock_fd, data, PACKETSIZE);
			
			read_num = read(sock_fd, data, PACKETSIZE);
			if (read_num == 0) {
				cout << "nothing read\n";
			} else {
				char arr[NUMMEMBERSMSG*BUFSIZE];
				MSG* r = new MSG;
				deserialize(data, r, arr);
				if (strcmp(r->status, "200") == 0) {
					registered = true;
					cout << "Registration successful. Username: " << r->username << endl;
				} else if (strcmp(r->status, "500") == 0) {
					registered = false;
					cout << "Registration unsuccessful. Try again with another username\n";
				}
			}
		} else if (strcmp(buf, "l") == 0) {
			if (!registered) {
				cout << "Not registered. Try registering first\n";
				continue;
			} 
			if (logged_in) {
				cout << "Already logged in.\n";
				continue;
			}
			printf("Enter a username: ");
			fgets(username, BUFSIZE, stdin);
			username[strlen(username)-1] = '\0';

			printf("Enter a password: ");
			fgets(password, BUFSIZE, stdin);
			password[strlen(password)-1] = '\0';

			strcpy(status, "no");
			MSG* reg = new MSG(buf, username, password, status);

			serialize(reg, data);

			write(sock_fd, data, PACKETSIZE);
			read_num = read(sock_fd, data1, PACKETSIZE);
			if (read_num == 0) {
				cout << "nothing read\n";
			} else {
				MSG* r = new MSG;
				deserialize(data1, r, arr1);
				if (strcmp(r->status, "200") == 0) {
					logged_in = true;
					cout << "Login successful. Username: " << r->username << endl;
					strcpy(logged_in_user, r->username);

					char ports[10];
					int port;
					cout << "\n\nEnter the port number client listens on. Just press enter for default (5100): ";
					fgets(ports, 10, stdin);
					ports[strlen(ports)-1] = '\0';
					if (strlen(ports) == 0) {
						port = 5100;
					} else {
						port = stoi(ports);
					}

					args* pthread_args = new args;
					pthread_args->port = port;
					pthread_args->serv_sock = sock_fd;
					strcpy(pthread_args->logged_in_user, logged_in_user);

					if (pthread_create(&tid, NULL, &handle_peers, (void *)pthread_args) != 0) {
						perror(": pthread");
						return;
					}

					int *sock = new int;
					*sock = sock_fd;

					if (pthread_create(&tid, NULL, &handle_invitations, (void *)sock) != 0) {
						perror(": pthread");
						return;
					}
				
				} else if (strcmp(r->status, "500") == 0) {
					logged_in = false;
					cout << "Login unsuccessful. Try again.\n";
				}
			}

		} else if (strcmp(buf, "exit") == 0) {
			MSG* reg = new MSG(string(buf), logged_in_user, "", "", 1);

			serialize(reg, data);

			write(sock_fd, data, PACKETSIZE);
			read_num = read(sock_fd, data1, PACKETSIZE);
			reg = new MSG;
			deserialize(data1, reg, arr);
			if (strcmp(reg->status, "200") == 0)
			{
				logged_in = false;
				strcpy(logged_in_user, "");
				cout << "successfully logged out \n";
			}
			return;
		} else if (strcmp(buf, "i") == 0) {
			if (!logged_in) {
				cout << "Login first.\n";
				continue;
			}
			cout << "Enter friend username: ";
			fgets(username, BUFSIZE, stdin);
			username[strlen(username)-1] = '\0';
			cout << "Enter message to include(optional): ";
			fgets(status, BUFSIZE, stdin);
			status[strlen(status)-1] = '\0';
			char dat[PACKETSIZE];
			map<string, Loc>::iterator i = online_friends.find(string(username));
			if (i != online_friends.end()) {
				cout << "\nYou are already friends with " << username << endl;
			} else {
				MSG* res = new MSG(buf, username, logged_in_user, status);
				serialize(res, dat);

				write(sock_fd, dat, PACKETSIZE);
			}

		} else if (strcmp(buf, "ia") == 0) {
			if (!logged_in) {
				cout << "Login first.\n";
				continue;
			}
			cout << "Enter the username of the friend you want to accept invitation for: ";
			fgets(username, BUFSIZE, stdin);
			username[strlen(username)-1] = '\0';
			cout << "\nEnter message to include(optional): ";
			fgets(status, BUFSIZE, stdin);
			status[strlen(status)-1] = '\0';
			char dat[PACKETSIZE];
			MSG* res = new MSG(buf, username, logged_in_user, status);
			serialize(res, dat);

			write(sock_fd, dat, PACKETSIZE);
			res = new MSG;
			int read_num = read(sock_fd, dat, PACKETSIZE);
			struct sockaddr_in sa;
			if (read_num != 0) {
				deserialize(dat, res, arr);
				// cout << res->password << " hello " << res->status << endl;
				inet_pton(AF_INET, res->password, &(sa.sin_addr));
				sa.sin_port = htons((short)stoi(res->status));
				Loc* l = new Loc(sa);
				map<string, Loc>::iterator i = online_friends.find(username);
				if (i == online_friends.end()) {
					online_friends.insert({username, *l});
				}
			}
		} else if (strcmp(buf, "h") == 0) {
			printf("\n\nFollowing options available: \n1) Register with the server: enter r\n");
			printf("2) Log into the server: enter l\n3) Exit the client program: enter exit\n");
			printf("4) Logout of the server: enter logout\n");
			printf("4) Invite a friend to chat (only after logging in): enter i\n");
			printf("5) Accept invitation of a friend (only after logging in): enter ia\n");
			printf("Press enter after the input\n");	
		} else if (strcmp(buf, "logout") == 0) {
			MSG* reg = new MSG(string(buf), logged_in_user, "", "", 1);

			serialize(reg, data);

			write(sock_fd, data, PACKETSIZE);
			read_num = read(sock_fd, data1, PACKETSIZE);
			reg = new MSG;
			deserialize(data1, reg, arr);
			if (strcmp(reg->status, "200") == 0)
			{
				logged_in = false;
				strcpy(logged_in_user, "");
				cout << "successfully logged out \n";
			}
		}
		else {
			printf("Invalid input\n");
		}

		printf("Type h for available options\n");

	}

	return;
}

void client() {
	struct sockaddr_in addr;
	int sock_fd;
	pthread_t tid;
	
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror(": Can't get socket");
		return;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(config_info.at("servhost").c_str());
	addr.sin_port = htons((short)stoi(config_info.at("servport")));

	if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror(": connect");
		return;
	}

	handle_con_server(sock_fd);
}

int main(int argc, char const *argv[])
{
	if (argc < 2) {
		cout << "Usage: messenger_client configuration_file\n";
		exit(0);
	}

	read_configuration_info(argv[1]);
	client();
	// cout << "servport: " << config_info.at("servport") << endl;
	// cout << "servhost: " << config_info.at("servhost") << endl;

	return 0;
}