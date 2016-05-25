/*
 * MessengerClient.cpp
 *
 *  Created on: Apr 10, 2016
 *      Author: parallels
 */

#include <map>
#include "MessengerClient.h"

using namespace npl;

MessengerClient::MessengerClient(){
	this->clientState = State::DISCONNECTED;
	this->currentRoomName = "";
    this->peersInRoom = new map<string,string>();
    this->peerInSeesion = NULL;
    this->udpPeer = NULL;
}
void MessengerClient::run() {
    //Main server reader thread
	while (clientState == State::CONNECTED) {
		int cmd;
		string str;
		readFromServer(cmd,str,mainServer);
		if (cmd > 0)
		{
            if (cmd == SUCCESS_LOGIN) {
                //Expecting input in format: <user>

                this->clientState = State::LOGGED_IN;
                this->currentUserName = str;
                cout << "You successfully logged in user: ["<<str<<"]"<<endl;
            }
            else if (cmd == SUCCESS_REGISTER){
                //Expecting input in format: <user>

                this->clientState = State::LOGGED_IN;
                this->currentUserName = str;

                cout << "You successfully registered and logged in user ["<<str<<"]"<<endl;
            }
            else if (cmd == LOGIN_REFUSE) {
                cout << "Login was refused by server, check your username and/or password"<<endl;
            }
            else if (cmd == REGISTER_REFUSE) {
                cout << "Register was refused by server, your username exists already"<<endl;
            }
			else if (cmd == SESSION_ESTABLISHED)
			{
                //Expecting input in format: <user> <ip>:<port>

                this->clientState = State::IN_SESSION;

                std::istringstream splitter(str);
                string peerUser;
                string peerIpAndPort;
                splitter >> peerUser;
                splitter >> peerIpAndPort;

                cout << SESSION_ESTABLISHED_TEXT << "["<< peerUser<<"]" << endl;
                this->peerInSeesion = new pair<string,string>(peerUser,peerIpAndPort);
                //TODO: OPEN UDP IN READER THREAD ON THE PORT WHICH I CONNECTED TO THE MAIN SERVER
                this->udpPeer = new UDPSocket();
			}else if (cmd == SUCCESS_ENTER_ROOM){
                //Expecting input in format: <roomName>

                this->clientState = State::IN_ROOM;
                this->currentRoomName = str;

                //TODO: OPEN UDP IN READER THREAD ON THE PORT WHICH I CONNECTED TO THE MAIN SERVER
                this->udpPeer = new UDPSocket();
            }
            else if (cmd == SESSION_REFUSED) {
                //Expecting input in format: <user>
                cout << SESSION_REFUSED_TEXT << "["<< str <<"]" << endl;
            }
            else if (cmd == CLOSE_SESSION_WITH_PEER){
                //Expecting input in format: NONE
                cout << CLOSE_SESSION_WITH_PEER_TEXT << "["<< this->peerInSeesion->first <<"]" << endl;
                this->clientState = State::CONNECTED;
                this->peerInSeesion = NULL;
            }
            else if (cmd == CLIENT_DISCONNECTED_FROM_ROOM){
                //Expecting input in format: <user>

                map<string,string>::iterator iter = this->peersInRoom->find(str);
                if(iter != this->peersInRoom->.end())
                    this->peersInRoom->erase(iter);
                else
                    cout << "Unsuccessful delete of user: ["<<str<<"] from room"<<endl;

            }
            else if (cmd == CLIENT_ENTERED_ROOM){
                //Expecting input in format: <user> <ip>:<port>

                std::istringstream splitter(str);
                string peerUser;
                string peerIpAndPort;
                splitter >> peerUser;
                splitter >> peerIpAndPort;

                this->peersInRoom->insert(make_pair(peerUser,peerIpAndPort));
                cout << "User ["<<str<<"]"<<" has entered the room"<<endl;
            }
            else if (cmd == CHAT_CLOSED_BY_ADMIN){
                this->peersInRoom->clear();
                this->clientState = State::LOGGED_IN;

                cout << "Chat room closed by admin"<<endl;
            }
		}
		else
		{
			cout << "Server disconnected" << endl;
			this->clientState = State::DISCONNECTED;
		}

	}
}
void MessengerClient::connect(const string& ip){
	if(clientState == State::DISCONNECTED){
		mainServer = new TCPSocket(ip, MSNGR_PORT);
		if(mainServer != NULL){
			clientState = State::CONNECTED;
			start();
		}
	}
    else
    {
        cout << "You are already connected to a server"<<endl;
    }
}

void MessengerClient::login(const string& user,const string& password) {
    if (clientState == State::CONNECTED){
        sendToServer(LOGIN,user + " " + password,mainServer);
    }
    else{
        cout << "You are not connected to a server" << endl;
    }
}
void MessengerClient::reg(const string& user,const string& password) {
    if (clientState == State::CONNECTED){
        sendToServer(REGISTER,user + " " + password,mainServer);
    }
    else{
        cout << "You are not connected to a server" << endl;
    }
}

void MessengerClient::listUsers(){
    if (clientState == State::LOGGED_IN){
        sendToServer(LIST_USERS,"",mainServer);
    }
    else{
        cout << "You are not logged in" << endl;
    }
}
void MessengerClient::listConnectedUsers(){
    if (clientState == State::LOGGED_IN){
        sendToServer(LIST_CONNECTED_USERS,"",mainServer);
    }
    else{
        cout << "You are not logged in" << endl;
    }
}
void MessengerClient::listRooms(){
    if (clientState == State::LOGGED_IN){
        sendToServer(LIST_ROOMS,"",mainServer);
    }
    else{
        cout << "You are not logged in" << endl;
    }
}
void MessengerClient::listRoomUsers(const string& roomName){
    if (clientState == State::LOGGED_IN){
        sendToServer(LIST_ROOM_USERS,roomName,mainServer);
    }
    else{
        cout << "You are not logged in" << endl;
    }
}
void MessengerClient::openOrConnectToRoom(const string& roomName){
    if (clientState == State::LOGGED_IN){
        sendToServer(OPEN_OR_CONNECT_TO_ROOM,roomName,mainServer);
    }
    else{
        cout << "You are not logged in" << endl;
    }
}
void MessengerClient::printClientState(){
    if (clientState == State::IN_ROOM)
        cout << "Client in room ["<<this->currentRoomName<<"]"<<endl;
    else if (clientState == State::IN_SESSION)
        cout << "Client is in session"<<endl;
    else if (clientState == State::LOGGED_IN)
        cout << "Client is logged in to server ["<<this->mainServer->fromAddr()<<"]"<<endl;
    else if (clientState == State::CONNECTED)
        cout << "Client is connected to server ["<<this->mainServer->fromAddr()<<"]"<<endl;
    else if (clientState == State::DISCONNECTED)
        cout << "Client is not connected to any server"<<endl;
}
void MessengerClient::closeCurrentRoom(){
    if (clientState == State::IN_ROOM){
        sendToServer(CLOSE_ROOM,this->currentRoomName,mainServer);
    }
    else{
        cout << "You are not in a room" << endl;
    }
}

void MessengerClient::openSession(const string& ipAndPort){
	if(clientState == State::LOGGED_IN){
		sendToServer(OPEN_SESSION_WITH_PEER,ipAndPort,mainServer);
	}
	else
		cout<<"Not logged in to server"<<endl;
}
void MessengerClient::closeSessionOrExitRoom(){
	if(clientState == State::IN_SESSION){
		sendToServer(CLOSE_SESSION_WITH_PEER,"",mainServer);
	}else if (clientState == State::IN_ROOM)
        sendToServer(DISCONNECT_FROM_ROOM,this->currentRoomName,mainServer);
	else
		cout<<"Not in session or room"<<endl;
}
void MessengerClient::disconnect(){
    if (clientState != State::DISCONNECTED)
        sendToServer(EXIT,"",mainServer);
    else
        cout<<"Client is already disconnected"<<endl;

}
void MessengerClient::exit(){
    if (clientState != State::DISCONNECTED)
	    sendToServer(EXIT,"",mainServer);
	cout<<"Disconnected from server"<<endl;
	delete mainServer;
}
void MessengerClient::sendToServer(int command, const string& data, TCPSocket* mainServer){
	TCPMessengerProtocol::sendToServer(command,data,mainServer);
}
void MessengerClient::readFromServer(int & command,string& data,TCPSocket* mainServer){
	TCPMessengerProtocol::readFromServer(command,data,mainServer);
}
void MessengerClient::send(const string & msg){

    std::istringstream splitter(this->peerInSeesion->second);
    string peerIp;
    int peerPort;
    splitter >> peerIp;
    splitter >> peerPort;

    if (clientState == State::IN_SESSION) {
        //send peer in session the msg
        this->udpPeer->sendTo(">["+currentUserName+"] " + msg, peerIp, peerPort);
    }
    else if (clientState == State::IN_ROOM) {
        //iterate over all peers in room and send msg
        for (map<string, string>::iterator peer = peersInRoom->begin(); peer != peersInRoom->end(); ++peer)
        {
            splitter.clear();
            splitter.str(peer->second);
            splitter >> peerIp;
            splitter >> peerPort;

            this->udpPeer->sendTo(">["+currentUserName+"] " + msg, peerIp, peerPort);
        }
    }
    else{
        cout << "You are not in a room or a session"<<endl;
    }
}





