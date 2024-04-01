Program explanations:

UDP Client:
1. Configure the client socket. Use SOCK_DGRAM for UDP.
2. Configure the server address with the desired hostname, IP address and port number.
3. Load the buffer with the ping message and store the start time.
4. Send the message to the server. 
5. Receive the server response and store the end time.
6. Repeat steps 3-5 10 times, tracking the number of successful pings, min rtt, max rtt, and sum of rtts.
7. Compute and print the summary metrics.