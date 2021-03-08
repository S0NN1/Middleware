package it.polimi.middlewaretechfordistsys.messages;

import it.polimi.middlewaretechfordistsys.model.Node;

public class RegistrationMessage {
    private final Node node;

    public RegistrationMessage(Node node) {
        this.node = node;
    }

    public Node getNode() {
        return node;
    }
}
