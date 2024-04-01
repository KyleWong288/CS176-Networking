Program explanations:

UDP Client:
1. Configure the client socket. Use SOCK_DGRAM for UDP.
2. Configure the server address with the desired IP address and port number.
3. Read in a string from user input.
4. Load the buffer with the user input and send to the server. 
5. Receive the server response, ensure null termination, and print the response.
6. Repeat step 5 until the integer from the server response is a single digit.
7. Close the client socket.

UDP Server:
1. Configure the server socket. Use SOCK_DGRAM for UDP.
2. Configure the server address with INADDR_ANY and the desired port number.
3. Bind the server socket to the server address.
4. Receive a string from client-side user input.
5. Validate if the string is all digits.
6. Compute the digit sum, load the buffer with the result, and send to the client.
7. Repeat step 6 until the digit sum is a single digit

TCP Client:
1. Configure the client socket. Use SOCK_STREAM for TCP.
2. Configure the server address with the desired IP address and port number.
3. Read in a string from user input.
4. Initiate the handshake and connect with the server.
5. Load the buffer with the user input and send to the server. 
6. Receive the server response, ensure null termination, and print the response.
7. Repeat step 6 until the integer from the server response is a single digit.
8. Close the client socket.

TCP Server:
1. Configure the server socket. Use SOCK_STREAM for TCP.
2. Configure the server address with INADDR_ANY and the desired port number.
3. Bind the server socket to the server address.
4. Await and receive the handshake and connect with the client.
5. Receive a string from client-side user input.
6. Validate if the string is all digits.
7. Compute the digit sum, load the buffer with the result, and send to the client.
8. Repeat step 7 until the digit sum is a single digit.
9. Close the connection with the client.


Utility Functions explanations:

server_finish():
The client should stop looking for responses when it receives "From server: Sorry, cannot compute!" or "From server: %d", where d is a single digit.
This function does a naive check for either of these cases.

validate_input():
Validates if the user input string is all digits and is non-empty.

digit_sum_str():
Gets the digit sum of a string type.
We need a version that takes a string since the user input can be 128 bytes long, which would overflow an integer based datatype.

digit_sum_int():
Gets the digit sum of an integer type.