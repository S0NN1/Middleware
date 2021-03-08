package it.polimi.middlewaretechfordistsys.model;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

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
