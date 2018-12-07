#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

class One_User {
	public:
		string username;
		string password;
		vector<string> friends;
};

map<string, string> config_info;
map<string, One_User> users_info;
map<string, string> online_users;

void register_user(string username, string password) {
	map<string, One_User>::iterator it = users_info.find(username);
	if (it == users_info.end()) {
		One_User user;
		user.username = username;
		user.password = password;
		users_info.insert({username, user});
		return;
	}
	cout << "Already a user by that name\n";
}

void login_user(string username, string password) {
	map<string, One_User>::iterator it = users_info.find(username);
	if (it != users_info.end()) {
		if (it->second.password == password) {
			online_users.insert({username, "null"});
			cout << "successfully logged in " << username << endl;
			return;
		}
	}
	cout << "username/password doesn't match " << username << endl;
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
		len = parse_string(line, ";", &vec1);

		for (int i = 0; i < len; i++) {
			user.friends.push_back(vec1[i]);
		}
		users_info.insert({user.username, user});
	}
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
	map<string, string>::iterator it = online_users.find(username);
	if (it != online_users.end()) {
		online_users.erase(it);
		cout << "user disconnected " << username << endl;
	} else {
		cout << "Error removing the user: already disconnected " << username << endl;
	}
}

int main(int argc, const char *argv[]) {
	if (argc < 3) {
		cout << "Usage: messenger_server user_info_file configuration_file" << endl;
		exit(0);
	}

	read_user_info(argv[1]);
	read_configuration_info(argv[2]);
	register_user("hamzuz", "ad");
	login_user("hamzuz", "ad");
	delete_online_user("hamzuz");
	return 0;
}
