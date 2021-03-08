package it.polimi.middlewaretechfordistsys.model;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

public class Node {
    private final String id;
    private final String ip;

    @JsonCreator
    public Node(@JsonProperty("id") String id, @JsonProperty("ip") String ip) {
        this.id = id;
        this.ip = ip;
    }

    public String getId() {
        return id;
    }

    public String getIp() {
        return ip;
    }

}
