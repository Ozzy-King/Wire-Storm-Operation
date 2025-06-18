#include <iostream>
#include <thread>
#include "networkDestination.hpp"
#include "networkSource.hpp"

int main(){

	std::thread destHandleThead(destinationHandler);

	sourceHandler();
	

	std::cout << "hello end!\n";
	return 0;
}