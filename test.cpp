#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mysql.h>
#include <iomanip>
#include <sstream>
#include <string.h>
using namespace std;
#include "server_chat.h"
#include "MY_SQL.h"
#pragma warning(disable:4996)
#pragma comment (lib, "Ws2_32.lib")

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/wait.h>
#include <arpa/inet.h>
#endif

#define PORT 20000 
#define MY_QUEUE 5//num of clients in the connection queue
#define MaxUserCount 10//limit of Users
#define MESSAGE_LENGTH 1024 // Максимальный размер буфера для данных
char ch_message[MESSAGE_LENGTH];

struct sockaddr_in serveraddress, client;
socklen_t length;
extern MYSQL mysql;
#ifdef _WIN32
SOCKET connection;
int activity, sd, max_sd, max_clients = 5;
#else
int socket_file_descriptor, connection, bind_status, connection_status, activity, sd, max_sd, max_clients = 5;
#endif

vector <int>connections;//to store socket descriptors
FD_SET readfds;//set of socket descriptors

int main()
{
    setlocale(LC_ALL, "rus");
    CHAT_DB_connect();    
    build_db();
    

    
    string name, login, password, email;//current user  
    size_t choice = 1;//switchmax_clients = 30
    bool ch(true);//while    
    bool n;//cycles       
    stringstream ss;//to collect strings for my_send()    
    int ERR;//for DB response
    map<int, string>UC;//stores login of connection

#ifdef _WIN32
    //WSAStartup
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        std::cout << "Error WSAStartup" << std::endl;
        exit(1);
    }
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    //addr.sin_addr.s_addr = inet_addr("127.0.0.1");   
    addr.sin_addr.S_un.S_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;
    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);
#else

    // Создадим сокет
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor == -1)
    {
        cout << "Socket creation failed.!" << endl;
        exit(1);
    }
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
        sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // Зададим номер порта для связи
    serveraddress.sin_port = htons(PORT);
    // Используем IPv4
    serveraddress.sin_family = AF_INET;
    // Привяжем сокет
    bind_status = bind(socket_file_descriptor, (struct sockaddr*)&serveraddress,
        sizeof(serveraddress));
    if (bind_status == -1)
    {
        cout << "Socket binding failed.!" << endl;
        exit(1);
    }
    // Поставим сервер на прием данных 
    connection_status = listen(socket_file_descriptor, MY_QUEUE);
    if (connection_status == -1)
    {
        cout << "Socket is unable to listen for new connections.!" << endl;
        exit(1);
    }
    else cout << "Server is listening for new connection: " << endl;
#endif 
       
    while (ch)
    {
      switch (choice)
      {
        case 1:

            //clear the socket set 
            FD_ZERO(&readfds);
#ifdef _WIN32
            //add listening socket to set
            FD_SET(sListen, &readfds);
            max_sd = sListen;
            //add child sockets to set
            for (int i = 0; i < connections.size(); i++)
            {
                //socket descriptor 
                sd = connections[i];

                //if valid socket descriptor then add to read list 
                if (sd > 0)
                    FD_SET(sd, &readfds);

                //highest file descriptor number, need it for the select function 
                if (sd > max_sd)
                    max_sd = sd;
            }

#else

            FD_SET(socket_file_descriptor, &readfds);
            max_sd = socket_file_descriptor;



            //add child sockets to set 
            for (int i = 0; i < connections.size(); i++)
            {
                //socket descriptor 
                sd = connections[i];

                //if valid socket descriptor then add to read list 
                if (sd > 0)
                    FD_SET(sd, &readfds);

                //highest file descriptor number, need it for the select function 
                if (sd > max_sd)
                    max_sd = sd;
            }
#endif
#ifdef _WIN32

            //wait for an activity on one of the sockets
            activity = select(NULL, &readfds, NULL, NULL, NULL);
            if (activity == 0)
            {
                printf("select() returned with error %d\n", WSAGetLastError());
                return 1;
            }
            else cout << "select() is ok" << endl;

#else       //wait for an activity on one of the sockets          
            activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            {
                printf("select error");
            }
#endif

#ifdef _WIN32
            //If something happened on the master socket , 
            //then its an incoming connection 
            
            SOCKET connection;
            if (FD_ISSET(sListen, &readfds))
            {
                connection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
                if (connection <= 0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                else cout << "accept() is ok" << endl;

                // socket number for send and receive 
                printf("New connection , socket fd is %d , ip is : %s , port : %d \n", (int)connection, inet_ntoa(addr.sin_addr), ntohs
                (addr.sin_port));
                connections.push_back(connection);
            }
            else
            {
                for (int i = 0; i < connections.size(); i++)
                    my_receive(connections[i]);
                choice = 2;
                break;
            }

#else 
            //If something happened on the master socket , 
            //then its an incoming connection 
            if (FD_ISSET(socket_file_descriptor, &readfds))
            {
                if ((connection = accept(socket_file_descriptor,
                    (struct sockaddr*)&client, &length)) < 0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                //inform user of socket number - used in send and receive commands 
                printf("New connection , socket fd is %d , ip is : %s , port : %d \n", connection, inet_ntoa(serveraddress.sin_addr), ntohs
                (serveraddress.sin_port));
                connections.push_back(connection);
            }

            else
            {
                for (int i = 0; i < connections.size(); i++)
                    my_receive(connections[i]);
                choice = 2;
                break;
            }
#endif

                       
            n = true;
            while (n)//REGISTRATION
            { 
                name.clear();login.clear(); password.clear(); email.clear();
                my_send(ss.str() + "REGISTRATION: Enter your NAME", connection);
                ss.str("");
                while (name.empty())
                {
                    name = my_receive(connection);
                }
                
                my_send(ss.str() + "Enter E-MAIL", connection);
                ss.str("");
                while (email.empty())
                {
                    email = my_receive(connection);
                }            
            
                
                my_send(ss.str() + "Enter UNIQUE LOGIN ", connection);
                ss.str("");
                while (login.empty())
                {
                    login = my_receive(connection);
                }          


                my_send("Enter PASSWORD", connection);
                while (password.empty())
                {
                    password = my_receive(connection);
                }              

            
                ERR = registration(name, login, email, password);
                if (ERR == 1)
                {
                    ss << "ERROR OF INSERT into USERS: login wasn't UNIQUE\n";
                    
                }
                else if (ERR == 2)
                {
                    ss << "ERROR OF INSERT into HASH\n";
                   
                }               
               
                else
                {                    
                    UC.insert(make_pair(connection,login));
                    ss << get_users();
                    my_send(ss.str() + "Enter SPACE to continue \n", connection);
                    n = false;
                }
             }

            break;

        case 2://AUTHORIZATION

        n = true;
         {
           int i = 0;
            while (n)
            {                
                while (i < connections.size())
                {
                    connection = connections[i];
                    name.clear(); login.clear(); password.clear();
                    my_send(ss.str() + "AUTHORIZATION: Enter LOGIN", connection);
                    ss.str("");
                    while (login.empty())
                    {
                        login = my_receive(connection);
                    }

                    my_send("Enter PASSWORD", connection);
                    while (password.empty())
                    {
                        password = my_receive(connection);
                    }

                    name = get_name_from_DB(login, password);
                    if (name == "")
                    {
                        ss << "Invalid password or login. Try again\n";
                        login.clear(); password.clear();                        
                        break;
                    }

                    ss << "\nHELLO " + name + "!\n";
                    if (i < connections.size() -1)
                    {
                        my_send(ss.str() + "Please wait until all the users are authorized, ENTER SPACE\n", connection);
                        ss.str("");
                    }
                    else
                    {
                        my_send(ss.str() + "All the users are authorized, ENTER SPACE\n", connection);
                        ss.str("");
                        choice = 3;
                        n = 0;
                    }
                    my_receive(connection);
                    i++;
                } 
            }
         }
            break;

        case 3://individual messages
        {

            ss << "Your messages:\nid | sender | receiver | text            | data):\n" << get_message(get_login(UC, connection));
            my_send(ss.str() + "Enter 1 to choose the login of your friend to text to\nEnter 2 to text to everyone\nEnter 3 to delete your account\nEnter # to quit chat\n", connection);
            ss.str("");
            string result;
            while (result == "")
            {
                result = my_receive(connection);
            }

            if (result == "1")
            {
                choice = 4; break;
            }
            else if (result == "2")
            {
                choice = 5; break;
            }
            else if (result == "3")
            {
                choice = 6; break;
            }            
            if (ch_message[0] == '#')
            {
                choice = 7; break;          
            
            }
            else 
            {
                ss << "Wrong sign, try again\n";
                result.clear();
            }
            break;
        }
        case 4://individual message
        {

            string l;//stores login of friend
            while (1)
            {                
                my_send(ss.str() + "Enter the login of your friend", connection);
                ss.str("");
                l = my_receive(connection);
                if (check_login(l))//// and his connection is active! 
                break;
                ss << "unregistered user, try again\n";
            }

            my_send("text your message to " + l, connection);
            string m = my_receive(connection);//receives message

            ERR = store_mes(get_login(UC, connection), l, m);
            if (ERR)
            {
                cout << "Error of store_mes\n";
            }   
                


           
            map<int, string >::iterator it = UC.find(get_con(UC, l));            
            if (it == UC.end())
                {
                    ss << "User is offline, message was stored\n";
                    my_send(ss.str() + "\nENTER SPACE to continue", connection);
                    my_receive(connection);
                }
            else
                {
                    connection = it->first;
                    my_send(m + "\nENTER SPACE to continue", connection);
                    my_receive(connection);
                }      
                
            

            
        }
        choice = 3;
        break;

        case 5://message for everyone             

        {
            my_send("text your message for everyone\n", connection);
            string m = my_receive(connection);
            store_mes(get_login(UC, connection), "common chat", m);
            for (int i = 0; i < connections.size(); i++)
            {
                if (connections[i] == connection && i < connections.size() - 1)
                {
                    i++;
                    my_send(m + "\nEnter SPACE to continue", connections[i]);
                    my_receive(connections[i]);
                }
                else if (connections[i] != connection)
                {
                    my_send(m + "\nEnter SPACE to continue", connections[i]);
                    my_receive(connections[i]);
                }
                else break;
            }
        }

        choice = 3;
        break;

        case 6://delete account
        {    delete_user(get_login(UC, connection));
             my_send("#Account has been deleted, connection has been stopped by server", connection);

            map<int, string >::iterator it;            
            it = UC.find(connection);
            UC.erase(it);
            if (connections.size() == 1)
            {
                choice = 7; break;
            }
            for (int i = 0; i < connections.size(); i++)
            {
                if (connections[i] == connection)
                {
                    connections.erase(connections.begin() + i);
                    connection = connections[0];
                    choice = 3;
                    break;
                }                
            }     
                     
        }
        break;

        case 7://FINISH
#ifdef _WIN32  
         
         if (connections.size() == 1)
         {
             cout << "server is to close" << endl;
             my_send("# BYE!", connection);
             closesocket(connection);
         }
         else
         {                      
            my_send("# BYE!", connection);            
            for (int i = 0; i < connections.size(); i++)
            {
                
                if (connections[i] == connection)
                {
                    map<int, string >::iterator it;
                    it = UC.find(connection);
                    UC.erase(it);

                    closesocket(connection);
                    connections.erase(connections.begin() + i);
                    connection = connections[0];
                    choice = 3;
                    break;
                }
                 
            }
         }if (choice == 3) break;

         WSACleanup();
#else

           if (ch == false)//FINISH
            {
                for (int i = 0; i < connections.size(); i++)
                {
                    if (connections[i] == connection) i++;
                    my_send("#", connections[i]);
                }
                cout << "server is to close" << endl;
                for (int i = 0; i < connections.size(); i++)
                {
                    if (connections[i] == connection) i++;
                    close(connections[i]);
                }
            }

#endif
           ch = false;
           break;

        
      }//switch

    }
    mysql_close(&mysql);
    return 0;
}




