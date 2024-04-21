#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"

#define LOG_PREFIX "[PEX]"

#define MAX_PRODUCT 17
#define MAX_COMMAND 60
#define MAX_FIFO_PATH 32
#define FEE_OPERATION 100

typedef struct trader {
  int trader_id;
  int exchange_fd;
  int trader_fd;
  pid_t pid;
  int connected;
  int order_id;
  long* rank_size;
  long* rank_profit;
} Trader;

typedef struct order{
  struct trader* trader;
  int order_id;
  char product[MAX_PRODUCT];
  char command[MAX_FIRST_WORD];
  int size;
  int price;
  int time;
  int completed;
  int end;
}Order;


typedef struct orderbook {
    int price;   
    int size_level;       
    int size_order;       
    char command[MAX_FIRST_WORD];    
}Orderbook;

#endif

