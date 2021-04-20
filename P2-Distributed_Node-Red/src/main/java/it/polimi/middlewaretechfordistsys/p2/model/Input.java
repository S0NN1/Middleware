package it.polimi.middlewaretechfordistsys.p2.model;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * It encapsulate the content of an input message which needs to be exchanged from two different nodes.
 * It is mapped to the json standard input using the Jackson Library.
 */
public class Input {
    private final String destinationId;
    private final String content;

    @JsonCreator
    public Input(@JsonProperty("destinationId") String destinationId, @JsonProperty("content") String content) {
        this.destinationId = destinationId;
        this.content = content;
    }

    public String getDestinationId() {
        return destinationId;
    }

    public String getContent() {
        return content;
    }
}
