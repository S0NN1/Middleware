package it.polimi.middlewaretechfordistsys.p2.model;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * It encapsulate the information about a registered node, which are basically its identifier, its ip address and its
 * node-red running port.
 */
public class Node {
    private final String id;
    private final String ip;
    private final String port;

    @JsonCreator
    public Node(@JsonProperty("id") String id, @JsonProperty("ip") String ip, @JsonProperty("port") String port) {
        this.id = id;
        this.ip = ip;
        this.port = port;
    }

    public String getId() {
        return id;
    }

    public String getIp() {
        return ip;
    }

    public String getPort() {
        return port;
    }
}
