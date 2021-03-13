package it.polimi.middlewaretechfordistsys.p2.messages;

import it.polimi.middlewaretechfordistsys.p2.model.Node;

/**
 * It is sent from the client actor to the compute one, and contains the information about a new node that wants to join
 * the system, which are encapsulated in the node class.
 * @see Node
 */
public class RegistrationMessage {
    private final Node node;

    public RegistrationMessage(Node node) {
        this.node = node;
    }

    public Node getNode() {
        return node;
    }
}
