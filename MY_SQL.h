#pragma once

void CHAT_DB_connect();
string query1();
void build_db();
void default_filling();
int registration(string name, string login, string email, string hash);
//bool authorization(string login, string hash);
string get_name_from_DB(string login, string password);
bool check_login(string name);
string get_users();
int  get_id(string login);
int store_mes(string sender_id, string receiver_id, string text);
string get_message(string cur_user);
void delete_user(string login);
