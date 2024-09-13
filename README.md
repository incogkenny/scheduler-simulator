# Scheduler Simulator

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [Installation](#installation)
- [Supported Scheduling Algorithms](#supported-scheduling-algorithms)

## Introduction

Scheduler Simulator is a C-based simulation tool designed to emulate and visualise CPU scheduling algorithms. The project is aimed at helping developers, students, and instructors better understand how various scheduling algorithms work by simulating the execution of processes over time.
The simulators gradually increases in complexity as the number attached to them increases.

CPU scheduling is a key component of operating systems, determining how processes are assigned CPU time to optimise performance. This tool provides an interactive way to experiment with different scheduling methods and observe their outcomes.

## Features
- Simulate various CPU scheduling algorithms.
- Visualise process execution order.
- Customise process parameters like arrival time and burst time.
- Compare different scheduling strategies in real-time.

## Installation

To install and run this project locally:

1. Clone the repository:
   ```bash
   git clone https://github.com/incogkenny/scheduler-simulator.git

2. Navigate to the project directory:
   ```bash
   cd scheduler-simulator
3. Compile the source code using gcc or any C compiler:
   ```bash
   gcc -o simulatorx simulatorx.c
4. Run chosen scheduler:
   ```bash
   ./simulatorx

## Supported Scheduling Algorithms
- First-Come, First-Served (FCFS): Processes are executed in the order they arrive.
- Shortest Job Next (SJN): The process with the shortest burst time is executed first.
- Priority Scheduling: Processes are executed based on priority levels.
- Round Robin (RR): Each process gets a fixed time slice in a cyclic order.
- Shortest Remaining Time First (SRTF): The process with the shortest remaining time is selected for execution.

  
