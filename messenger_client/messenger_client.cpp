#include <iostream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

map<string, string> config_info;
map<string, string> online_friends;

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

int main(int argc, char const *argv[])
{
	if (argc < 2) {
		cout << "Usage: messenger_client configuration_file\n";
		exit(0);
	}

	read_configuration_info(argv[1]);
	// cout << "servport: " << config_info.at("servport") << endl;
	// cout << "servhost: " << config_info.at("servhost") << endl;

	return 0;
}