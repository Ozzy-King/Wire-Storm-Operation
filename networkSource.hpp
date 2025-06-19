#pragma once

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>   // for sockaddr_in
#include <arpa/inet.h>    // for inet_addr or inet_pton
#include <unistd.h>       // for close()
#include <fcntl.h>		  // for setting non blokcing

#include "networkDestination.hpp"

//define ports to easily change later
#define _SOURCE_PORT_ (33333)
#define _PACKET_HEADER_SIZE_ (8)
#define _MAX_PACKET_DATA_SIZE_ (65535) //(MAX 16bit number)
#define _MAGIC_NUMBER_ (0xcc)

//single buffer has been split up to make sending faster
#define _BUFFER_HEADER_START_ 0
#define _BUFFER_DATA_START_ 8
char fullPacketBuffer[_PACKET_HEADER_SIZE_ + _MAX_PACKET_DATA_SIZE_];
#define packetHeader (fullPacketBuffer+_BUFFER_HEADER_START_)
#define packetHeaderChecksumIndex 4

#define packetHeaderSensitiveBit (fullPacketBuffer[1]&0b00000010)
#define packetHeaderChecksum ((uint16_t)(packetHeader[4] << 8) | packetHeader[5])

#define packetData (fullPacketBuffer+_BUFFER_DATA_START_)


bool recvMsg(int sock, char* buf, size_t len, int flags) {
	static int counter;
    int n = recv(sock, buf, len, flags);
    if (n > 0) { //if bigger than 0 then message recived
        std::cout << counter++ << ")[recvMsg] Got " << n << " bytes (peeked or read)" << std::endl;
        return false;
    } else if (n == 0) {//if 0 then connection was closed(closes connection)
        std::cout << "[recvMsg] Connection closed by peer." << std::endl;
        return true;
    } else {//if -1 (somehting else happend)
        if (errno == EAGAIN || errno == EWOULDBLOCK) { //check if connection still open(yes)
            std::cout << "[recvMsg] No data available (non-blocking)." << std::endl;
            return false;
        } else {//else an error occured (closes connection)
            perror("[recvMsg] recv error");
            return true;
        }
    }
}

void setNonblocking(int socket){
	int flags = fcntl(socket, F_GETFL, 0);
	fcntl(socket, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(socket, F_GETFL, 0);
	std::cout << "Non-blocking? " << ((flags & O_NONBLOCK) ? "yes" : "no") << std::endl;
}
void setBlocking(int socket){
	int flags = fcntl(socket, F_GETFL, 0);
	fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
	flags = fcntl(socket, F_GETFL, 0);
	std::cout << "blocking? " << ((flags & ~O_NONBLOCK) ? "yes" : "no") << std::endl;
}


int sourceHandler(){
	
	int sourceSocket;
	sockaddr_in serverAddressSource;
	int connectedSource;
	int rResult = 0;


	sourceSocket = socket(AF_INET, SOCK_STREAM, 0);
	serverAddressSource.sin_family = AF_INET;
	serverAddressSource.sin_port = htons(_SOURCE_PORT_);
	serverAddressSource.sin_addr.s_addr = INADDR_ANY;


	bind(sourceSocket, (struct sockaddr*)&serverAddressSource, sizeof(serverAddressSource));
	std::cout << "bound"<<std::endl;
	listen(sourceSocket,1024); //except just one connection

	while(true){//main loop to accept and retive data from source
		std::cout << "listening for connection" << std::endl;
		connectedSource = accept(sourceSocket, nullptr, nullptr); //accept and store source client
		std::cout << "clinet connected" << std::endl;

		bool RecvHost = true;
		//recie messages until the connection closes or error occures
		while( RecvHost ){ 
			char tempByte[1]; //assume 0xcc is unique and wont appear otherwise
			if(recvMsg(connectedSource, tempByte, 1, MSG_PEEK)){ RecvHost = false; continue; }
			while(tempByte[0] != (char)_MAGIC_NUMBER_){
				if(recvMsg(connectedSource, tempByte, 1, 0)){ RecvHost = false; break;  }//remove
				if(recvMsg(connectedSource, tempByte, 1, MSG_PEEK)){ RecvHost = false; break;  }//peek next byte
			}
			if(!RecvHost){continue;}//if broken skip to stop loop

			//gets the inital 8byte header and parse length
			if(recvMsg(connectedSource, packetHeader, _PACKET_HEADER_SIZE_, 0)){ RecvHost = false; continue;  }//get inital header
			int msgLen = (packetHeader[2] << 8) | packetHeader[3];
			int fullPacketLen = msgLen + _BUFFER_HEADER_START_;

			//get the amount of set in header
			std::cout << "data length: " << msgLen << std::endl;
			if(recvMsg(connectedSource, packetData, msgLen, 0)){ RecvHost = false; continue; }//get data amount of data spesifiyed in header

			//set nonblocking and attempt to get one more byte of data to check if the there is too much data
			tempByte[0] = {(char)_MAGIC_NUMBER_};
			setNonblocking(connectedSource);
			if(recvMsg(connectedSource, tempByte, 1, MSG_PEEK)){ RecvHost = false; continue; }//peek next header(validates if the message is correct)
			setBlocking(connectedSource);
			if(tempByte[0] != (char)_MAGIC_NUMBER_){//if data was written then there is too much data
				continue; //continue to next message to handle
			}
			std::cout << "real packet" << std::endl;

			//print packet
			std::cout << "thing: " << std::endl;
			for(int i = 0; i < fullPacketLen; i++){
				std::cout << std::hex << (uint16_t)fullPacketBuffer[i] << ":";
			}
			std::cout << std::endl;

			bool secure = false;
			//if the packets real check sensitivity bit
			std::cout << std::hex << "checksum: " << (uint16_t)((packetHeader[4] << 8) | packetHeader[5]) << std::endl;
			if((fullPacketBuffer[1] & 0b00000010) == 0){//if sensitive bit is 0 (not set)
				secure = true;
				std::cout << "secure bit not secure: " << packetHeaderSensitiveBit << std::endl;
			}
			else{//if bit is set
				uint32_t checksumTest = 0;
				int i;
				for(i = 0; i < (fullPacketLen/2)*2; i+=2){
					uint16_t wordValue = 0;
					if(i == packetHeaderChecksumIndex){ wordValue = 0xcccc; }
					else{ wordValue = fullPacketBuffer[i] << 8 | fullPacketBuffer[i+1]; }
					checksumTest += wordValue;
				}
				if(fullPacketLen & 0x01){ //if the length is odd, need to add the last byte for the check sum
					checksumTest += fullPacketBuffer[fullPacketLen - 1] << 8;
				}
				while(checksumTest >> 16){
					checksumTest = (checksumTest & 0xFFFF) + (checksumTest >> 16);
 				}
 				checksumTest = ~checksumTest;

				std::cout << std::hex << "checksum: " << (uint16_t)((packetHeader[4] << 8) | packetHeader[5]) << " - cal Checksum: " << checksumTest << std::endl;
				if(checksumTest == ((packetHeader[4] << 8) | packetHeader[5])){
					secure = true;
					std::cout << "secure" <<std::endl;
				}
			}
			if(!secure){
				std::cout << "not secure" << std::endl;
				continue;
			}


			//if real broadcast to all destinations
			for(int i = listOfDestinations.size()-1; i >= 0; i--){
				//wucik check socket open (from chat)
				int error = 0;
				socklen_t len = sizeof(error);
				int sockfd = listOfDestinations[i];
				int retval = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
				if (retval != 0 || error != 0) {
				    std::cerr << "Socket " << sockfd << " is not valid. getsockopt error=" << error << std::endl;
				    // Close and remove socket from list
				    listOfDestinations.erase(listOfDestinations.begin()+(i) );
				    continue;
				}

				std::cout << "\nsending to socket: " << listOfDestinations[i] << "at index " << i << std::endl;
				int res = send(listOfDestinations[i], fullPacketBuffer, msgLen+_PACKET_HEADER_SIZE_, 0);
				std::cout << "sent result: " << res<< std::endl ; 
				if(res < 0){ //dirty check //if borken remove from list
					std::cout << "removeing" << std::endl;
					listOfDestinations.erase(listOfDestinations.begin()+(i) );
				}
			}
		}

		close(connectedSource);
	}

	std::cout << "hello end!\n";
	return 0;
}