# Wireless-Network-Communication-Simulation-using-CNET

Description:
This project involves the development of a TCP-like protocol simulation using the CNET network simulator. The primary goal is to facilitate efficient and reliable message transmission in a simulated wireless network environment. This environment consists of mobile nodes and access points (anchors), where communication is based on transmitting, forwarding, and acknowledging messages.

Technical Highlights:
1. **Randomized Message Transmission**: The simulation randomly selects a mobile node as the message recipient, ensuring dynamic communication patterns.
2. **Handling Duplicate Messages**: A mechanism is implemented to avoid processing duplicate messages. This is crucial for reducing network congestion and improving overall efficiency.
3. **Acknowledgment Handling**: The simulation includes the acknowledgment of received messages. This feature is vital for confirming successful message delivery and aligns with TCP's reliability principle.
4. **Beacon Message Broadcasting**: Access points broadcast beacon messages, a feature that emulates a real-world scenario where base stations periodically send signals to indicate their presence.
5. **Request and Relay Mechanism**: Mobile nodes can request data from access points and relay messages to other nodes, simulating a fundamental aspect of network routing and data distribution.
6. **Efficient Data Storage and Retrieval**: The implementation of frame tables (for both mobile nodes and anchors) to store and manage messages effectively demonstrates an understanding of data structures in network programming.
7. **Mobility Handling**: The code accounts for node mobility, a critical aspect of wireless networking, affecting signal strength and network topology.

Challenges and Solutions:
- **Efficient Data Management**: Designing and managing frame tables to store, retrieve, and forward messages efficiently was a significant challenge. This was addressed by implementing robust checking mechanisms to avoid duplicate entries and effectively manage memory.
- **Dynamic Network Conditions**: Adapting to changing network conditions, such as node mobility and variable transmission ranges, required a flexible design. This was achieved through randomized message transmission and adaptive response to beacon messages.
- **Reliability in Message Delivery**: Ensuring message delivery in a dynamic network environment was a challenge. The solution involved implementing acknowledgment mechanisms and message retransmission in case of failures.

This project showcases skills in network programming, specifically in designing protocols that mimic TCP behavior in a wireless setting. It highlights an understanding of the complexities involved in reliable data transmission and efficient handling of network dynamics.
