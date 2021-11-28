#include<iostream>
#include<string>
#include<string.h>
#include "server_chat.h"
#include <vector>
using namespace std;

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

#define MESSAGE_LENGTH 1024 // Максимальный размер буфера для данных
extern char ch_message[MESSAGE_LENGTH];
extern SOCKET connection;

 bool compare_LP(string LP, string L, string P)//compares login and password
 {
    return LP == L + P;
 }

//server -> client
#ifdef _WIN32
void my_send(string _mes, int connection)
{
    size_t bytes;
    memset(ch_message, 0, MESSAGE_LENGTH);
    strncpy_s(ch_message, _mes.c_str(), MESSAGE_LENGTH);
    if (_mes.size() >= MESSAGE_LENGTH)
    {
        ch_message[MESSAGE_LENGTH - 1] = '\0';
    }
    cout << ch_message << endl;
    bytes = send(connection, ch_message, sizeof(ch_message), 0);
    if (bytes < 0) cout << "send failed" << endl;
    
}

//client -> server
string my_receive(int connection)
{
    memset(ch_message, 0, MESSAGE_LENGTH);
    size_t bytes = recv(connection, ch_message, MESSAGE_LENGTH, 0);
    if (bytes < 0)
    {
        perror("recv");
    }
    string s_read = string(ch_message, strlen(ch_message));//for received msg    
    cout << s_read << endl;
    //memset(ch_message, 0, MESSAGE_LENGTH);
    return s_read;
}
#else
void my_send(string _mes, int connection)
{
    ssize_t bytes;
    bzero(ch_message, MESSAGE_LENGTH);
    strncpy(ch_message, _mes.c_str(), MESSAGE_LENGTH);
    if (_mes.size() >= MESSAGE_LENGTH)
    {
        ch_message[MESSAGE_LENGTH - 1] = '\0';
    }
    bytes = send(connection, ch_message, sizeof(ch_message), 0);
    if (bytes < 0) cout << "send failed" << endl;

}

//client -> server
string my_receive(int connection)
{
    bzero(ch_message, MESSAGE_LENGTH);
    ssize_t bytes = recv(connection, ch_message, MESSAGE_LENGTH, 0);
    if (bytes < 0)
    {
        perror("recv");
    }
    string s_read = string(ch_message, strlen(ch_message));//for received msg    
    cout << s_read << endl;
    return s_read;
}

#endif

string get_login(map<int, string>UC, int connection)
{
    string login;
    map<int, string >::iterator it;
    for (it = UC.begin(); it != UC.end(); ++it)
        if (it->first == connection)
            login = it->second;
    return login;
}


int get_con(map<int, string>UC, string login)
{
    int con;
    map<int, string >::iterator it;
    for (it = UC.begin(); it != UC.end(); ++it)
        if (it->second == login)
        {
            con = it->first;
            return con;
        }
    return 0;
}
