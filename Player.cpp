/* 
 * File:   Player.cpp
 * Author: Anthony Correia <anthony.correia71@gmail.com>
 * 
 * Created on 18 mars 2015, 09:06
 */

#include "Player.h"
#include "Server.h"
#include "Client.h"

#include <cstring>
#include "errcodes.h"

Player::Player(Server* server, int id, int socket)
: Client(server, id, socket)
{
    lifePoints = 10;
}

Player::~Player()
{
}

bool Player::execCmd(char* msg)
{
    bool ret = false;
    char* cmd = extractCmd(msg);
    char *arg1, *arg2;
    if(strcmp(cmd, "/join") == 0)
    {
        ret = join(strtok(NULL, "\r"));
    }
    else if(strcmp("/talk", cmd) == 0)
    {
        if(server->isPlaying())
            ret = talk();
        else
            sendCode(GAME_HAS_NOT_STARTED);
    }
    else if(strcmp("/players", cmd) == 0)
    {
        ret = playersInfo();
    }
    delete msg;
    return ret;
}

bool Player::join(char* nickname)
{
    bool ret = false;
    if(nickname == NULL)
        sendCode(YOU_MUST_CHOOSE_A_NICKNAME);
    else if(!checkChars(nickname))
    {
        sendCode(NICKNAME_NOT_VALID);
    }
    else
    {
        if(server->hasJoined(this))
        {
            sendCode(YOU_ALREADY_JOINED);
        }
        else if(!server->isNicknameAvailable(nickname))
        {
            sendCode(NICKNAME_ALREADY_USED);
        }
        else
        {
            ret = true;
            strcpy(this->nickname, nickname);
            server->joinGame(this);
            sendCode(SUCCESS);
        }
    }
    return ret;
}

char* Player::getNickname() const
{
    char* copy = new char[strlen(nickname)+1];
    strcpy(copy, nickname);
    return copy;
}

int Player::getLifePoint()
{
    return lifePoints;
}