# A model for virus spreading

<img src="https://upload.wikimedia.org/wikipedia/commons/6/6f/Open_MPI_logo.png" width=192px height=192 px align="right" >

![latest release](https://img.shields.io/github/v/release/ArmelliniFederico/Middleware?color=green)

A simple implementation of a virus spreading model made with MPI.

## Description

This project is part of an assignment for Middleware course at Politecnico di Milano Year 2021.

It consists in a realization of a distributed virus spreading computational model.

It is realized with the Message Passing Interface technology, written in C language.

## Specification

Full specification can be found [**here**](../specs/specification.pdf) under Project 3 section.

## Architecture

![lel](.github/images/UML.png)



## Platforms

- [**MPI**](https://www.open-mpi.org/)

## Documentation

You can find detailed documentation at:
- [**Akka Node-Red backend**](https://pirox4256.github.io/node-red-javadocs/)

## Requirements

- [C Programming Language](https://www.learn-c.org/)

## Installation

### OpenMpi
Update apt packages by running:

```sudo apt-get update -y```.

Install openMPI by running:

 `sudo apt-get install -y openmpi-bin`.

Test the installation by running:

`mpiexec`.

### Repository
Simply clone the repository in a local folder and extract the project directory.

## Running

### Compilation
Open a terminal and run the following command:

`mpicc -o out project4_virus_spreading.c  -lm`

### Execution
You can launch the execution on the local machine by running:

```mpiexec -n WorldSize executable_path numInfectedTotal numPeopleTotal dimWorldX dimWorldY days dimSubNationX dimSubNationY timestep distance velocity```

If you want to run the program in cluster mode, you have to configure your hostfile according to the [MPI Documentation](https://www.open-mpi.org/faq/?category=running).

Once configured, you can run:

`mpiexec --hostfile <path/to/hostfile> -np <number-of-processes-to-launch> -n WorldSize executable_path numInfectedTotal numPeopleTotal dimWorldX dimWorldY days dimSubNationX dimSubNationY timestep distance velocity`
