#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define BUF_SIZE 128

#define MAX_AUCTIONS 5
#ifndef VERBOSE
#define VERBOSE 0
#endif

#define ADD 0
#define SHOW 1
#define BID 2
#define QUIT 3

/* Auction struct - this is different than the struct in the server program
 */
struct auction_data {
    int sock_fd;
    char item[BUF_SIZE];
    int current_bid;
};

/* Displays the command options available for the user.
 * The user will type these commands on stdin.
 */

void print_menu() {
    printf("The following operations are available:\n");
    printf("    show\n");
    printf("    add <server address> <port number>\n");
    printf("    bid <item index> <bid value>\n");
    printf("    quit\n");
}

/* Prompt the user for the next command 
 */
void print_prompt() {
    printf("Enter new command: ");
    fflush(stdout);
}


/* Unpack buf which contains the input entered by the user.
 * Return the command that is found as the first word in the line, or -1
 * for an invalid command.
 * If the command has arguments (add and bid), then copy these values to
 * arg1 and arg2.
 */
int parse_command(char *buf, int size, char *arg1, char *arg2) {
    int result = -1;
    char *ptr = NULL;
    if(strncmp(buf, "show", strlen("show")) == 0) {
        return SHOW;
    } else if(strncmp(buf, "quit", strlen("quit")) == 0) {
        return QUIT;
    } else if(strncmp(buf, "add", strlen("add")) == 0) {
        result = ADD;
    } else if(strncmp(buf, "bid", strlen("bid")) == 0) {
        result = BID;
    } 
    ptr = strtok(buf, " "); // first word in buf

    ptr = strtok(NULL, " "); // second word in buf
    if(ptr != NULL) {
        strncpy(arg1, ptr, BUF_SIZE);
    } else {
        return -1;
    }
    ptr = strtok(NULL, " "); // third word in buf
    if(ptr != NULL) {
        strncpy(arg2, ptr, BUF_SIZE);
        return result;
    } else {
        return -1;
    }
    return -1;
}

/* Connect to a server given a hostname and port number.
 * Return the socket for this server
 */
int add_server(char *hostname, int port) {
        // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }
    
    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    struct addrinfo *ai;
    
    /* this call declares memory and populates ailist */
    if(getaddrinfo(hostname, NULL, NULL, &ai) != 0) {
        close(sock_fd);
        return -1;
    }
    /* we only make use of the first element in the list */
    server.sin_addr = ((struct sockaddr_in *) ai->ai_addr)->sin_addr;

    // free the memory that was allocated by getaddrinfo for this list
    freeaddrinfo(ai);

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        return -1;
    }
    if(VERBOSE){
        fprintf(stderr, "\nDebug: New server connected on socket %d.  Awaiting item\n", sock_fd);
    }
    return sock_fd;
}
/* ========================= Add helper functions below ========================
 * Please add helper functions below to make it easier for the TAs to find the 
 * work that you have done.  Helper functions that you need to complete are also
 * given below.
 */

/* Print to standard output information about the auction
 */
void print_auctions(struct auction_data *a, int size) {
    printf("Current Auctions:\n");

    /* TODO Print the auction data for each currently connected 
     * server.  Use the follosing format string:
     *     "(%d) %s bid = %d\n", index, item, current bid
     * The array may have some elements where the auction has closed and
     * should not be printed.
     */
     for (int index = 0; index < size; index++) {
        if(a[index].sock_fd != -1){
            printf("(%d) %s bid = %d\n", index, a[index].item, a[index].current_bid);
        }
    
    }
    
    
}

/* Process the input that was sent from the auction server at a[index].
 * If it is the first message from the server, then copy the item name
 * to the item field.  (Note that an item cannot have a space character in it.)
 */
void update_auction(char *buf, int size, struct auction_data *a, int index) {
    // TODO: Complete this function

    char item[size];
    int highest_bid;
    long tv_sec;
    if(sscanf(buf, "%s %d %ld", item, &highest_bid, &tv_sec) != 3){
        fprintf(stderr, "ERROR malformed bid: %s", buf);
    }
    item[size - 1] = '\0';
    if(a[index].item[0] == '\0'){
        strncpy(a[index].item, item, BUF_SIZE);
        a[index].item[BUF_SIZE - 1] = '\0';
    }
    a[index].current_bid = highest_bid;
    printf("\nNew bid for %s [%d] is %d (%ld seconds left)\n", item, index, highest_bid, tv_sec);
    // fprintf(stderr, "ERROR malformed bid: %s", buf);
    // printf("\nNew bid for %s [%d] is %d (%d seconds left)\n",           );
    
}


int main(void) {

    char name[BUF_SIZE];

    // Declare and initialize necessary variables
    // TODO
    char buf[BUF_SIZE];
  
    struct auction_data auctions[MAX_AUCTIONS];
    for (int index = 0; index < MAX_AUCTIONS; index++) {
        auctions[index].sock_fd = -1;
        auctions[index].item[0] = '\0';
        auctions[index].current_bid = -1;
    }

    // Get the user to provide a name.
    printf("Please enter a username: ");
    fflush(stdout);
    int num_read = read(STDIN_FILENO, name, BUF_SIZE);
    
    if(num_read <= 0){
        fprintf(stderr, "ERROR: read from stdin failed\n");
        exit(1);
    }
    name[num_read] = '\0';
    print_menu();

    // TODO
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    int max_fd = STDIN_FILENO;
   
    while(1) {
        print_prompt();
        // TODO
        char *arg1 = malloc(BUF_SIZE);
        char *arg2 = malloc(BUF_SIZE);
       
        fd_set listen_fds = all_fds;
        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) == -1) {
            perror("server: select");
            exit(1);
        }
        if(FD_ISSET(STDIN_FILENO, &listen_fds)){ // if stdin is ready
            int num_read = read(STDIN_FILENO, buf, BUF_SIZE);
            buf[num_read] = '\0';
            if(num_read <= 0){
                fprintf(stderr, "ERROR: read from stdin failed\n");
                exit(1);
            }
            int command = parse_command(buf, BUF_SIZE, arg1, arg2);
            
    
            if(command == SHOW){
                print_auctions(auctions, MAX_AUCTIONS);
            }
            else if(command == QUIT){
                for (int index = 0; index < MAX_AUCTIONS; index++) {
                    int auction_fd = auctions[index].sock_fd;
                    if(auction_fd!= -1){
                        close(auction_fd);
                    }  
                }
                free(arg1);
                free(arg2);
                exit(0);
            }
            else if(command == ADD){ // add <server address> <port number>
                int server_index = 0;
                while (server_index < MAX_AUCTIONS && auctions[server_index].sock_fd == -1){
                    server_index += 1;
                }
                if (server_index == MAX_AUCTIONS){
                     fprintf(stderr, "max concurrent connections\n");
                     continue;
                }
    
                int new_fd = add_server( arg1, strtol(arg2, NULL, 10)); //connect to the server
                if(write(new_fd, name, strlen(name)) == -1){ //write name to the server
                    close(new_fd); //if server is closed, close the fd
                    continue;
                }
                FD_SET(new_fd, &all_fds);
                if(new_fd > max_fd){ // update max_fd
                    max_fd = new_fd;
                }
                for (int index = 0; index < MAX_AUCTIONS; index++) {
                    int auction_fd = auctions[index].sock_fd;
                    if(auction_fd == -1){// find an empty spot in the auctions array
                        auctions[index].sock_fd = new_fd; // put the server into the auctions array
                        break;   //do not need to go into the next iteration of the loop
                    }
                }
                
            }
            else if (command == BID){ // bid <item index> <bid value>
                
                
                int item_index = strtol(arg1, NULL, 10);
               
                int bid_value = strtol(arg2, NULL, 10);
              
                if(auctions[item_index].sock_fd == -1){
                    fprintf(stderr,"There is no auction open at %d\n", item_index);
                    continue;
                }
                
                if(write(auctions[item_index].sock_fd, arg2, strlen(arg2)) == -1){
                    close(auctions[item_index].sock_fd);
                    auctions[item_index].sock_fd = -1;
                    auctions[item_index].item[0] = '\0';
                    auctions[item_index].current_bid = -1;
                    FD_CLR(auctions[item_index].sock_fd, &all_fds);
                }


            }else{ // command == -1
                print_menu();
            }
        }
        for (int index = 0; index < MAX_AUCTIONS; index++) { 
            int auction_fd = auctions[index].sock_fd;
            if(auction_fd != -1 && FD_ISSET(auction_fd, &listen_fds)){// if one auction_server is ready
                
                int num_read = read(auction_fd, buf, BUF_SIZE);
                buf[num_read] = '\0';
                
                if(num_read == 0){
                    close(auction_fd);
                    auctions[index].sock_fd = -1;
                    auctions[index].item[0] = '\0';
                    auctions[index].current_bid = -1;
                    FD_CLR(auction_fd, &all_fds);
                }
                char *str = "Auction closed";
                if(strstr( buf, str)!= NULL){
                    printf("%s\n", buf);
                    close(auction_fd);
                    auctions[index].sock_fd = -1;
                    auctions[index].item[0] = '\0';
                    auctions[index].current_bid = -1;
                    FD_CLR(auction_fd, &all_fds);
                }else{
                    update_auction(buf, BUF_SIZE, auctions, index);
                }
                
                
            }
        }
        
    }
    return 0; // Shoud never get here
}
