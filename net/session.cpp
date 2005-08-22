/* Copyright (c) 2005, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include "session.h"
#include "connection.h"
#include "connectionDescription.h"
#include "session.h"
#include "sessionPackets.h"
#include "user.h"

#include <eq/base/log.h>

#include <alloca.h>

using namespace eqNet;
using namespace std;

Session::Session( Node* node, const char* name )
        : _node(node),
          _userID(1)
{
    ASSERT( node );

    for( int i=0; i<CMD_SESSION_ALL; i++ )
        _cmdHandler[i] = &eqNet::Session::_cmdUnknown;

    _cmdHandler[CMD_SESSION_CREATE_USER] = &eqNet::Session::_cmdCreateUser;

    _id = node->createSession( this, name );
    INFO << "New session" << this << endl;
}

void Session::handlePacket( Node* node, const SessionPacket* packet)
{
    INFO << "handle " << packet << endl;

    switch( packet->datatype )
    {
        case DATATYPE_EQ_SESSION:
            ASSERT( packet->command < CMD_SESSION_ALL );
            (this->*_cmdHandler[packet->command])( node, packet );
            break;

        case DATATYPE_EQ_USER:
        {
            UserPacket* userPacket = (UserPacket*)(packet);
            User* user = _users[userPacket->userID];
            ASSERT( user );

            //user->handlePacket( connection, node, userPacket );
        } break;

        default:
            WARN << "Unhandled packet " << packet << endl;
    }
}

void Session::_cmdCreateUser( Node* node, const Packet* pkg )
{
    ASSERT( _node->getState() == Node::STATE_LISTENING );

    SessionCreateUserPacket* packet  = (SessionCreateUserPacket*)pkg;
    INFO << "Cmd create user: " << packet << endl;
 
}

void Session::pack( Node* node ) const
{
    for( IDHash<User*>::const_iterator iter = _users.begin();
         iter != _users.end(); iter++ )
    {
        SessionNewUserPacket newUserPacket;
        newUserPacket.sessionID = getID();
        newUserPacket.userID    = (*iter).first;
        node->send( newUserPacket );
    };
}


std::ostream& eqNet::operator << ( std::ostream& os, Session* session )
{
    if( !session )
    {
        os << "NULL session";
            return os;
        }
    
    os << "    session " << session->getID() << "(" << (void*)session
       << "): " << session->_users.size() << " user[s], ";
    
    for( IDHash<User*>::iterator iter = session->_users.begin();
         iter != session->_users.end(); iter++ )
    {
        User* user = (*iter).second;
        os << std::endl << "    " << user;
    }
    
    return os;
}
