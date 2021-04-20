package it.polimi.middlewaretechfordistsys.p2.messages;

/**
 * It is sent from the compute actor to the client one when a new message needs to be exchanged from two different nodes.
 * In fact, it provides all the information necessary to correctly instradate the message to the new node.
 */
public class ResponseMessage {
    private String destinationId;
    private String destinationIp;
    private String destinationPort;
    private String content;

    public ResponseMessage() {}

    public ResponseMessage(String destinationId, String destinationIp, String content, String destinationPort) {
        this.destinationId = destinationId;
        this.destinationIp = destinationIp;
        this.content = content;
        this.destinationPort = destinationPort;
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

    public String getDestinationPort() {
        return destinationPort;
    }
}
