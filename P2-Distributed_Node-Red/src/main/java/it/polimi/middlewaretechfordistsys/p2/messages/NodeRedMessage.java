package it.polimi.middlewaretechfordistsys.p2.messages;

/**
 * This message is exchanged when a node on client side wants to send something to another one.
 * It contains only the destination id and a content String. Since the application is requested to be agnostic to the
 * content of message, my assumption is that nodes exchange messages following the json standard.
 */
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
