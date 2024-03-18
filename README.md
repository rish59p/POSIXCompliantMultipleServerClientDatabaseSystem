# POSIX Compliant Multiple Server-Client Database System
This repository contains the implementation of a distributed graph database system, developed as part of the Operating Systems (CS F372) course assignment for the semester I of 2023-2024.

## Problem Statement

The goal of this assignment is to simulate an application for a distributed graph database system involving a load balancer process, a primary server process, two secondary server processes (secondary server 1 and secondary server 2), a cleanup process, and several clients.

![alt text](/images/problemstmt.png)


### Overall Workflow

- Clients send requests (read or write) to the load balancer.
- Load balancer forwards the write requests to the primary server.
- Load balancer forwards odd-numbered read requests to secondary server 1 and even-numbered read requests to secondary server 2.
- Each server (primary or secondary) creates a new thread to process a request and sends a message or output back to the client once processing is complete.
- Cleanup process is responsible for terminating the application and performing necessary cleanup activities.

## Implementation Processes

### Client Process (`client.c`)

- Sends requests to the load balancer via a single message queue.
- Displays a menu to the user with options to add a new graph, modify an existing graph, perform DFS, and perform BFS.
- Uses a 3-tuple format for sending requests: `<Sequence_Number Operation_Number Graph_File_Name>`.
- For write operations, writes the number of nodes and the adjacency matrix to a shared memory segment.
- For read operations, specifies the starting vertex for BFS/DFS traversal in a shared memory segment.

### Load Balancer (`load_balancer.c`)

- Receives client requests via the single message queue.
- Forwards write requests to the primary server and read requests to secondary servers based on request numbers.
- Manages the single message queue and deletes it upon termination.

### Primary Server (`primaryserver.c`)

- Receives write requests from the load balancer via the single message queue.
- Creates a new thread to handle each request, reads the graph information from the shared memory segment, and modifies the corresponding graph file.
- Sends a success message back to the client via the single message queue.

### Secondary Server (`secondaryserver.c`)

- Handles read operations.
- Spawns a new thread to handle each client request.
- Implements DFS and BFS using multithreading for acyclic graphs.
- Reads the starting vertex from the shared memory segment and sends the output back to the client via the single message queue.

### Cleanup Process (`cleanup.c`)

- Runs alongside clients, load balancer, and servers.
- Displays a menu to terminate the application.
- Informs the load balancer to terminate, which triggers the termination of all other processes.

## Synchronization Mechanisms

- Use of semaphores: For handling concurrent client requests and ensuring that conflicting operations on the same graph file are performed serially.
- Use of shared memory: For storing graph information and starting vertices for BFS/DFS traversal.
- Use of multithreading: Each server creates a new thread to handle client requests, allowing for concurrent processing.
- Use of mutex: Ensures that conflicting operations on the same graph file are performed serially.

