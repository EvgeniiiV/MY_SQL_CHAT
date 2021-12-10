#include <iostream>
#include <mysql.h>
#include <sstream>
#include <iomanip>
using namespace std;
#include "MY_SQL.h"
MYSQL mysql;
MYSQL_RES* res;
MYSQL_ROW row;
 


void CHAT_DB_connect()
{
	// Получаем дескриптор соединения
	mysql_init(&mysql);
	if (&mysql == nullptr) {
		// Если дескриптор не получен — выводим сообщение об ошибке
		cout << "Error: can't create MySQL-descriptor" << endl;
	}

	// Подключаемся к серверу
	if (!mysql_real_connect(&mysql, "localhost", "root", "root", "testdb", NULL, NULL, 0)) {
		// Если нет возможности установить соединение с БД выводим сообщение об ошибке
		cout << "Error: can't connect to database " << mysql_error(&mysql) << endl;
	}
	else {
		// Если соединение успешно установлено выводим фразу — "Success!"
		cout << "MySQL connection is ok" << endl;
	}

	mysql_set_character_set(&mysql, "utf8");
	//Смотрим изменилась ли кодировка на нужную, по умолчанию идёт latin1
	cout << "connection characterset: " << mysql_character_set_name(&mysql) << endl;	
}

string query1()
{
	stringstream ss;
	int i = 0;	
	mysql_query(&mysql, "SELECT * FROM test"); //Делаем запрос к таблице

	//Выводим все что есть в базе через цикл
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			for (i = 0; i < mysql_num_fields(res); i++) {
				ss << row[i] << "  ";
			}			
			ss << '\n';
		}
	}
	else
		cout << "Ошибка MySql номер " << mysql_error(&mysql);
	return ss.str();
	
}
void build_db()
{
	mysql_query(&mysql, "CREATE DATABASE IF NOT EXISTS chat default charset cp1251");
	if (mysql_errno(&mysql))
		cout << "Error of CREATE\n";
	mysql_query(&mysql, "USE chat");
	if (mysql_errno(&mysql))
		cout << "Error of USE\n";
	mysql_query(&mysql, "CREATE TABLE IF NOT EXISTS users(user_id SERIAL NOT NULL AUTO_INCREMENT PRIMARY KEY, name varchar (255) NOT NULL, login varchar(255) UNIQUE, email varchar(255) NOT NULL)");
	if (mysql_errno(&mysql))
		cout << "Error of creating tabl USERS\n";
	mysql_query(&mysql, "CREATE TABLE IF NOT EXISTS hash(hash_id SERIAL NOT NULL AUTO_INCREMENT PRIMARY KEY, hash  varchar (40) NOT NULL, FOREIGN KEY (hash_id)  REFERENCES users (user_id) ON DELETE CASCADE)");
	if (mysql_errno(&mysql))
		cout << "Error of creating tabl HASH\n";
	mysql_query(&mysql, "CREATE TABLE IF NOT EXISTS message (message_id SERIAL NOT NULL AUTO_INCREMENT PRIMARY KEY, sender varchar (255), receiver varchar (255) , text TEXT (255), Timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
	if (mysql_errno(&mysql))
		cout << "Error of creating tabl MESSAGE\n";	
	
	mysql_query(&mysql, "CREATE TRIGGER  hash_insert AFTER INSERT ON users FOR EACH ROW INSERT INTO hash (hash_id, hash) VALUES(new.user_id, 'empty')");
	if (mysql_errno(&mysql))
		cout << "Error of creating trigger hash_insert\n";

}


int registration(string name, string login, string email, string hash)
{
	
	string for_users = "INSERT users (name, login, email) VALUES('" + name + "', '" + login + "', '" + email + "')";
	mysql_query(&mysql, for_users.c_str());
	
	if (mysql_errno(&mysql))
		return 1;
	
	string for_update = "UPDATE hash SET hash = '" + hash + "' where hash_id = (SELECT MAX(user_id) from users)";
	mysql_query(&mysql, for_update.c_str());
	if (mysql_errno(&mysql))
		return 2;
	return 0;
}

string get_users()
{
	mysql_query(&mysql, "select login from users");
	int i;stringstream ss;
	ss << "REGISTERED USERS:\n";

	if (res = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(res))
		{
			for (i = 0; i < mysql_num_fields(res); i++)
			{
				ss << row[i] << "  ";
			}
			ss << endl;
		}
	}
	else cout << "Ошибка MySQL registration1() " << mysql_error(&mysql);		

	return ss.str();
}

string get_name_from_DB(string login, string hash)
{
	int i;
	stringstream ss;
 
	string for_name = "select name from users left join hash on users.user_id = hash.hash_id where users.login = '" + login + "' and hash.hash = '" + hash + "'";
	mysql_query(&mysql, for_name.c_str());
	if (res = mysql_store_result(&mysql))
	{		
		while (row = mysql_fetch_row(res))
		{
			for (i = 0; i < mysql_num_fields(res); i++)
			{
				ss << row[i];
			}
			
		}
	}	

	else
	cout << "Ошибка MySql get_name() " << mysql_error(&mysql);	
	return ss.str();
}

bool check_login(string login)
{
	int i;
	
	stringstream ss;
	string for_name = "select count(*) FROM users WHERE name ='" + login + "'";
	mysql_query(&mysql, for_name.c_str());
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			for (i = 0; i < mysql_num_fields(res); i++) {
				ss << row[i];			
				
			}			
		}
	}
	else
		cout << "Ошибка MySql in check_name " << mysql_error(&mysql);	
	return ss.str() == "1";
}

int store_mes(string  sender, string receiver, string text)
{
	string str = "INSERT message (sender, receiver, text) VALUES('" + sender + "', '" + receiver + "', '" + text + "')";
	mysql_query(&mysql, str.c_str());
	
	if (mysql_errno(&mysql))
	 return 1;
	return 0;

}

int  get_id(string login)
{
	stringstream ss;
	string str = "select user_id from users where login = '" + login + "'";
	mysql_query(&mysql, str.c_str());
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			for (int i = 0; i < mysql_num_fields(res); i++) {
				ss << row[i];				
			}
		}
	}
	else
		cout << "Ошибка MySql in get_id " << mysql_error(&mysql);
	return stoi(ss.str());
}

void delete_user(string login)
{
	string str = "UPDATE message SET sender = 'deleted' WHERE sender = '" + login + "'";
	mysql_query(&mysql, str.c_str());
	if (mysql_errno(&mysql))
		cout << "error 'sender' of delete_user";
	str = "UPDATE message SET receiver = 'deleted' WHERE receiver = '" + login + "'";
	mysql_query(&mysql, str.c_str());
	if (mysql_errno(&mysql))
		cout << "error 'receiver' of delete_user";
	str = "delete from users where login = '" + login + "'";
	mysql_query(&mysql, str.c_str());
	if (mysql_errno(&mysql))
		cout << "error of delete_user";
}

string get_message(string cur_user)
{
	stringstream ss;
	string str = "select * from message where sender = '" + cur_user + "'";
	mysql_query(&mysql, str.c_str());
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			for (int i = 0; i < mysql_num_fields(res); i++) {
				ss << row[i] << " | ";
			}ss << endl;
		}
	}
	else
		cout << "ERROR of get_messages sender; " << mysql_error(&mysql);

	str = "select * from message where receiver = '" + cur_user + "'";
	mysql_query(&mysql, str.c_str());
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			for (int i = 0; i < mysql_num_fields(res); i++) {
				ss << row[i] << " | ";
			}ss << endl;
		}
	}
	else
		cout << "ERROR of get_messages receiver; " << mysql_error(&mysql);

	str = "select * from message where receiver = 'common chat'";
	mysql_query(&mysql, str.c_str());
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			for (int i = 0; i < mysql_num_fields(res); i++) {
				ss << row[i] << " | ";
			}ss << endl;
		}
	}
	else
		cout << "ERROR of get_messages common chat; " << mysql_error(&mysql);

	return ss.str();
}




