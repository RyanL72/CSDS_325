/*
Author: Ryan Lin
CaseID: rhl72
File name: proj1.cpp
Date: 10-21-2024
Description: IPv4 format checker
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <cstdlib>   // for exit()
#include <string>    // for std::string
#include <cctype>    // for isdigit
#include <stdexcept> // for std::stoi
#include <arpa/inet.h>   // For inet_ntoa()
#include <sstream>
#include <map>
#include <array>

#define REQUIRED_ARGC 6
#define PORT_POS 1
#define MSG_POS 2
#define ERROR 1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define METHOD_POS 0
#define TARGET_POS 1 
#define PROTOCOL_VERS_POS 2

int usage (std::string progname)
{
    std::cout << progname << " is an unknown option, please use the format: \n" <<  "./proj3 -p <Port Number> -r <Root directory> -t <Terminate token>" << std::endl;
    exit (ERROR);
}

int badArgumentValues(std::string message="Bad Argument Value Error"){
    std::cout << message << std::endl;
    std::cout << "Argument values were invalid: \n" <<  "Please use the format: ./proj3 -p <Port Number> -r <Root directory> -t <Terminate token>" << std::endl;
    exit (ERROR);
}

int ArgumentValidation(int port, std::string root, std::string token){
    if(port <= 1024){
    badArgumentValues("Port Numbers less than 1024 are reserved.");
    }
    return 0;
}

int errexit (std::string format, std::string arg)
{
    std::cerr << format << arg << std::endl;
    fprintf (stderr,"\n");
    exit (ERROR);
}


//returns array that has 
//method at 0
//url at 1
//http version at 2
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept> // For exceptions
#include <map>

std::array<std::string, 3> parseHTTPRequest(char buffer[], size_t bufferSize) {
    try {
        // Create a string stream from the buffer
        std::istringstream requestStream(std::string(buffer, bufferSize));
        std::string line;

        // 1. Parse the request line (METHOD ARGUMENT HTTP/VERSION)
        std::getline(requestStream, line);
        if (line.empty()) {
            throw std::invalid_argument("HTTP request must start with a request line. 1");
        }

        // Verify that the request line ends with \r\n
        if (line.back() != '\n' || line[line.size() - 2] != '\r') {
            std::cout << line << std::endl;
            throw std::invalid_argument("Request line must end with \\r\\n. 2");
        }

        std::istringstream requestLineStream(line);
        std::string method, url, httpVersion;
        requestLineStream >> method >> url >> httpVersion;

        // Verify that we got all three parts of the request line
        if (method.empty() || url.empty() || httpVersion.empty()) {
            throw std::invalid_argument("Malformed request line. Must be 'METHOD ARGUMENT HTTP/VERSION'. 3");
        }

        // Store the method, URL, and HTTP version in a fixed-size array
        std::array<std::string, 3> requestInfo = {method, url, httpVersion};

        // 2. Parse headers (arbitrary lines, but should end with \r\n)
        std::map<std::string, std::string> headers;
        while (std::getline(requestStream, line) && line != "\r") {
            if (line.back() != '\n' || line[line.size() - 2] != '\r') {
                throw std::invalid_argument("Each header line must end with \\r\\n. 4");
            }

            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string headerName = line.substr(0, colon);
                std::string headerValue = line.substr(colon + 2); // Skip ": "
                headers[headerName] = headerValue;
            }
        }

        // 3. Check for the blank line at the end of headers
        if (line != "\r") {
            throw std::invalid_argument("HTTP request must end with a blank line (\\r\\n). 5");
        }

        // 4. Optionally parse the body (this example just prints it if present)
        std::string body;
        if (std::getline(requestStream, body)) {
            std::cout << "Body: " << body << std::endl;
        }

        // Return the array containing method, URL, and HTTP version
        return requestInfo;
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid HTTP request: " << e.what() << std::endl;
        // Return an empty array or handle this error as needed
        return {};
    }
}



int main (int argc, char *argv [])
{
    //for ipv4
    struct sockaddr_in sin;
    
    //address for ipv4 or ipv6
    struct sockaddr addr;

    //protocal information 
    struct protoent *protoinfo;

    //length of address
    unsigned int addrlen;

    //socket descriptors 
    int sd, sd2;
    
    //check to see if the right options were provided.
    int portNumber = -1;
    std::string rootDirectory = "";
    std::string terminationToken = "";

    //get the options.
    int opt;
    while((opt = getopt(argc, argv,"p:r:t:")) != -1){
        switch(opt){
            case 'p':
                portNumber = atoi(optarg);
                break;
            case 'r':   
                rootDirectory = optarg;
                break;
            case 't':
                terminationToken = optarg;
                break;
            case '?':
                usage(std::to_string(opt));
                break;
        }
    }
    //check to see if all the port, root directory, and terminate token were provided
    std::cout << portNumber << std::endl;
    std::cout << rootDirectory << std::endl;
    std::cout << terminationToken << std::endl;
    
    //handle missing things
    if(portNumber == -1 || rootDirectory.empty() || terminationToken.empty() ){
        badArgumentValues();
    }

    //validate the port
    ArgumentValidation(portNumber, rootDirectory, terminationToken);

    /* determine protocol */
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for ", PROTOCOL);
    
    std::cout << "Protocol: " << protoinfo->p_name << "\n" << "Number: " << protoinfo->p_proto << std::endl;

    /* setup endpoint info */

    // (char *)&sin means converting memory address of sin into a byte ptr
    // 0x0 is value to set this memory to
    // sizeof (sin) is sockaddr_in size 
    /*
    so this is saying set memory to this address of sin (in the form of a pointer to a 
    char or byte- typically used in memory level access) to the value of 0x0 
    and allocate a size of however big sin is */
    memset ((char *)&sin,0x0,sizeof (sin));

    // AF_INET is for ipv4
    sin.sin_family = AF_INET;

    // INADDR_ANY tells the socket to listen to all available interfaces
    sin.sin_addr.s_addr = INADDR_ANY;

    // host to networks byte order (Big Endian) 
    //Not sure if this should be my port number or what. Consult the socketsd.c for original.
    sin.sin_port = htons (portNumber);
 
    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);
    
    /* bind the socket */
    if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        errexit ("cannot bind to port %s", std::to_string(portNumber));
    }


    /* listen for incoming connections */
    if (listen (sd, QLEN) < 0)
        errexit ("cannot listen on port %s\n", std::to_string(portNumber));

    //check point
    if (protoinfo != NULL) {
        std::cout << "Protocol Name: " << protoinfo->p_name << std::endl;
        std::cout << "Protocol Number: " << protoinfo->p_proto << std::endl;
    } else {
        std::cerr << "Protocol information is missing!" << std::endl;
    }
    std::cout << "Socket Family: " << sin.sin_family << std::endl;
    std::cout << "Socket Address: " << inet_ntoa(sin.sin_addr) << std::endl;  
    std::cout << "Socket Port: " << ntohs(sin.sin_port) << std::endl;  
    std::cout << "Socket Descriptor: " << sd << std::endl;
    if (listen(sd, QLEN) == 0) {
    std::cout << "Server is now listening on port: " << portNumber << std::endl;
    } else {
        std::cerr << "Failed to listen on port: " << portNumber << std::endl;
    }

    /* accept a connection */
    addrlen = sizeof (addr);
    sd2 = accept (sd,&addr,&addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);

    //see what request looks like
    std::cout << "Request Accepted with sd2: " << sd2 << std::endl;
    
    char buffer[BUFLEN]; 
    memset(buffer, 0, sizeof(buffer));  

    // Read the message from the client
    int bytesReceived = recv(sd2, buffer, sizeof(buffer) - 1, 0);  // Read up to buffer size
    if (bytesReceived < 0) {
        std::cerr << "Error reading from client" << std::endl;
    } else {
        // Null-terminate the buffer to treat it as a string
        buffer[bytesReceived] = '\0';
        std::cout << "Client Message: " << buffer << std::endl;
    }

    //parse requeset
    std::array<std::string, 3> requestInfo = parseHTTPRequest(buffer, BUFLEN);
    std::cout << "Method: " << requestInfo[0] << std::endl;
    std::cout << "URL: " << requestInfo[1] << std::endl;
    std::cout << "HTTP Version: " << requestInfo[2] << std::endl;
    exit (0);

    /* write message to the connection */
    if (write (sd2,argv [MSG_POS],strlen (argv [MSG_POS])) < 0)
        errexit ("error writing message: %s", argv [MSG_POS]);

    /* close connections and exit */
    close (sd);
    close (sd2);
    exit (0);
}