/**
 * COMP2017 - ASSIGNMENT 3
 * Eduardo Ferronato Breunig
 * efer7583
 */

#include "pe_trader.h"

int trader_id, order_id;
int exchange_fd, trader_fd;
char fifo_trader[MAX_LENGTH], fifo_exchange[MAX_LENGTH], msg_from_exchange[MAX_LENGTH];;
char *msg_to_exchange;

void auto_order(char *order) {

    char *first_word = strtok(order, " ");
    
    if (strcmp(first_word, "MARKET") == 0) {
        char *request = strtok(NULL, " ");

        
        if (strcmp(request, "SELL") == 0){

            char *product = strtok(NULL, " ");
            char *token = strtok(NULL, " ");
            int order_size = atoi(token);

            if (order_size > MAX_QUANTITY) {
                //printf("ORDER SIZE IS TOO LARGE!");
                exit(0);
            }

            int price = atoi(strtok(NULL, " "));
            char auto_order[MAX_LENGTH];
            sprintf(auto_order, "BUY %d %s %d %d;", order_id++, product, order_size, price);
            int len = strlen(auto_order);
            if (write(trader_fd, auto_order, len) != len) {
                perror("MESSAGE TO EXCHANGE ERROR");
                exit(0);
                }
            else{
                kill(getppid(), SIGUSR1);
            }
            
        }
        }
}

void handler(int sig) {
    if (sig == SIGUSR1) {
        if (read(exchange_fd, msg_from_exchange, MAX_LENGTH) < 0) {
        perror("MESSAGE FROM EXCHNAGE ERROR");
        exit(0);
        }   
        auto_order(msg_from_exchange);
    }
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    trader_id = atoi(argv[1]);

    // connect to named pipes
    sprintf(fifo_exchange, FIFO_EXCHANGE, trader_id);
    sprintf(fifo_trader, FIFO_TRADER, trader_id);
    exchange_fd = open(fifo_exchange, O_RDONLY);
    trader_fd = open(fifo_trader, O_WRONLY);
    
    // register signal handler
    struct sigaction sig;
    sig.sa_handler = handler;

    if (sigaction(SIGUSR1, &sig, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sleep(1);
    }

    close(exchange_fd);
    close(trader_fd);
    unlink(fifo_exchange);
    unlink(fifo_trader);
}