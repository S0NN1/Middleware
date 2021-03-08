package it.polimi.middlewaretechfordistsys.messages;

public class NodeRedMessage {
    private final String destinationId;
    private final String content;

    public NodeRedMessage(String destinationId, String content) {
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
