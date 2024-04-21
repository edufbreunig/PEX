/**
 * COMP2017 - ASSIGNMENT 3
 * Eduardo Ferronato Breunig
 * efer7583
 */

#include "pe_exchange.h"

//==================CHANGE SIZE ================//
char exchange_pipe[MAX_COMMAND];
char trader_pipe[MAX_COMMAND];
int num_products;
int num_traders;
int num_orders;
int fees_collected;
int time = 0;
Order* curr_order;


//flags
int TRADER_MESSAGE = -1;
int TRADER_DISCONNECTED = -1;

void sig_handler(int sig, siginfo_t *info, void *con){
	if (sig == SIGUSR1){
		TRADER_MESSAGE = info -> si_pid;
	}
	if (sig == SIGCHLD){
		TRADER_DISCONNECTED = info -> si_pid;
	}
}

char **get_products(char *path) {
    FILE *file;
    char **products;
    char buffer[MAX_PRODUCT]; 

    file = fopen(path, "r");
    if (file == NULL) {
        exit(1);
    }

    fgets(buffer, MAX_PRODUCT, file);
	num_products = atoi(buffer);

    products = (char **) malloc((num_products + 1) * sizeof(char *));
    if (products == NULL) {
        exit(1);
    }

    for (int i = 0; i < num_products; i++) {
        fgets(buffer, MAX_PRODUCT, file);
		buffer[strlen(buffer) - 1] = '\0';
        products[i] = (char *) malloc(sizeof(char) * MAX_PRODUCT);
        strcpy(products[i], buffer);
    }

    fclose(file);
	products[num_products] = NULL;

    return products;
}
void product_message(char** products){
	printf("%s Trading %d products:", LOG_PREFIX, num_products);
	for (int i = 0; i < num_products; i++) {
        printf(" %s", products[i]);
    }
    printf("\n");

}
void free_traders(int num_traders, Trader** all_traders){
	
	if(all_traders != NULL){
		for (int i = 0; i < num_traders; i++) {
			free(all_traders[i] -> rank_size);
			free(all_traders[i] -> rank_profit);
			free(all_traders[i]);
		}
		free(all_traders);
	}
	
}
void free_products(char** products){
	
	if(products != NULL){
		for (int i = 0; i <= num_products; i++) {
    		free(products[i]);
		}
		free(products);
	}

}
void free_messages(char** messages){
	
	if(messages != NULL){
		for (int i = 0; messages[i]!= NULL; i++) {
    		free(messages[i]);
		}
		free(messages);
	}

}
void free_orders(Order** all_orders){
    printf("inside orders\n");
	if(all_orders != NULL){  
		for (int i = 0; all_orders[i] != NULL; i++){
			printf("inside for orders\n");
			if(all_orders[i] != curr_order){
				printf("inside if orders\n");
				free(all_orders[i]);
			}
			
		}
		printf("before free orders\n");
		free(all_orders);
	}
}
void write_fifo(char* str, int fd){
	//printf("Inside pipe write\n");
    size_t str_length = strlen(str);
    if (str_length <= MAX_COMMAND) {
        if (fd == -1) {
			printf(" before exit 1 \n");
            exit(1);
        }
       // printf("before writing\n");
        ssize_t bytes_written = write(fd, str, str_length);
        if (bytes_written == -1) {
			printf("exit 2\n");
            exit(1);
        }
    } else {
		//printf(" exit \n");
        exit(1);
    }

	//printf("after pipe write\n");
}
char **read_fifo(int fd) {
  char** messages = malloc(sizeof(char**));
   messages[0] = malloc(MAX_PRODUCT);
   int commands= 0;


	for (int counter = 0, chars = 0; counter < MAX_COMMAND && chars < MAX_PRODUCT - 1; counter++) {
    	int num_read = read(fd, &messages[commands][chars], 1);
    	if (num_read < 0){
        	return NULL;
    	}

   		if (messages[commands][chars] == ' ' || messages[commands][chars] == ';') {
        	messages = realloc(messages, sizeof(char**) * (commands + 2));

        if (messages[commands][chars] == ';') {
            messages[commands][chars] = '\0';
            messages[commands + 1] = NULL;
            return messages;
        }

        messages[commands][chars] = '\0';
        commands++;

        messages[commands] = malloc(MAX_PRODUCT);
        chars = 0;
        continue;
    	}

    chars++;
	}
	return NULL;
}
int get_command(char** messages){
	int num_commands = 0;

	if (strcmp(messages[0], "CANCEL") == 0) {
		num_commands = 2;
	}
	if (strcmp(messages[0], "AMEND") == 0) {
		num_commands = 4;
	}
	if (strcmp(messages[0], "SELL") == 0 || strcmp(messages[0], "BUY") == 0) {
		num_commands = 5;
	}
	return num_commands;
}
int check_message_traders(int num_commands, char** messages, Trader** all_traders, Order *all_orders, int found_trader, char** products, int num_parse){
	//printf("Inside checked message\n");
	if(num_parse != num_commands){
		return -1;
	}

	//check_id()

	//check_product()

	char* msg_to_trader = malloc(MAX_COMMAND * sizeof(char));

	if(num_commands == 5){
		int size = strtol(messages[3], NULL, 10);
    	int price = strtol(messages[4], NULL, 10);

		if(size <= 0 || size > MAX_QUANTITY){
			free(msg_to_trader);
			return -1;
		}
		if(price <= 0 || price > MAX_QUANTITY){
			free(msg_to_trader);
			return -1;
		}

        sprintf(msg_to_trader, "ACCEPTED %s;", messages[1]);
    }

    if(num_commands == 4 || num_commands == 2){

        if(num_commands == 4){
            int size = strtol(messages[2], NULL, 10);
            int price = strtol(messages[3], NULL, 10);

			if(size <= 0 || size > MAX_QUANTITY){
				free(msg_to_trader);
				return -1;
			}

			if(price <= 0 || price > MAX_QUANTITY){
				free(msg_to_trader);
				return -1;
			}
            sprintf(msg_to_trader, "AMENDED %s;", messages[1]);
        }

        if (num_commands == 2){
            sprintf(msg_to_trader, "CANCELLED %s;", messages[1]);
        }
	}

	write_fifo(msg_to_trader, all_traders[found_trader] -> exchange_fd);
	kill(all_traders[found_trader]->pid, SIGUSR1);
	free(msg_to_trader);
	return 1;

}
void market_message(Order curr_order, Trader** all_traders, char* msg_to_trader){
	
	
	//printf(" market message func \n");
	for (int i= 0; i < num_traders; i++) {
			//printf(" market message for \n");
		if (all_traders[i] != curr_order.trader && all_traders[i] -> connected == 1){
			//printf(" market message for  2 \n");
			write_fifo(msg_to_trader, all_traders[i] -> exchange_fd);
			//printf(" market message fifo \n");
			kill(all_traders[i] -> pid, SIGUSR1);
			//printf(" market message kill \n");
		}
	}
		//printf(" market message after \n");
}
Orderbook* make_levels(int* command_level, Orderbook* levels, Order order, int* total_levels) {
    
    Orderbook* new_levels = NULL;

    for (int i = 0; i < *total_levels; i++) {
        int match = (order.price == levels[i].price && strcmp(order.command, levels[i].command) == 0); // match between order prices and commands
        if (match){
			levels[i].size_order += order.size; //update the total size of the order
            levels[i].size_level++; // increment the number of orders 
            return levels;
        }
    }

    //if no match is found, create a new level
    *total_levels += 1;
    *command_level += 1;

    //reallocates memory for the new order
    new_levels = realloc(levels, sizeof(Orderbook) * (*total_levels));
	if (new_levels == NULL){//error handling
        free(levels);
        return NULL;
    }

	levels = new_levels;

    //create a new level with for the new order
    Orderbook curr_level;
    curr_level.price = order.price;
    curr_level.size_level = 1;
    curr_level.size_order = order.size;

	strcpy(curr_level.command, order.command);
	
    	
    // add the new level to the end of the levels array
    levels[*total_levels - 1] = curr_level;

    return levels;
}
Orderbook*  print_levels(Orderbook* levels, int total_levels) {

    for (int index = total_levels; index > 0; index--) {
        int found_level;
        int highest_price = 0;

        for (int j = 0; j < index; j++) {
            if (levels[j].price > highest_price) {
                highest_price = levels[j].price;
                found_level = j;
            }
        }

        printf("%s\t\t%s %d @ $%d (%d %s)\n", LOG_PREFIX, levels[found_level].command, levels[found_level].size_order, levels[found_level].price, levels[found_level].size_level, (levels[found_level].size_level > 1) ? "orders" : "order");

        for (int i = found_level; i < index - 1; i++) {
            levels[i] = levels[i + 1];
        }

        levels = realloc(levels, sizeof(Orderbook) * (index - 1));
    }

    return levels;
}

void print_positions(Trader** all_traders, char** products) {
    printf("%s\t--POSITIONS--\n", LOG_PREFIX);

	int i = 0;
	while(all_traders[i] != NULL){
    	printf("%s\tTrader %d: ", LOG_PREFIX, all_traders[i] -> trader_id);
    	for (int j = 0; j < num_products; j++){
			printf("%s %ld ($%ld)", products[j], all_traders[i]->rank_size[j], all_traders[i] -> rank_profit[j]);
			if (j != num_products - 1) {
				printf(", ");
			} else {
				printf("\n");
			}
		}
		i++;
	}
}
void orderbook(struct trader **all_traders, struct order *all_orders, char** products){

	printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);

	for (int i = 0; i < num_products; i++){

	int total_levels = 0;
	Orderbook* levels = malloc(0);
    int sell_levels = 0;
    int buy_levels = 0;
	
    

    for (int order = 0; order < num_orders; order++) {
        if (strcmp(all_orders[order].product, products[i]) == 0 && all_orders[order].completed != 1) {
            if (strcmp(all_orders[order].command, "BUY") == 0) {
                levels = make_levels(&buy_levels, levels, all_orders[order], &total_levels);
            }
            if (strcmp(all_orders[order].command, "SELL") == 0 && all_orders[order].completed != 1) {
                levels = make_levels(&sell_levels, levels, all_orders[order], &total_levels);
            }
        }
    }

	printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, products[i], buy_levels, sell_levels);
	levels = print_levels(levels, total_levels);
	free(levels);
	}
	print_positions(all_traders, products);
}
int buy(Trader** all_traders, Order* all_orders, int curr_order_index, char** products, int found_position){
	char msg_to_trader[MAX_COMMAND];

	while(1){//while matching sell is still found

		int sell_index = -1; //index of the order with lowest sell
		int sold = 0; // Amount sold in the transaction

		for (int order = 0; order < num_orders; order++){
    		
    		if (strcmp(all_orders[order].command, "SELL") == 0 && strcmp(all_orders[order].product, all_orders[curr_order_index].product) == 0 && all_orders[order].completed != 1 && all_orders[curr_order_index].completed != 1){//check if the order is a sell order, matches the product of the current order and is not completed
       			if (all_orders[order].price <= all_orders[curr_order_index].price){
            		if (sell_index == -1){
                	sell_index = order; //set the sell_index to the first matching order in first iteration
           			}
				    //check if the price of the current order is lower than the price of the order at sell_index or if the prices are equal but the time of the current order is earlier than the time of the order at sell_index
					else if (all_orders[order].price < all_orders[sell_index].price || (all_orders[order].price == all_orders[sell_index].price && all_orders[order].time < all_orders[sell_index].time)){
    				sell_index = order;
					}
            	}
       		}	
    	}


		if(sell_index == -1){//break from the loop and return -1 if no order matched
			break;
		}



		if (all_orders[sell_index].size > all_orders[curr_order_index].size){
		all_orders[sell_index].size -= all_orders[curr_order_index].size; 
		sold = all_orders[curr_order_index].size; //set the sold amount to the size of the current order
		all_orders[curr_order_index].size = 0; //set the size of the current order to 0
		all_orders[curr_order_index].completed = 1;
		}

		if (all_orders[sell_index].size <= all_orders[curr_order_index].size){
			sold = all_orders[sell_index].size; 
			all_orders[curr_order_index].size -= all_orders[sell_index].size; 
			all_orders[sell_index].size = 0; 
			all_orders[sell_index].completed = 1; 
		}

		if(all_orders[sell_index].size == all_orders[curr_order_index].size){
			all_orders[sell_index].completed = 1;
			all_orders[curr_order_index].completed = 1;

		}


		float value = (float)sold * all_orders[sell_index].price;
		int order_fee = round(value * 0.01);
		

		//for the trader that sold, calculates the profit in the transaction and size to the ranking
		all_orders[curr_order_index].trader->rank_profit[found_position] -= value + order_fee;

		// printf("SOLD Value: %f, SOLD Order Fee: %d\n", value, order_fee);
		all_orders[curr_order_index].trader->rank_size[found_position] += sold;

		// printf("SOLD SOLD: %d\n",sold);

		//for the trader that bought,  calculates the profit in the transaction and size to the ranking
		all_orders[sell_index].trader->rank_size[found_position] -= sold;
		all_orders[sell_index].trader->rank_profit[found_position] += value;

		if(all_orders[sell_index].trader -> connected){
			snprintf(msg_to_trader, MAX_COMMAND, "FILL %d %d;", all_orders[sell_index].order_id, sold);
			write_fifo(msg_to_trader, all_orders[sell_index].trader->exchange_fd);
			kill(all_orders[sell_index].trader -> pid, SIGUSR1);
		}


		if(all_orders[curr_order_index].trader -> connected){
			snprintf(msg_to_trader, MAX_COMMAND, "FILL %d %d;", all_orders[curr_order_index].order_id, sold);
			write_fifo(msg_to_trader, all_orders[curr_order_index].trader->exchange_fd);
			kill(all_orders[curr_order_index].trader->pid, SIGUSR1);
		}

		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n", LOG_PREFIX, all_orders[sell_index].order_id, all_orders[sell_index].trader -> trader_id , all_orders[curr_order_index].order_id, all_orders[curr_order_index].trader -> trader_id, (int)value, order_fee);
		
		fees_collected += order_fee;

	}
	return -1;

}		
int sell(Trader** all_traders, Order* all_orders, int curr_order_index, char** products, int found_position){

	char msg_to_trader[MAX_COMMAND];

	while(1){

		int buy_index = -1;//index of the order with the highest buy price with matching product
		int bought = 0;
		

		for (int order = 0; order < num_orders; order++){
			if(strcmp(all_orders[order].command, "BUY") == 0 && strcmp(all_orders[order].product, all_orders[curr_order_index].product) == 0 && all_orders[order].completed != 1 && all_orders[curr_order_index].completed != 1){//checks if order is an order that hs not been completed, selling a matchign product
					if(all_orders[order].price >= all_orders[curr_order_index].price){ //
						if(buy_index == -1){
							buy_index = order;//sets the index in the first iteration so its compares every order in the array
						}
						else if(all_orders[order].price > all_orders[buy_index].price || (all_orders[order].price == all_orders[buy_index].price && all_orders[order].time < all_orders[buy_index].time)){
							buy_index = order;
						}
					}
					
				}
		}

		if(buy_index == -1){
			return -1;//breaks the loop and returns if no match is found
		}
		
		if (all_orders[buy_index].size > all_orders[curr_order_index].size){
			all_orders[buy_index].size -=  all_orders[curr_order_index].size;
			bought = all_orders[curr_order_index].size;
			all_orders[curr_order_index].size = 0;
			all_orders[curr_order_index].completed = 1;
		} 
		if (all_orders[buy_index].size <= all_orders[curr_order_index].size){
			bought = all_orders[buy_index].size;
			all_orders[curr_order_index].size -= all_orders[buy_index].size;
			all_orders[buy_index].size = 0;
			all_orders[buy_index].completed = 1;
		}


		if(all_orders[buy_index].size == all_orders[curr_order_index].size){
			all_orders[buy_index].completed = 1;
			all_orders[curr_order_index].completed = 1;

		}

		float value = (float)bought * all_orders[buy_index].price;
		int order_fee = round(value/FEE_OPERATION);
		

	//for the trader that bought,  calculates the profit in the transaction and size to the ranking
		all_orders[buy_index].trader -> rank_size[found_position] += bought;
		all_orders[buy_index].trader -> rank_profit[found_position] -= value;

		//for the trader that sold, calculates the profit in the transaction and size to the ranking
		all_orders[curr_order_index].trader -> rank_size[found_position] -= bought;
		all_orders[curr_order_index].trader -> rank_profit[found_position] += value - order_fee;


		//inform the trader that sold
		if(all_orders[curr_order_index].trader -> connected){
			snprintf(msg_to_trader, MAX_COMMAND, "FILL %d %d;", all_orders[curr_order_index].order_id, bought);
			write_fifo(msg_to_trader, all_orders[curr_order_index].trader->exchange_fd);
			kill(all_orders[curr_order_index].trader->pid, SIGUSR1);
		}

		//inform the trader that bought
		if(all_orders[buy_index].trader -> connected){
			snprintf(msg_to_trader, MAX_COMMAND, "FILL %d %d;", all_orders[buy_index].order_id, bought);
			write_fifo(msg_to_trader, all_orders[buy_index].trader->exchange_fd);
			kill(all_orders[buy_index].trader -> pid, SIGUSR1);
		}
		
		//print the match message
		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n", LOG_PREFIX, all_orders[buy_index].order_id, all_orders[buy_index].trader -> trader_id , all_orders[curr_order_index].order_id, all_orders[curr_order_index].trader -> trader_id, (int)value, order_fee);

		fees_collected += order_fee;//add transaction fee to the exchnage profit

	}
	return -1;

}
Order *make_order(Trader** all_traders, Order *all_orders, Trader* found_trader, int time, char* str_command, int price, int size, char* product, int order_id, char** products){  
	char msg_to_trader[MAX_COMMAND];


	Order curr_order;
	curr_order.order_id = order_id;
	curr_order.size = size;
	curr_order.price = price;
	curr_order.trader = found_trader;
	curr_order.time = time;


    strncpy(curr_order.product, product, MAX_PRODUCT);

    strncpy(curr_order.command, str_command, MAX_FIRST_WORD - 1);


	sprintf(msg_to_trader, "MARKET %s %s %d %d;", str_command, product, size, price);
	market_message(curr_order, all_traders, msg_to_trader);

	int curr_order_index = num_orders - 1;
	all_orders = realloc(all_orders, sizeof(Order) * num_orders);
	all_orders[curr_order_index] = curr_order;
	
	int found_position = 0;
	for (int i = 0; i < num_products; i++) {
		if (strcmp(products[i], curr_order.product) == 0) {
			found_position = i;
			break;
		}
	}

	if (strcmp(str_command, "BUY") == 0){
		buy(all_traders, all_orders, curr_order_index, products, found_position);
		orderbook(all_traders, all_orders, products);
		return all_orders;
	} 
	if (strcmp(str_command, "SELL") == 0){
		sell(all_traders, all_orders, curr_order_index, products, found_position);
		orderbook(all_traders, all_orders, products);
		return all_orders;
	}

	return all_orders;
}
Trader* make_trader(char* path, int trader_id){
	Trader* curr_trader = malloc(sizeof(Trader));
	curr_trader -> trader_id = trader_id;
	curr_trader -> connected = 1;
	curr_trader -> order_id = 0;
	curr_trader->rank_size = calloc(sizeof(int), sizeof(int) * num_products);
  	curr_trader->rank_profit = calloc(sizeof(int), sizeof(int) * num_products);
	curr_trader -> pid = fork();

	if (curr_trader -> pid > 0){  
		return curr_trader;
	}

	if (curr_trader -> pid < 0){
		free(curr_trader -> rank_size);
		free(curr_trader -> rank_profit);
		free(curr_trader);
		return NULL;
	}

	if (curr_trader -> pid == 0){
		char str_trader_id[10];
		sprintf(str_trader_id, "%d", trader_id);
		if (execl(path, path, str_trader_id, NULL) < 0){
			printf("%s INVALID TRADER BINARY", LOG_PREFIX);
			free(curr_trader);
			exit(1);
		}
	}
	return NULL;
}





int main (int argc, char **argv) {
	setbuf(stdout, NULL);

	if (argc < 3) {
		printf("%s ERROR: INVALID COMMAND LINE ARGUMENT\n", LOG_PREFIX);
		return 1;
	}

	//setup siginals
	struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &sig_handler;

    sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);

	printf("%s Starting\n", LOG_PREFIX);
	
	//array of products
	char** products = get_products(argv[1]);
	product_message(products);

	num_traders = argc - 2; //number of traders given in the command line
	Trader** all_traders = malloc(sizeof(Trader) * (num_traders + 1)); //array of traders
	all_traders[num_traders] = NULL; // end of the array

	//creates processes and connects to named pipes for all traders
	for(int trader_id = 0; trader_id < num_traders; trader_id++){

		//create fifo for exchange
		sprintf(exchange_pipe, FIFO_EXCHANGE, trader_id);
		if (mkfifo(exchange_pipe, 0777) < 0){ //sets read and write permissions for all users
			printf("%s ERROR: COULD NOT CREARE FIFO\n", LOG_PREFIX);
			return 1;
		}
		printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_pipe);

		//create fifos for traders
		sprintf(trader_pipe, FIFO_TRADER, trader_id);
		if (mkfifo(trader_pipe, 0777) < 0){ //sets read and write permissions for all users
			printf("%s ERROR: COULD NOT CREARE FIFO\n", LOG_PREFIX);
			return 1;
		}
		printf("%s Created FIFO %s\n", LOG_PREFIX, trader_pipe);

		//make trader and processess
		char* path = argv[2];
		printf("%s Starting trader %d (%s)\n", LOG_PREFIX, trader_id, argv[trader_id + 2]);
		all_traders[trader_id] = make_trader(path, trader_id);

		if (all_traders[trader_id] == NULL){
			free_products(products);
			free_traders(num_traders, all_traders);
			return 1;
		}

		//connect to trader pipes
		all_traders[trader_id] -> exchange_fd = open(exchange_pipe, O_WRONLY);
		printf("%s Connected to %s\n", LOG_PREFIX, exchange_pipe);

		all_traders[trader_id] -> trader_fd = open(trader_pipe, O_RDONLY);
		printf("%s Connected to %s\n", LOG_PREFIX, trader_pipe);
	}

	for(int i = 0; all_traders[i] != NULL; i++){
    	write_fifo("MARKET OPEN;", all_traders[i] -> exchange_fd);
	}

	for(int i = 0; all_traders[i] != NULL; i++){
		kill(all_traders[i] -> pid, SIGUSR1);
	}
	
	Order* all_orders = malloc(num_orders * sizeof(Order));

	//main event loop
	while(1){

		char** messages;
		int found_trader = -1;
		//checks if trader sent massage
		if(TRADER_MESSAGE != -1){
			//locates which trader sent the message
			for(int i = 0; all_traders[i] != NULL; i++){
				if (all_traders[i] -> pid == TRADER_MESSAGE){
					messages = read_fifo(all_traders[i] -> trader_fd);
					TRADER_MESSAGE = -1;
					found_trader = i;
					break;
				}
			}

			if(messages == NULL){
				free_messages(messages);
			}
			
			//prints parsing message
			int num_parse = 0;
			if(found_trader >= 0){
				printf("%s [T%d] Parsing command: <", LOG_PREFIX, all_traders[found_trader]-> trader_id);
				for(int i = 0; messages[i] != NULL; i++){
					printf("%s", messages[i]);
					num_parse = i + 1;// number of words parsed
					if (messages[i + 1] != NULL){
						printf(" ");
					}
				}
				printf(">\n");
			}


			int num_commands = get_command(messages);//gets the command based on the first wors in the command
			int checked_message = check_message_traders(num_commands, messages, all_traders, all_orders, found_trader, products, num_parse);//checks validity of command
			
			if (checked_message == -1){
				write_fifo("INVALID;", all_traders[found_trader] -> exchange_fd);
				kill(all_traders[found_trader] -> pid, SIGUSR1);
				
			}
			else{
				num_orders ++;
				char* str_command = messages[0];
				int order_id = strtol(messages[1], NULL, 10);
				if(strcmp(str_command, "SELL") == 0){
					
					all_traders[found_trader] -> order_id++;
					int size = strtol(messages[3], NULL, 10);
					int price = strtol(messages[4], NULL, 10);
					char* product = messages[2];
				    all_orders = make_order(all_traders, all_orders, all_traders[found_trader], time, str_command, price,  size,  product,  order_id, products);
					time++;
				}
				if (strcmp(str_command, "CANCEL") == 0){
					//all_orders = make_order(all_traders, all_orders, all_traders[found_trader], time, str_command, price, size, NULL, order_id);
				}
				if (strcmp(str_command, "AMEND") == 0){
					//all_orders = make_order(all_traders, all_orders, all_traders[found_trader], time, str_command, price, size, NULL, order_id);
				}
				if(strcmp(str_command, "BUY") == 0){
					
					int size = strtol(messages[3], NULL, 10);
					int price = strtol(messages[4], NULL, 10);
					char* product = messages[2];
					all_traders[found_trader] -> order_id++;
					all_orders = make_order(all_traders, all_orders, all_traders[found_trader], time, str_command, price, size, product, order_id, products);
					time++;
					
				}
				
			}

			free_messages(messages);
			
			
		}

		if (TRADER_DISCONNECTED != -1){
			int traders_connected = 0;
			for(int trader = 0; all_traders[trader] != NULL; trader++){
				if(TRADER_DISCONNECTED == all_traders[trader] -> pid) {
					printf("%s Trader %d disconnected\n", LOG_PREFIX, all_traders[trader] -> trader_id);
					all_traders[trader] -> connected = 0;
				}
				if(all_traders[trader] -> connected == 1){
					traders_connected++;
				}

			}
			TRADER_DISCONNECTED = -1;

			if(traders_connected == 0){
				printf("%s Trading completed\n", LOG_PREFIX);
				printf("%s Exchange fees collected: $%d\n", LOG_PREFIX, fees_collected);
				free(all_orders);
				for(int trader_id = 0; all_traders[trader_id] != NULL; trader_id++){
					char path[MAX_FIFO_PATH];
					snprintf(path, MAX_FIFO_PATH, FIFO_TRADER, trader_id);
					unlink(path);
					snprintf(path, MAX_FIFO_PATH, FIFO_EXCHANGE, trader_id);
					unlink(path);
				}
				free_traders(num_traders, all_traders);
				free_products(products);
				return 0;
			}
			
		}
			
			
		}
			
	}