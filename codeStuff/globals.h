// We define the ports as values other than "20" and "21" because they were privileged. Using "sudo" when running the code will allow to use those port numbers

#define CONTROL_PORT 1026
#define DATA_PORT 1025

// We wanted to divide the content of the data into packets, so as to send information in chunks

#define PACKET_SIZE 47