# Contiki-NG with Apache Kafka Contact Tracing

<img src=".github/images/logo.png" width=192px height=192 px align="right" >

![latest release](https://img.shields.io/github/v/release/ArmelliniFederico/Middleware?color=green)

A simple integration between Apache Kafka and Contiki-NG.

## Description

This project is part of an assignment for Middleware course at Politecnico di Milano Year 2021.

It consists in a contact tracing implementation using IoT devices with Contiki-NG (with custom processes) and Apache Kafka as the middleware layout for the backend.

Each mote communicates with the other ones using UDP, reports these connections via MQTT to the Kafka cluster, which it stores them into a KSQL db.

At random intervals, the motes send alerts to the broker which then publishes via MQTT the event later caught by each mote involved.

## Specification

Full specification can be found [**here**](../specs/specification.pdf) under Project 1 section.

## Architecture

![lel](.github/images/middleware.png)

The general Architecture consists into:

- Contiki motes with a RPL Border Router for communication between them and outsides networks.
- 2 MQTT brokers, one acting in bridge mode and the other one on the remote server.
- 2 Kafka Connect connectors transferring from MQTT topics to Kafka ones.
- Apache Kafka system with a cluster, a broker and multiple topics.
- 2 KSQL streams, one for connections between nodes and the other one for incoming alerts.
- One KSQL table for storing connections.
- One jar program querying the table and instantiating a Kafka producer in order to publish on a topic.

## Platforms

- [**Cooja simulator**](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Running-Contiki%E2%80%90NG-in-Cooja): realtime simulation of Contiki IoT motes.
- [**Confluent Platform**](https://www.confluent.io/product/confluent-platform/): deployment of Apache Kafka with a view to enterprise production and distribution.
- [**Docker**](https://www.docker.com/): easy management of Apache Kafka and Confluent Platform with images and containers.

## Requirements

- [Docker](https://docs.docker.com/get-docker/) (remote server).
- [Java JDK 16](https://openjdk.java.net/projects/jdk/16/) (remote server).

## Installation

### Client side

**Contiki-NG**

In order to work Contiki-NG needs a few dependencies, you can use different options:

- Docker image: [here](https://github.com/contiki-ng/contiki-ng/wiki/Docker).
- Native toolchain installation (Linux): [here](https://github.com/contiki-ng/contiki-ng/wiki/Toolchain-installation-on-Linux)
- Virtual Image running on Virtualbox: [here]()

Assuming you have chosen the third option, after running the VM you need to install Contiki-NG repo by executing the following commands:

```bash
cd ~

git clone https://github.com/contiki-ng/contiki-ng.git

cd contiki-ng

git submodule update --init --recursive
```

**MQTT broker**

Install mosquitto package based on your distro (the following command is for Debian/Ubuntu based distros) [user password is `user`]:

```bash
sudo apt install mosquitto
```

Then edit its service file under `/etc/mosquitto/mosquitto.conf` by adding the following lines:

```
connection bridge-01
address <remote_server_ip>:1883
topic # out 0
topic # in 0
```

Enable systemd service and start it for mosquitto broker by using:

```bash
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

### Server side

**MQTT broker**

Install mosquitto package based on your distro (the following command is for Debian/Ubuntu based distros) [user password is `user`]:

```bash
sudo apt install mosquitto
```

Enable the mosquitto service and start it by typing (Ubuntu/Debian):

```bash
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

**Confluent Platform**

You can follow the Confluent [**guide**]() if you want to setup individually each component, otherwise you can follow the remaining of this section.

> **❗ IMPORTANT**
>
> Be sure to change under broker section of `docker-compose.yml` file the `KAFKA_ADVERTISED_LISTENERS` field.
>
> Replace `<remote-server-ip>` with your corresponding remote server IP.

Assuming you have installed Docker, install all images and create related containers by placing the two docker files contained in the [**`configs`**](https://github.com/ArmelliniFederico/Middleware/tree/main/contact-tracing/configs) folder into a directory of your choice and running:

```bash
docker-compose up -d
```

Check if all containers are running by typing:

```bash
docker-compose ps
```

You should see an output like this:

![kek](.github/images/docker-compose-ps-screen.png)

After waiting for about 3 minutes you should reach (assuming you have done port-forwarding on your server) the Confluent Control Center at `REMOTE_SERVER_IP_ADDRESS:9021`.

![kek](.github/images/control-center-screen.png)

You can now create Kafka topics by clicking on the cluster and under the `Topics` section

![kek](.github/images/topics-screen.png)

Create three topics with the following names (change them if you want to customize them as described in the **Configuration** section):

- mqtt-to-kafka-connections
- mqtt-to-kafka-alerts
- kafka-to-mqtt-alerts

Each topic must have `Replication factor` set to 1 like the following.

![kek](.github/images/replication-screen.png)

Now setup the two connectors responsible to read and write from MQTT to Kafka and vice versa.

Open `Connect` section and choose the default one.

![kek](.github/images/connect-screen.png)

Then create two MQTTSourceConnector with the following settings:

![kek](.github/images/mqtt-source-screen.png)

Finally one MQTTSinkConnector with this config:

![kek](.github/images/mqtt-sink-screen.png)

You should see their status as `Running`, otherwise you can

## Configuration

**`contact-tracing.c`**

**`KafkaKsqlProducer.jar`**

## Running

### Native

### COOJA simulator
