## How it works:
  The exchange operates by receiving orders through named pipes from traders who are acting as child processes of the exchange. These orders are first read, validated, put into data memory, and then go through opperation to be matched with each other, using a price time priority mechanism for order matching, ensuring that the most economically suitable orders are paired together. For instance, a buy order will be matched with the lowest-priced sell order, and in cases where multiple valid sell orders exist at the same price level, the oldest order will be given priority. To facilitate these operations, the exchange maintains an orderbook that contains all the current orders, allowing for efficient tracking and management. The traders and the exchange communicate through signals. The pipes are read when a signal isreceived by the exchnage and vice-versa. 

## Fault Tolerant Design Decisions.
  The algorithm incorporates several design decisions to enhance fault tolerance. It uses signal handling through the sigaction function, enabling asynchronous message processing and prompt response to incoming messages. Named pipes, or FIFOs, facilitate reliable inter-process communication, ensuring data integrity during process failures. The auto_order function processes messages from the exchange, generating corresponding orders to be executed. Error handling mechanisms are in place to handle exceptional scenarios, such as order size exceeding limits or errors during communication.
