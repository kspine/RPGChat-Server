/* 
 * File:   GameMaster.h
 * Author: Anthony Correia <anthony.correia71@gmail.com>
 *
 * Created on 18 mars 2015, 09:06
 */

#ifndef GAMEMASTER_H
#define	GAMEMASTER_H

#include "Client.h"


class GameMaster : public Client
{
public:
    GameMaster();
    GameMaster(const GameMaster& orig);
    virtual ~GameMaster();
private:

};

#endif	/* GAMEMASTER_H */
