#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../common.h"

#define PORT "6969"
#define HOST "localhost"

int get_address(struct addrinfo *server_info) {
  int socket_descriptor;

  // For system call return values
  int return_code;

  // Iterate through the address linked-list until
  // we find one we can get a socket for
  for (; server_info != NULL; server_info = server_info->ai_next) {

    // The call to socket() is the first step in establishing a socket
    // based communication. It takes the following arguments:
    // 1. The address family (PF_INET or PF_INET6)
    // 2. The socket type (SOCK_STREAM or SOCK_DGRAM)
    // 3. The protocol (TCP or UDP)
    // Note that all of these fields will have been already populated
    // by getaddrinfo. If the call succeeds, it returns a valid file descriptor.
    // clang-format off
		socket_descriptor = socket(
			server_info->ai_family,
			server_info->ai_socktype,
			server_info->ai_protocol
		 );
    // clang-format on

    if (socket_descriptor == -1) {
      continue;
    }

    // Once we have a socket, we can connect it to the server's socket.
    // Again, this information we get from the addrinfo struct
    // that was populated by getaddrinfo(). The arguments are:
    // 1. The socket file_descriptor from which to connect.
    // 2. The address to connect to (sockaddr_in struct)
    // 3. The size of this address structure.
    // clang-format off
		return_code = connect(
			socket_descriptor,
			server_info->ai_addr,
			server_info->ai_addrlen
		);
    // clang-format on

    if (return_code == 1) {
      close(socket_descriptor);
    }

    break;
  }

  // If we didn't actually find a valid address
  if (server_info == NULL) {
    throw("Error finding valid address!");
  }

  // Return the valid address info
  return socket_descriptor;
}

void cleanup(int descriptor, void *buffer) {
  close(descriptor);
  free(buffer);
}

void read_data(int descriptor, int bytes) {
  // Buffer into which to read our data
  char *buffer = (char *)malloc(bytes);
  // For benchmarking
  int start;

  *buffer = '1';

  // Send the first message (as part of our protocol)
  send(descriptor, buffer, 1, 0);

  do {
    recv(descriptor, buffer, 1, 0);
  } while (*buffer != '2');

  start = now();
	recv(descriptor, buffer, bytes, 0);
  benchmark(start);

  cleanup(descriptor, buffer);
}

int main(int argc, const char *argv[]) {
  // Address info structs are basic (relatively large) structures
  // containing various pieces of information about a host's address,
  // such as:
  // 1. ai_flags: A set of flags. If we set this to AI_PASSIVE, we can
  //              pass NULL to the later call to getaddrinfo for it to
  //              return the address info of the *local* machine (0.0.0.0)
  // 2. ai_family: The address family, either AF_INET, AF_INET6 or AF_UNSPEC
  //               (the latter makes this struct usable for IPv4 and IPv6)
  //               note that AF stands for Address Family.
  // 3. ai_socktype: The type of socket, either (TCP) SOCK_STREAM with
  //                 connection or (UDP) SOCK_DGRAM for connectionless
  //                 datagrams.
  // 4. ai_protocol: If you want to specify a certain protocol for the socket
  //                 type, i.e. TCP or UDP. By passing 0, the OS will choose
  //                 the most appropriate protocol for the socket type (STREAM
  //                 => TCP, DGRAM = UDP)
  // 5. ai_addrlen: The length of the address. We'll usually not modify this
  //                ourselves, but make use of it after it is populated via
  //                getaddrinfo.
  // 6. ai_addr: The Internet address. This is yet another struct, which
  //             basically contains the IP address and TCP/UDP port.
  // 7. ai_canonname: Canonical hostname.
  // 8. ai_next: This struct is actually a node in a linked list. getaddrinfo
  //             will sometimes return more than one address (e.g. one for IPv4
  //             one for IPv6)
  struct addrinfo hints, *server_info;

  // For system call return values
  int return_code;

  // The bytes to send
  int bytes = get_bytes(argc, argv);

  // Sockets are returned by the OS as standard file descriptors.
  // It will be used for all communication with the server.
  int socket_descriptor;

  // Fill the hints with zeros first
  memset(&hints, 0, sizeof hints);
  // We set to AF_UNSPEC so that we can work
  // with either IPv6 or IPv4
  hints.ai_family = AF_UNSPEC;
  // Stream socket (TCP) as opposed to datagram sockets (UDP)
  hints.ai_socktype = SOCK_STREAM;
  // By setting this flag to AI_PASSIVE we can pass NULL for the hostname
  // in getaddrinfo so that the current machine hostname is implied
  //  hints.ai_flags = AI_PASSIVE;

  // This function will return address information for the given:
  // 1. Hostname or IP address (as string in digits-and-dots notation).
  // 2. The port of the address.
  // 3. The struct of hints we supply for the address.
  // 4. The addrinfo struct the function should populate with addresses
  //    (remember that addrinfo is a linked list)
  return_code = getaddrinfo(HOST, PORT, &hints, &server_info);

  if (return_code != 0) {
    fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(return_code));
    exit(EXIT_FAILURE);
  }

  socket_descriptor = get_address(server_info);

  // Don't need this anymore
  freeaddrinfo(server_info);

  read_data(socket_descriptor, bytes);

  return EXIT_SUCCESS;
}