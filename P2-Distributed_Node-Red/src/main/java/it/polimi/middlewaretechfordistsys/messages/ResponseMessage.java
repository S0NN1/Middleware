package it.polimi.middlewaretechfordistsys.messages;

public class ResponseMessage {
    private String destinationId;
    private String destinationIp;
    private String content;

    public ResponseMessage() {}

    public ResponseMessage(String destinationId, String destinationIp, String content) {
        this.destinationId = destinationId;
        this.destinationIp = destinationIp;
        this.content = content;
    }

    public String getDestinationId() {
        return destinationId;
    }

    public String getDestinationIp() {
        return destinationIp;
    }

    public String getContent() {
        return content;
    }
}
