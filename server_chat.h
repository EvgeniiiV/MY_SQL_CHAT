#pragma once
#include<iostream>
#include <map>
using namespace std;

bool compare_LP(string LP, string L, string P);
void my_send(string _mes, int connection);
string my_receive(int connection);
string get_login(map<int, string>UC, int connection);