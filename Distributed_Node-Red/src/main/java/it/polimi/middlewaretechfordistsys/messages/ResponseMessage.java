package it.polimi.middlewaretechfordistsys.messages;

public class ResponseMessage {
    private String destinationId;
    private String destinationIp;

    public ResponseMessage() {}

    public ResponseMessage(String destinationId, String destinationIp) {
        this.destinationId = destinationId;
        this.destinationIp = destinationIp;
    }

    public String getDestinationId() {
        return destinationId;
    }

    public String getDestinationIp() {
        return destinationIp;
    }
}
