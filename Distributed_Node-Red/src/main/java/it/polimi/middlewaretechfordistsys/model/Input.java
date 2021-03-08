package it.polimi.middlewaretechfordistsys.model;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

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
