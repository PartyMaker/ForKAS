#include "ForKAS.hpp"
#include <iostream>
#include <fstream>
#include <thread>

std::istream& operator>>(std::istream& str, CSVRow& data)
{
	data.readNextRow(str);
	return str;
}
void do_somethink(product_set& product_set)
{
	std::this_thread::sleep_for(std::chrono::seconds(1));
	product_set.erase("720");
}
int main()
{
	product_set product_set;
	std::ifstream file("MOCK_DATA.csv");
	CSVRow row;

	while (file >> row)
		product_set.insert(product(row[0], row[1], row[2], std::atoi(row[3].c_str())));

	auto id = product_set.find_id("10");
	auto products = product_set.find_name("Acura");

	std::thread t1(do_somethink, std::ref(product_set));
	t1.detach();
	for (auto it = products.first; it != products.second; it++)
		std::cout << it->second->manufacturer + " " + it->second->id << std::endl;
	
	product_set.erase("11");
	return 0;
}

