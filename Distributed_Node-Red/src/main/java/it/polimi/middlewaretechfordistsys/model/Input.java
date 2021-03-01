package it.polimi.middlewaretechfordistsys.model;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

public class Input {
    private final String destinationId;

    @JsonCreator
    public Input(@JsonProperty("destinationId") String destinationId) {
        this.destinationId = destinationId;
    }

    public String getDestinationId() {
        return destinationId;
    }
}
