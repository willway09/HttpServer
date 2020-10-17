#include "Server.hpp"

#include "ServerException.hpp"

//C compatible headers
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include <thread>
#include <chrono>


std::string serveFile(const std::string& fileName, int client_val) { //Return the main web page, for user interfacing

	std::cout << fileName << std::endl;


	std::ifstream file(fileName.substr(1, fileName.length() - 1), std::fstream::binary);



	if(file.is_open()) { //If the file opened successfully


		file.seekg(0, file.end);

		int length = file.tellg();

		file.seekg(0, file.beg);

		std::cout << "Length: " << length << std::endl;

		//write(client_val, response.c_str(), response.length()); //Send response to the client

		write(client_val, "HTTP/1.1 200 OK\r\n", std::strlen("HTTP/1.1 200 OK\r\n"));
		std::string lengthStr = "Content-Length: " + std::to_string(length) + "\r\n";
		write(client_val, lengthStr.c_str(), lengthStr.length());

		std::string typeStr = "Content-Type: ";
		if(fileName.find(".html") != -1) {
			typeStr += "text/html";
		} else if(fileName.find(".js") != -1) {
			typeStr += "text/js";
		} else if(fileName.find(".css") != -1) {
			typeStr += "text/css";
		} else {
			typeStr += "text/plain";
		}
		typeStr += "\r\n";
		write(client_val, typeStr.c_str(), typeStr.length());

		write(client_val, "\r\n", 2);


		int bufferSize = 1000;

		char* buffer = new char[bufferSize];

		for(int i = 0; i < length / bufferSize; i++) {
			file.read(buffer,bufferSize);
			write(client_val, buffer, 1);
			//std::cout << buffer[0];
		}

		file.read(buffer, length % bufferSize);
		write(client_val, buffer, length % bufferSize);


		delete buffer;

		file.close();

	} else { //Else send error message

		std::string resonse = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
		write(client_val, resonse.c_str(), resonse.length());
	}

	return "";

}

std::string errorPage() { //Return a basic HTTP 404 Error page
	std::string responseContent = "404 NOT FOUND";
	std::string response = "HTTP/1.1 404 Not Found\r\n";
	response += "Content-Length: " + std::to_string(responseContent.length()) + "\r\n";
	response += "\r\n";
	response += responseContent;

	return response;
}

std::string evaluateRequest(std::string file, std::unordered_map<std::string, std::string>& params, void* data) {
	return serveFile(file, *((int*)data));
}


void Server::registerSocket(unsigned int port, unsigned int connectionCount) {
  //Declare variables for sockets
  struct sockaddr_in local;

  //Create TCP socket
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  //Check failure condition
  if(serverSocket == -1) {
    throw ServerException("Socket creation failed. Error number: " + std::to_string(errno));
  }


  //Clear memory of local
  std::memset(&local, 0, sizeof(local));

  local.sin_family = AF_INET; //Internet socket
  local.sin_addr.s_addr = htonl(INADDR_ANY); //Listen on any IP address
  local.sin_port = htons(8000); //Open on port 8000


  //Bind socket
  int success = bind(serverSocket, (struct sockaddr*) &local, sizeof(local));
  if(success == -1) {
    throw ServerException("Socket binding failed. Error number: " + std::to_string(errno));
  }

  //Set socket to listen to up to three connections
  success = listen(serverSocket, connectionCount);
  if(success == -1) {
    throw ServerException("Creating passive listening socket failed. Error number: " + std::to_string(errno));
  }
}

Server::Server(unsigned int port) {
  registerSocket(port);

  handlerThread = std::thread(&Server::handle, this);

}

void Server::handle() {
  //Main loop
  while(true) {

    //Declare client
    struct sockaddr_in client;
    unsigned int clientLength = sizeof(client);

    //Accept connection to client
    //Note that accept() internally prevents the infinite while loop from spinlocking the CPU
    int client_val = accept(serverSocket, (struct sockaddr*) &client, &clientLength);



    //Declare buffer of size and clear its contents
    int buffSize = 101; //101 for 100 characters and \0 terminator
    char buffer[buffSize];
    std::memset(buffer, '\0', buffSize);


    //Populate request string based on HTTP content of values read from client connection
    std::string request;
    for(int count = read(client_val, buffer, buffSize - 1); count > 0; count = read(client_val, buffer, buffSize - 1)) { //buffSize - 1, to maintain \0 terminator

      //If buffer isn't completely full, add \0 terminator after last byte of data
      if(count != buffSize - 1) {
        buffer[count] = '\0';
      }

      request += buffer; //Append buffer to request string


      //Break loop when \r\n\r\n string is encountered
      //Break is necessary because otherwise read() will never return, because the socket remains open
      if(request[request.length() - 4] == '\r' && request[request.length() - 3] == '\n' && request[request.length() - 2] == '\r' && request[request.length() - 1] == '\n')
        break;

    }

    //use request's c_str for quicker char access
    const char* requestC_str = request.c_str();


    //In HTTP, the first line always looks like
    //<METHOD> <URL> HTTP/<VERSION>
    //Thus, the URL is always contained between the first and second spaces

    //Identify indeces of first and second spaces
    int firstSpace = -1;
    int secondSpace = -1;

    for(int i = 0; i < request.length(); i++) {
      if(requestC_str[i] == ' ') {
        if(firstSpace == -1) {
          firstSpace = i;
        } else {
          secondSpace = i;
          break;
        }
      }
    }


    //Identify URL from substring between spaces
    std::string URL = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);


    //Declare a hashmap to contain the variable/value pairs contained in the URL
    //These pairs form the basis of the REST-style interface
    //A REST interface was chosen to be consistent with those of other products in the VUE,
    //namely the WebRelays
    std::unordered_map<std::string, std::string> params;

    //URLs are of the form
    // /path/to/file.html?var1=a&var2=b&var3=c&...&varX=x

    //Question mark separates file component of URL from parameter component
    int questionPos = URL.find("?");

    //Declare string to hold file component of URL
    std::string file;

    if(questionPos != std::string::npos) { //If the string contains a question mark
      //File component is everything before the question mark
      file = URL.substr(0, questionPos);

      //Declar variables to hold positions between distinct parameters
      int parStart = questionPos + 1;
      int parEnd = 0;

      while(parStart < URL.length()) {
        int parEnd = URL.find('&', parStart); //Identify & inidcating end of parameter, after the end of the last parameter
        if(parEnd == std::string::npos) //If it is not in the string, this must be the last parameter
          parEnd = URL.length();

        std::string par = URL.substr(parStart, parEnd - parStart); //Get parameter substring

        int eqPos = par.find("="); //Equal sign separarates variable from value

        if(eqPos != std::string::npos) { //Minor error handling, ensure that parameter string contains =
          std::string variable = par.substr(0, eqPos); //Set variable to equal variable component of parameter
          std::string value = par.substr(eqPos + 1, par.length() - eqPos); //Set value to equal value component of parameter
          params.emplace(variable, value); //Add variable, value pair to parameter hashmap
        }

        parStart = parEnd + 1; //Set the parameter start index for the next iteration to be 1 after this iteration's end index

      }
    } else { //Else the string does not contain a question mark, thus only contains a file
      file = URL;
    }



    std::string response = evaluateRequest(file, params, &client_val); //Call evaluate request, which handles the HTTP request and returns response content

    //write(client_val, response.c_str(), response.length()); //Send response to the client

    close(client_val); //Close the client
  }
}

void Server::addHandle(std::string handle, void (*function)(void*)) {

}
