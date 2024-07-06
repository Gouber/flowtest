# Overview
This Trading Server application handles trading orders by setting up a server that manages client connections, processes various types of orders, and updates the trading positions accordingly. It ensures that all operations comply with predefined thresholds for buy and sell actions.

## Functionality
### Server Initialization
1. Socket Setup
     
  Creates and configures a master socket.
  Binds the socket to the specified port (6969).
  Starts listening for incoming client connections.

2. Connection Handling:

Accepts new client connections and adds the new sockets to the list of active sockets.
Manages multiple client connections simultaneously.

### Order Processing
1. New Orders:
   
  Validates new buy and sell orders against predefined thresholds (buyThreshold and sellThreshold).
  Updates internal records for instruments, orders, and traders.
  Sends a response back to the client indicating whether the order was accepted or rejected.

3. Delete Orders:

  Processes requests to delete existing orders.
  Updates the internal records to reflect the deletion.

3. Modify Orders:
   
  Placeholder for handling modifications to existing orders (functionality not fully implemented).
  
4. Trade Updates:

Receives trade information and updates the net position for instruments accordingly.
### Data Management
Instrument Positions: Keeps track of the net position, buy/sell quantities, and hypothetical worst-case positions for various instruments.
Trader Orders: Maps traders to their respective orders.
Order Details: Maintains detailed records of all orders, including quantity, instrument ID, side (buy/sell), and trader ID.
### Cleanup and Error Handling
Properly removes disconnected sockets and cleans up resources.
Handles errors gracefully by printing appropriate error messages and ensuring continued operation.
### Socket Communication
Uses select to monitor multiple sockets for activity.
Reads data from active sockets and processes it based on the message type (new order, delete order, trade update, etc.).
Sends responses to clients to confirm the status of their requests.

### Self critique 

1. Code Modularity: The code could benefit from better modularization. For example, encapsulating functionalities related to order processing, socket handling, and data management into separate classes or modules would enhance readability and maintainability.
2. Documentation: Inline comments are present, but more detailed documentation on the logic and flow of the program would help future developers understand and maintain the code more easily.
3. No build tooling
