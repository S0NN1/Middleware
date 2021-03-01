package it.polimi.middlewaretechfordistsys.messages;

public class NodeRedMessage {
    private String destinationId;

    public NodeRedMessage(String destinationId) {
        this.destinationId = destinationId;
    }

    public String getDestinationId() {
        return destinationId;
    }
}
