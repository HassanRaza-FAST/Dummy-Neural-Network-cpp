# Dummy Neural Network Project

## Overview

This project is a dummy neural network implemented in C++ for an Operating Systems course. It demonstrates the use of pipes for inter-process communication and synchronization using semaphores and pthreads.

## Features

- Neural network computation using threads.
- Inter-process communication using named pipes.
- Synchronization using pthreads and semaphores.
- Computation for input, hidden, and output layers.

## Technologies Used

- Programming Language: C++
- Libraries: pthread, semaphore, fstream, iostream, vector, string, unistd, fcntl, sys/stat, sys/types, cstring, sstream, iomanip, sys/wait

## File Structure

- `main.cpp`: Contains the main logic for the neural network computation.
- `hidden_layer.cpp`: Handles computations for the hidden layer.
- `output_layer.cpp`: Handles computations for the output layer.
- `configuration.txt`: Configuration file containing network parameters and input data.
