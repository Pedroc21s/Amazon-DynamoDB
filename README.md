# Amazon-DynamoDB

A secure and reliable key-value storage service inspired by Amazon DynamoDB, developed in C. This project was created as part of the Distributed Systems course and focuses on high availability, fault tolerance, and efficient concurrency management.

## 🚀 Features
- **Custom Storage System:** Designed a custom hashmap structure for efficient key-value storage.
- **Concurrent Architecture:** Utilizes multithreading and synchronization with semaphores for handling multiple client requests.
- **Fault Tolerance & Scalability:** Implements server state replication and the Chain Replication model to enhance reliability.
- **Efficient Communication:** Uses Google Protocol Buffers for data serialization between threads.
- **Coordination with Apache ZooKeeper:** Ensures data consistency and system coordination.
- **Durability & High Availability:** Prevents data loss and ensures no downtime, making it a dependable solution for critical applications.

## 📌 Contributors
- **Pedro Correia**
- **Sara Ribeiro**
- **Miguel Mendes**

## 🛠️ Compilation & Execution

### 🔹 Compilation
```sh
make clean
make
```

### 🔹 Running the Client
```sh
./client_hashtable <server>:<port>
```
Example:
```sh
./client_hashtable localhost:2181
```

### 🔹 Running the Server
```sh
./server_hashtable <zookeeper_ip:port> <port> <n_lists>
```
Example:
```sh
./server_hashtable localhost:2181 12345 3
```

## 🔧 Supported Operations
```sh
put <key> <value>  # Store a key-value pair
get <key>          # Retrieve the value of a key
del <key>          # Delete a key-value pair
size               # Get the total number of stored keys
getkeys            # Retrieve all stored keys
gettable           # Get the entire key-value table
quit               # Exit the client
```

## 📜 License
This project was developed for academic purposes at the University of Lisbon as part of the Distributed Systems course.

