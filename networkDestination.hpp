#pragma once

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>   // for sockaddr_in
#include <arpa/inet.h>    // for inet_addr or inet_pton
#include <unistd.h>       // for close()
#include <fcntl.h>		  // for setting non blokcing

#include <vector>

//define ports to easily change later
#define _SOURCE_PORT_DEST_ (44444)

std::vector<int> listOfDestinations;

//this is threaded
int destinationHandler(){
	std::cout << "destination Handler started" << std::endl;
	int destinationSocket;
	sockaddr_in serverAddressDestination;
	int connectedDestination;
	int rResult = 0;


	destinationSocket = socket(AF_INET, SOCK_STREAM, 0);
	serverAddressDestination.sin_family = AF_INET;
	serverAddressDestination.sin_port = htons(_SOURCE_PORT_DEST_);
	serverAddressDestination.sin_addr.s_addr = INADDR_ANY;


	bind(destinationSocket, (struct sockaddr*)&serverAddressDestination, sizeof(serverAddressDestination));
	std::cout << "bound"<<std::endl;
	listen(destinationSocket, 1024); //except just one connection

	while(true){//main loop to accept and retive data from source
		connectedDestination = accept(destinationSocket, nullptr, nullptr); //accept and store destinatino connected
		listOfDestinations.push_back(connectedDestination);//store destination in list
		std::cout << "destination accepted" <<std::endl;
	}

	std::cout << "hello end!\n";
	return 0;
}