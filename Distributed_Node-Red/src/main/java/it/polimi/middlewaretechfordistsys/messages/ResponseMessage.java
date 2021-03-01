package it.polimi.middlewaretechfordistsys.messages;

public class ResponseMessage {
    private String destinationId;

    public ResponseMessage() {}

    public ResponseMessage(String destinationId) {
        this.destinationId = destinationId;
    }

    public String getDestinationId() {
        return destinationId;
    }
}
