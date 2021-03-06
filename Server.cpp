/* 
 * File:   Server.cpp
 * Author: Anthony Correia <anthony.correia71@gmail.com>
 * 
 * Created on 18 mars 2015, 09:05
 */

#include "Server.h"
#include "GameMaster.h"
#include "errcodes.h"

#include "json/json.h"

#include <iostream>
#include <string.h>
#include <cstdio>
#include <thread>
#include <algorithm>
#include <ctype.h>
#include <cstdlib>

using namespace std;

Server::Server()
{
    cout << "[SERVER] Entering Server constructor." << endl <<
            "[SERVER] Init variables." << endl;
    
    srand(time(NULL));
    READY = false;
    ACCEPTING = false;
    accepting_thread = NULL;
    master = NULL;
    player_ids = 1;
    NB_PLAYERS = -1;
    PLAYING = false;
    
    memset(&host_info, 0, sizeof host_info);
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;
    
    status = getaddrinfo(NULL, SERVER_PORT, &host_info, &host_info_list);
    
    if (status != 0)  
    {
        cout << "getaddrinfo error" << gai_strerror(status) ;
        return;
    }
    
    cout << "Creating the listening socket"  << std::endl;
    bind_socketfb = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                      host_info_list->ai_protocol);
    if (bind_socketfb == -1)  
    {
        perror("[SERVER] Creating listening socket");
        return;
    }
    
    struct timeval timeout;      
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    if (setsockopt(bind_socketfb, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        perror("[SERVER] Setting socket timeout");

    cout << "Binding socket..."  << std::endl;
    int yes = 1;
    status = setsockopt(bind_socketfb, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if(status == -1)
    {
        perror("[SERVER] setsockopt");
        return;
    }
    status = bind(bind_socketfb, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1)  
    {
        perror("[SERVER] bind");
        return;
    }

    std::cout << "Setting socket as a listening socket..."  << std::endl;
    status =  listen(bind_socketfb, 5);
    if (status == -1)  
    {
        std::cout << "listen error" << std::endl ;
        return;
    }
    READY = true;
    cout << "[SERVER] All done, server ready." << endl;
}

Server::~Server()
{
    if(status != -1)
    {
        
    }
}

bool Server::ready()
{
    return READY;
}

bool Server::accept()
{
    bool ret;
    if(READY == false)
    {
        cout << "[SERVER] Cannot start accepting clients, server is not ready!" <<
                endl;
        ret = false;
    }
    else
    {
        ACCEPTING = true;
        accepting_thread = new thread(&Server::accept_thread, this);
        accepting_thread->join();
    }
    
    return ret;
}

void Server::accept_thread()
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    cout << "[SERVER] Entering accepting loop..." << endl;
    while(true)
    {
        accepting_mutex.lock();
        if(ACCEPTING)
        {
            int new_socket = accept4(bind_socketfb, (struct sockaddr *)&their_addr, 
                    &addr_size, 0);
            if(new_socket != -1)
            {
                cout << "[SERVER] New connexion!" << endl;
                struct timeval timeout;      
                timeout.tv_sec = 3;
                timeout.tv_usec = 0;

                if (setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                            sizeof(timeout)) < 0)
                    perror("[SERVER] Setting socket timeout for new client");
                if(master == NULL)
                {
                    cout << "[SERVER] First client, creating game master." << endl;
                    master = new GameMaster(this, 0, new_socket);
                }
                else
                {
                    cout << "[SERVER] Adding new player." << endl;
                    connected_players.insert(new Player(this, player_ids++, new_socket));
                }
            }
        }
        accepting_mutex.unlock();
    }
}

int Server::joinGame(Player* player)
{
    char* nickname = player->getNickname();
    cout << "[SERVER] New player joined the game: " << nickname << "." << endl;
    delete nickname;
    players.insert(player);
    connected_players.erase(player);
}

void Server::talk(char* msg, Client* except)
{
    cout << "[SERVER] Sending message to all clients: \"" << msg << "\"" << endl;
    if(master != except)
    {
        master->sendMsg(talkToJSON(((Player*)except)->getNickname(), msg, false));
    }
    for(Player* p : players)
    {
        if(p != except)
        {
            p->sendMsg(talkToJSON(p->getNickname(), msg, except == master));
        }
    }
}

void Server::talkto(std::vector<char*>& nicknames, char* msg)
{
    cout << "[SERVER] Sending message to selected clients: \"" << msg << "\"" << endl;
    char* duplicate, *nickname;
    for(char* str : nicknames)
    {
        toLower(str);
    }
    for(vector<char*>::iterator it=nicknames.begin(); it!=nicknames.end();)
    {
        bool erased = false;
        for(Player* p : players)
        {
            nickname = p->getNickname();
            toLower(nickname);
            if(strcmp(nickname, *it) == 0)
            {
                p->sendMsg(talkToJSON(p->getNickname(), msg, true));
                nicknames.erase(it);
                erased = true;
                break;
            }
        }
        if(!erased)
            ++it;
        else if(it == nicknames.end())
            break;
    }
}


bool Server::hasJoined(Player* p)
{
    return players.find(p) != players.end();
}

bool Server::isNicknameAvailable(char* nickname)
{
    char* cpy = new char[strlen(nickname)+1];
    strcpy(cpy, nickname);
    toLower(cpy);
    bool ret = true;
    for(Player* p : players)
    {
        char* nickname_p = p->getNickname();
        toLower(nickname_p);
        if(strcmp(cpy, nickname_p) == 0)
            ret = false;
        delete nickname_p;
        if(!ret)
            break;
    }
    return ret;
}

bool compareStr(const char* a, const char* b)
{
    return strcmp(a, b) < 0;
}

void Server::toLower(char* str)
{
    int length = strlen(str);
    for(int i = 0; i < length; i++)
        str[i] = tolower(str[i]);
}

void Server::setNbPlayers(int nb)
{
    NB_PLAYERS = nb;
}

void Server::startGame()
{
    PLAYING = true;
    for(Player* p : players)
    {
        p->sendCode(GAME_START_TRIGGER);
    }
}

void Server::endGame()
{
    PLAYING = false;
    for(Player* p : players)
    {
        p->sendCode(GAME_END_TRIGGER);
        connected_players.insert(p);
    }
    players.clear();
}

bool Server::isPlaying()
{
    return PLAYING;
}

int Server::getNbPlayers()
{
    return NB_PLAYERS;
}

int Server::getConnectedPlayers()
{
    return players.size();
}

bool Server::isSetNbPlayers()
{
    return NB_PLAYERS > 0;
}

char *Server::playersInfo(Client* c)
{
    Json::Value root;
    root["cmd"] = "players";
    Json::Value array = Json::arrayValue;
    for(Player* p : players)
    {
        if(p != c)
        {
            Json::Value pval;
            char* nickname = p->getNickname();
            pval["nickmane"] = nickname;
            delete nickname;
            pval["alive"] = p->getLifePoint() > 0;
            if(c == master)
                pval["lp"] = p->getLifePoint();
            array.append(pval);
        }
    }
    root["players"] = array;
    Json::FastWriter wr;
    string str = wr.write(root);
    char* ret = new char[str.length()+1];
    strcpy(ret, str.c_str());
    return ret;
}

void Server::lp(vector<char*>& nicknames, int mod)
{
    cout << "[SERVER] Modifying lifepoints of " << nicknames.size() << "player(s)" << endl;
    char* duplicate, *nickname;
    for(char* str : nicknames)
    {
        toLower(str);
    }
    for(vector<char*>::iterator it=nicknames.begin(); it!=nicknames.end();)
    {
        bool erased = false;
        for(Player* p : players)
        {
            nickname = p->getNickname();
            toLower(nickname);
            if(strcmp(nickname, *it) == 0)
            {
                if(p->isAlive())
                    p->editLifePoint(mod);
                nicknames.erase(it);
                erased = true;
                break;
            }
        }
        if(!erased)
            ++it;
        else if(it == nicknames.end())
            break;
    }
}

char* Server::talkToJSON(char* nickname, char* msg, bool master)
{
    Json::Value root;
    root["cmd"] = "talk";
    root["fromMaster"] = master;
    if(!master)
        root["nickname"] = nickname;
    root["msg"] = msg;
    Json::FastWriter wr;
    string str = wr.write(root);
    char* ret = new char[str.length()+1];
    strcpy(ret, str.c_str());
    delete nickname;
    return ret;
}
