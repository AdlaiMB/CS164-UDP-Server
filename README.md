# CS164-UDP-Server
A class project implementing a UDP server that mimics TCP-like behavior, including connection management, reliability, and ordered data delivery.

## Table of Contents
- Compiling and Running Table Of Contents
   - Compile
   - Run
- UDP Server Behavior
- Connection Initialization: 3-Way Handshake
   - Step1: Receiving SYN and Sending SYN-ACK
   - Step2: Receiving ACK to Complete Handshake
- Transmission Setup Phase
   - Receiving Window Size and Byte Request
- Data Transmission
   - Updating Transmit Window and Sequence Pointer
   - Starting Timer and Waiting for First ACK
- Connection Termination


## Compilingand Running

### Compile

```g++ ./server.c -o server ```


```g++ ./reference_client.c -o client```

### Run

The port number for the server and client are the same. The number is in the range of 1–11.

**Step 1: Run the Server**

```
./server {port_number}
```

**Step 2: Run the Client**

```
./client {port_number} test_{number}.txt
```

---

## UDP Server Behavior

![server behavior outline](/Images/Program%20Behavior.png)

## Connection Initialization: 3-Way Handshake

### Step 1: Receiving SYN and Sending SYN-ACK

![code snapshot](/Images/Figure%201.0.png)

*Figure 1.0: Receiving SYN packet*


![code snapshot](/Images/Figure%201.1.png)

*Figure 1.1: Send SYN-ACK packet*


We wait for a SYN packet from a client to begin the handshake process. After the SYN packet is received (Figure 1.0), we move towards setting up a SYN-ACK packet for the SYN packet received. We then send the SYN-ACK packet to the client (Figure 1.1).

### Step 2: Receiving ACK to Complete Handshake


![code snapshot](/Images/Figure%201.2.png)

*Figure 1.2: Receive ACK packet*

We then wait for an ACK packet from the client after our last SYN-ACK packet was sent to them (Figure 1.2).

---

## Transmission Setup Phase

### Receiving Window Size and Byte Request


![code snapshot](/Images/Figure%202.0.png)

*Figure 2.0: Receive Window and Byte Request*

After the 3-Way Handshake, we move to wait to receive the window size and the byte request.  
The window size is the amount of unacked packets we can have in flight during transmission.  
The byte request is the amount of bytes (data) the client is requesting from the server.

---

## Data Transmission

### Updating Transmit Window and Sequence Pointer

![code snapshot](/Images/Figure%203.0.png)

*Figure 3.0: Begin Transmission*

After receiving the window size and bytes request, the server begins transmitting the data.  
We have a while loop that continues (`conn_alive` in Figure 3.0) transmitting data while there is still data to transmit. Inside the loop we define our `window` variable, which will hold the first sequence number outside the window based on the `base` variable which holds the last unacked packet.

After that is decided, we loop through all the sequence numbers less than the last sequence number held by the `window` variable. After transmitting all the sequence numbers inside the window, we set the `next_seq_num` variable (which holds the next sequence number to be transmitted) to the `window` variable (which holds the first sequence number outside our window).

### Starting Timer and Waiting for First ACK


![code snapshot](/Images/Figure%203.1.png)

*Figure 3.1: Begin timer*

After transmitting all the packets with sequence numbers inside the window, we begin the timer and start waiting for an ACK for the oldest unacked packet.  
We also have a loop (waiting in Figure 3.1) for when an incorrect ACK packet is received, so we continue waiting for a timeout to occur (with the remaining time left on the timer if there is any) or a correct ACK packet is received.

---

**Correct Packet Received: Restart Timer**

![code snapshot](/Images/Figure%203.2.png)

*Figure 3.2: Receive correct packet and exit loop*

We then exit the select function after we receive a packet from the client.  
We then check the ACK field inside the packet to see if it matches with the oldest unacked sequence number (or if it is greater than the oldest unacked sequence number for a cumulative ACK).  
We then move the `base` variable to be one greater than the received packet ACK field and restart the timer and increment the number of successful sent packets.  

The new base will then progress the window as it is derived partly by the `base` variable.  
We also then set the `waiting` variable to `0` to exit the loop and begin transmitting the new data inside the new window. The `successful_sent` variable will allow us to determine whether or not we “grow” the window back to the original size (if two successful windows were sent to “grow” the window).

---

**Incorrect Packet Received: Continue Waiting**

![code snapshot](/Images/Figure%203.3.png)

*Figure 3.3: Receive incorrect packet and stay in loop*

When a packet is received with the incorrect ACK, we don’t progress the `base` variable or change the `waiting` variable to keep us inside the loop and wait for the remaining time on the timer if there is any.

---

**Handling Timer Timeout**

![code snapshot](/Images/Figure%203.4.png)

*Figure 3.4: Timer timeout*

In the case of the timeout, we reset the timer, successful sends, and reduce the window size by half.  
We also set the `next_seq_num` to the `base` to retransmit from the base up until the old window (since base was never updated).

---

## Connection Termination

![code snapshot](/Images/Figure%204.0.png)

*Figure 4.0: Window outside Bytes Request and exit Transmission loop*

![code snapshot](/Images/Figure%204.1.png)

*Figure 4.1: Send RST packet and close connection*

If the `base` variable is beyond the `BYTES_REQUEST` (Figure 4.0) variable, we then set the `conn_alive` to `0` to exit the transmitting loop.  
We then move to the closing process.  
We set the `sendpacket` to have the flag field to be `RST` to send to the client to notify the client we are closing the connection.  
The packet is then sent and we close the socket (Figure 4.1).
