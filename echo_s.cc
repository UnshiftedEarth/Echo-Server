// **************************************************************************************
// * Echo Strings (echo_s.cc)
// * -- Accepts TCP connections and then echos back each string sent.
// **************************************************************************************
#include "echo_s.h"

// **************************************************************************************
// * processConnection()
// * - Handles reading the line from the network and sending it back to the client.
// * - Returns 1 if the client sends "QUIT" command, 0 if the client sends "CLOSE".
// **************************************************************************************
int processConnection(int sockFd) {

  int quitProgram = 0;
  int keepGoing = 1;
  while (keepGoing) {
    DEBUG << "Waiting for new data" << ENDL;
    //
    // Call read() call to get a buffer/line from the client.
    // Hint - don't forget to zero out the buffer each time you use it.
    // 
    char buffer[1024];
    bzero(buffer, sizeof(buffer));
    if (read(sockFd, buffer, sizeof(buffer)) == -1) {
      DEBUG << "Error Reading From Connection: " << strerror(errno) << ENDL;
      return 1;
    }
    DEBUG << "Called read(" << sockFd << "," << &buffer << "," << sizeof(buffer) << ")" << ENDL;


    /* remove the excess (unused) bytes from the received message buffer */
    std::string temp = std::string(buffer, 1024);
    int bufferLen = temp.find('\0');
    temp = temp.substr(0, bufferLen);
    char received[temp.length()+1];
    strcpy(received, temp.c_str());


    std::string quitCommand = "QUIT";
    std::string closeCommand = "CLOSE";
    std::string command = std::string(buffer, 5);
    if (command.substr(0, 4).compare(quitCommand) == 0) {
      // the user typed the quit command close connection and terminate program
      quitProgram = 1;
      keepGoing = 0;
      DEBUG << "Recieved QUIT command" << ENDL;
    }
    else if (command.compare(closeCommand) == 0) {
      // the user typed the close command close connection and wait for another
      keepGoing = 0;
      DEBUG << "Recieved CLOSE command" << ENDL;
    }
    else {
      DEBUG << "Recieved " << sizeof(received) << " bytes which are: " << received;
      //
      // Call write() to send line back to the client.
      //
      if (write(sockFd, received, sizeof(received)) == -1) {
        DEBUG << "Error Writing To Connection: " << strerror(errno) << ENDL;
        return 1;
      }
      DEBUG << "Called write(" << sockFd << "," << &buffer << "," << sizeof(received) << ")" << ENDL;
      DEBUG << "Wrote " << sizeof(received) << " back to client" << ENDL;
    }
    

  }

  return quitProgram;
}
    


// **************************************************************************************
// * main()
// * - Sets up the sockets and accepts new connection until processConnection() returns 1
// **************************************************************************************

int main (int argc, char *argv[]) {

  // ********************************************************************
  // * Process the command line arguments
  // ********************************************************************
  boost::log::add_console_log(std::cout, boost::log::keywords::format = "%Message%");
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);

  // ********************************************************************
  // * Process the command line arguments
  // ********************************************************************
  int opt = 0;
  while ((opt = getopt(argc,argv,"v")) != -1) {
    
    switch (opt) {
    case 'v':
      boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -v" << std::endl;
      exit(-1);
    }
  }

  // *******************************************************************
  // * Creating the inital socket is the same as in a client.
  // ********************************************************************
  int listenFd = -1;
       // Call socket() to create the socket you will use for lisening.
  DEBUG << "Calling Socket() assigned file descriptor " << listenFd << ENDL;
  if ((listenFd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    DEBUG << "Error creating socket: " << strerror(errno) << ENDL;
    return -1;
  }

  
  // ********************************************************************
  // * The bind() and calls take a structure that specifies the
  // * address to be used for the connection. On the cient it contains
  // * the address of the server to connect to. On the server it specifies
  // * which IP address and port to listen for connections.
  // ********************************************************************
  struct sockaddr_in servaddr;
  srand(time(NULL));
  int port = (rand() % 10000) + 1024;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = PF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  socklen_t sockLength = sizeof(servaddr);

  // ********************************************************************
  // * Binding configures the socket with the parameters we have
  // * specified in the servaddr structure.  This step is implicit in
  // * the connect() call, but must be explicitly listed for servers.
  // ********************************************************************
  DEBUG << "Calling bind(" << listenFd << "," << &servaddr << "," << sizeof(servaddr) << ")" << ENDL;
  int bindSuccesful = 0;
  while (!bindSuccesful) {
    // You may have to call bind multiple times if another process is already using the port
    // your program selects.
    if (bind(listenFd, (sockaddr *) &servaddr, sockLength) == -1) {
      DEBUG << "Socket Error: " << strerror(errno) << ENDL;
      port += 1;
      continue;
    }
    bindSuccesful = 1;
  }

  std::cout << "Using port " << port << std::endl;


  // ********************************************************************
  // * Setting the socket to the listening state is the second step
  // * needed to being accepting connections.  This creates a queue for
  // * connections and starts the kernel listening for connections.
  // ********************************************************************
  int listenQueueLength = 1;
  DEBUG << "Calling listen(" << listenFd << "," << listenQueueLength << ")" << ENDL;
  if (listen(listenFd, listenQueueLength) == -1) {
    DEBUG << "Listening for connection failed: " << strerror(errno) << ENDL;
  }

  // ********************************************************************
  // * The accept call will sleep, waiting for a connection.  When 
  // * a connection request comes in the accept() call creates a NEW
  // * socket with a new fd that will be used for the communication.
  // ********************************************************************
  int quitProgram = 0;
  while (!quitProgram) {
    int connFd = 0;

    DEBUG << "Calling accept(" << listenFd << "," << &servaddr << "," << sockLength << ")" << ENDL;

    // The accept() call checks the listening queue for connection requests.
    // If a client has already tried to connect accept() will complete the
    // connection and return a file descriptor that you can read from and
    // write to. If there is no connection waiting accept() will block and
    // not return until there is a connection.

    if ((connFd = accept(listenFd, (sockaddr *) &servaddr, &sockLength)) == -1) {
      DEBUG << "Accept Failed: " << strerror(errno) << ENDL;
    }

    DEBUG << "We have recieved a connection on " << connFd << ENDL;

    
    quitProgram = processConnection(connFd);
   
    close(connFd);
  }

  close(listenFd);

}
