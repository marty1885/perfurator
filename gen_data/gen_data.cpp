#include <random>
#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <iostream>

#include <crypto.hpp>

std::string make_uuid()
{
	const std::string uuid_alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	static std::mt19937 rng;
	std::uniform_int_distribution<int> dist(0, uuid_alphabet.size() - 1);

	std::string uuid;
	uuid.resize(36);
	for (size_t index = 0; index < 36; ++index)
		uuid[index] = uuid_alphabet[dist(rng)];

	uuid[8] = '-';
	uuid[13] = '-';
	uuid[18] = '-';
	uuid[23] = '-';

	uuid[14] = '4';
	return uuid;
}

int main()
{
    constexpr size_t num_data = 23000000;

	std::unordered_set<std::string> ids;
	ids.reserve(num_data);
	while(ids.size() != num_data)
		ids.insert(make_uuid());

	std::vector<std::string> passwords;
	passwords.reserve(ids.size());
	for(size_t index = 0; index < ids.size(); ++index)
		passwords.push_back(make_uuid());

	// Generate a CSV as out "database" of user ID and passwords (in
	// plantext, we need it to simulate client behaivour)
	size_t index = 0;  // HACK: Properbly should use a zipped-iterator
	std::ofstream out("users_plantext.csv");
	out << "id,password\n";
	for (const auto& id : ids) {
		out << id << "," << passwords[index] << "\n";
		++index;
	}
	out.close();

	// Then we generate another file that contains the ID, salt and the salted hash of the password
	std::ofstream out2("users.csv");
	out2 << "id,password\n";
	index = 0;
	for (const auto& id : ids) {
		out2 << id << "," << generate_password(passwords[index]) << "\n";
		++index;
	}
	out2.close();
}
