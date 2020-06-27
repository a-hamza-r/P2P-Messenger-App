#define _POSIX_SOURCE
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <string.h>
#include <ifaddrs.h>
#include <signal.h>
#include <fstream>

#define MAXCONN 10
#define PACKETSIZE sizeof(MSG)
#define BUFSIZE 50
#define NUMMEMBERSMSG 4

using namespace std;

typedef struct One_User {
	string username;
	string password;
	vector<string> friends;
}One_User;

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

map<string, string> config_info;
map<string, One_User> users_info;
map<string, Loc> online_users;
map<string, int> online_sockets;

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

void register_user(int client_sock, string username, string password) {
	char status[BUFSIZE], data[PACKETSIZE], arr[NUMMEMBERSMSG*BUFSIZE];
	map<string, One_User>::iterator it = users_info.find(username);
	if (it == users_info.end()) {
		One_User user;
		user.username = username;
		user.password = password;
		users_info.insert({username, user});
		cout << "successfully registered " << username << endl;
		MSG* msg = new MSG("", username, "", "200", 1);
		serialize(msg, data);
		write(client_sock, data, PACKETSIZE);
	} else {
		MSG* msg = new MSG("", "", "", "500", 1);
		serialize(msg, data);
		write(client_sock, data, PACKETSIZE);
		cout << "Already a user by that name\n";
	}
}

void login_user(int client_sock, string username, string password) {
	bool logged_in = false;
	// struct sockaddr_in addr;
	char status[BUFSIZE], data[PACKETSIZE], arr[NUMMEMBERSMSG*BUFSIZE];
	map<string, One_User>::iterator it = users_info.find(username);
	if (it != users_info.end()) {
		if (it->second.password == password) {
			logged_in = true;
		}
	}
	if (logged_in) {
		cout << "successfully logged in " << username << endl;

		map<string, int>::iterator i = online_sockets.find(username);
		if (i != online_sockets.end()) {
			// cout << "Already a username in online_sockets " << 
			i->second = client_sock;
		} else {
			online_sockets.insert({username, client_sock});
		}

		MSG* msg = new MSG("", username, "", "200", 1);
		serialize(msg, data);
		write(client_sock, data, PACKETSIZE);
	} else {
		cout << "Unsuccessful login\n";
		MSG* msg = new MSG("", "", "", "500", 1);
		serialize(msg, data);
		write(client_sock, data, PACKETSIZE);
	}
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

void read_user_info(const char *user_info_file) {
	string line;
	ifstream file;
	file.open(user_info_file);
	One_User user;
	int len;
	while (getline(file, line)) {
		vector<string> vec, vec1;
		len = parse_string(line, "|", &vec);
		user.username = vec[0];
		user.password = vec[1];
		// cout << "username: " << user.username << " \npassword: " << user.password << endl;
		len = parse_string(line, ";", &vec1);
		// cout << "friends: ";
		for (int i = 0; i < len; i++) {
			user.friends.push_back(vec1[i]);
			// cout << vec1[i] << " ";
		}
		users_info.insert({user.username, user});
	}
	// cout << "\n\n";
	file.close();
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

void delete_online_user(string username) {
	map<string, Loc>::iterator it = online_users.find(username);
	if (it != online_users.end()) {
		online_users.erase(it);
		cout << "user disconnected " << username << endl;
	} else {
		cout << "Error removing the user: already disconnected " << username << endl;
	}
}

void handle_single_req(int server_sock, int client_sock, char *req) {
	char arr[NUMMEMBERSMSG*BUFSIZE], data[PACKETSIZE];
	MSG* msg = new MSG;
	int read_num;
	deserialize(req, msg, arr);
	
	string username(msg->username);
	string password(msg->password);

	if (strcmp(msg->command, "r") == 0) {
		register_user(client_sock, username, password);
	} else if (strcmp(msg->command, "l") == 0) {
		login_user(client_sock, username, password);
	} else if (strcmp(msg->command, "save_loc") == 0) {
		struct sockaddr_in sa;

		inet_pton(AF_INET, msg->password, &(sa.sin_addr));
		sa.sin_port = htons((short)stoi(msg->status));
		Loc* l = new Loc(sa);
		map<string, Loc>::iterator i = online_users.find(string(msg->username));
		if (i != online_users.end()) {
			i->second = *l;
		} else {
			online_users.insert({string(msg->username), *l});
		}
	} else if (strcmp(msg->command, "i") == 0) {
		string invite_from = string(msg->password);
		string invite_to = string(msg->username);
		string message_invite = string(msg->status);
		map<string, int>::iterator i = online_sockets.find(invite_to);
		if (i != online_sockets.end()) {
			MSG* res = new MSG("i", invite_from, "", message_invite, 1);
			serialize(res, data);
			write(i->second, data, PACKETSIZE);
		}
	} else if (strcmp(msg->command, "ia") == 0) {
		char str[INET_ADDRSTRLEN];
		string invite_accept_from = string(msg->password);
		string message_invite_accept = string(msg->status);
		string invite_accept_sending_to = string(msg->username);
		map<string, int>::iterator i = online_sockets.find(invite_accept_sending_to);
		if (i != online_sockets.end()) {
			MSG* res = new MSG("ia", invite_accept_from, "", message_invite_accept, 1);
			serialize(res, data);
			write(i->second, data, PACKETSIZE);
			
			map<string, Loc>::iterator it = online_users.find(invite_accept_from);
			if (it != online_users.end()) {
				inet_ntop(AF_INET, &(it->second.addr.sin_addr), str, INET_ADDRSTRLEN);
				res = new MSG("", "", string(str), to_string(ntohs((int)it->second.addr.sin_port)), 1);
				serialize(res, data);
				write(i->second, data, PACKETSIZE);
			}

			map<string, One_User>::iterator itt = users_info.find(invite_accept_from);
			if (itt != users_info.end()) {
				itt->second.friends.push_back(invite_accept_sending_to);
			}
		}

		map<string, int>::iterator i1 = online_sockets.find(invite_accept_from);
		if (i1 != online_sockets.end()) {
			MSG* res;
			
			map<string, Loc>::iterator it1 = online_users.find(invite_accept_sending_to);
			if (it1 != online_users.end()) {
				inet_ntop(AF_INET, &(it1->second.addr.sin_addr), str, INET_ADDRSTRLEN);
				res = new MSG("", "", string(str), to_string(ntohs((int)it1->second.addr.sin_port)), 1);
				serialize(res, data);
				write(i1->second, data, PACKETSIZE);
			}

			map<string, One_User>::iterator itt1 = users_info.find(invite_accept_sending_to);
			if (itt1 != users_info.end()) {
				itt1->second.friends.push_back(invite_accept_from);
			}
		}
	} else if (strcmp(msg->command, "logout") == 0) {
		string username = string(msg->username);
		delete_online_user(username);
		MSG* res = new MSG("", "", "", "200", 1);
		serialize(res, data);
		write(client_sock, data, PACKETSIZE);
	}
	return;
}

void handle_clients(int sock_fd) {
	int max_fd, client_sock;
	socklen_t length;
	struct sockaddr_in recv_addr;
	vector<int> client_sockets;
	fd_set all_set, read_set;
	char data[PACKETSIZE];
	char arr[NUMMEMBERSMSG*BUFSIZE];
	MSG* msg = new MSG;

	FD_ZERO(&all_set);
	FD_SET(sock_fd, &all_set);
	max_fd = sock_fd;

	client_sockets.clear();

	length = sizeof(recv_addr);

	while (1) {
		read_set = all_set;
		select(max_fd+1, &read_set, NULL, NULL, NULL);
		if (FD_ISSET(sock_fd, &read_set)) {
			if ((client_sock = accept(sock_fd, (struct sockaddr *)&recv_addr, &length)) < 0) {
				if (errno == EINTR) {
					continue;
				} else {
					perror(": accept");
					return;
				}
			}

			printf("Client connected. Address: %s, Port: %d\n",
				inet_ntoa(recv_addr.sin_addr), ntohs(recv_addr.sin_port));

			client_sockets.push_back(client_sock);
			FD_SET(client_sock, &all_set);
			if (client_sock > max_fd) max_fd = client_sock;
		}

		int read_num, fd;

		for (vector<int>::iterator it = client_sockets.begin(); it != client_sockets.end(); ++it) {
			fd = *it;
			if (FD_ISSET(fd, &read_set)) {
				read_num = read(fd, data, PACKETSIZE);
				if (read_num == 0) {
					continue;
				} else {
					handle_single_req(sock_fd, fd, data);
				}
			}
		}

		max_fd = sock_fd;
		if (!client_sockets.empty()) {
			max_fd = max(max_fd, *max_element(client_sockets.begin(), client_sockets.end()));
		}
	}

	return;
}

void sig_intr(int signo) {
	cout << "Interrupt Caught\nSaving data to file" << endl;
	ofstream myfile;
	myfile.open("user_info");
	map<string, One_User>::iterator it = users_info.begin();
	while (it != users_info.end()) {
		// cout << "username: " << it->first << ", password: " << it->second.password << endl;
		myfile << it->first << "|" << it->second.password << "|";
			// cout << "friends: ";
		for (int i = 0; i < it->second.friends.size()-1; ++i) {
			myfile << it->second.friends[i] << ";";
			// cout << it->second.friends[i] << " ";
		}
		myfile << it->second.friends[it->second.friends.size()-1];
		myfile << "\n";
		// cout << "\n";
		it++;
	}
	myfile.close();
	exit(0);
}

void server() {
	struct sockaddr_in addr;
	int sock_fd, res;
	socklen_t length;
	char host[NI_MAXHOST];
	char service[20];

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Can not get the socket");
		return;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)stoi(config_info.at("port")));

	length = sizeof(addr);

	if (bind(sock_fd, (struct sockaddr *)&addr, length) < 0) {
		perror(": bind");
		return;
	}

	if (getsockname(sock_fd, (struct sockaddr *)&addr, (socklen_t *)&length) < 0) {
		perror(": can't get name");
		return;
	}

	printf("Listening at ip = %s, port = %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	// if ((res=getnameinfo((struct sockaddr *)&addr, length, host, sizeof(host), service, sizeof(service), 0)) < 0)
	// {
	// 	cout << gai_strerror(res);
	// }

	if (listen(sock_fd, 50) < 0) {
		perror(": bind");
		return;
	}

	handle_clients(sock_fd);
}

int main(int argc, const char *argv[]) {
	if (argc < 3) {
		cout << "Usage: messenger_server user_info_file configuration_file" << endl;
		exit(0);
	}

	struct sigaction sig;
	sig.sa_handler = sig_intr;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;
	sigaction(SIGINT, &sig, NULL);

	read_user_info(argv[1]);
	read_configuration_info(argv[2]);
	server();
	return 0;
}
