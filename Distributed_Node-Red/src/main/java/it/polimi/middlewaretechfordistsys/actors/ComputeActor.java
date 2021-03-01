package it.polimi.middlewaretechfordistsys.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.Props;
import it.polimi.middlewaretechfordistsys.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.messages.ResponseMessage;
import it.polimi.middlewaretechfordistsys.model.Node;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class ComputeActor extends AbstractActor {
    private final List<Node> nodes = new ArrayList<>();
    Logger logger = LoggerFactory.getLogger("LOG");

    @Override
    public Receive createReceive() {
        return receiveBuilder()
                .match(NodeRedMessage.class, this::onMessage)
                .match(RegistrationMessage.class, this::onRegistration)
                .build();
    }

    public void onRegistration(RegistrationMessage registrationMessage) {
        for(Node item : nodes) {
            if(item.getId().equals(registrationMessage.getNode().getId())) {
                sender().tell(new RegistrationConfirmationMessage(), sender());
            }
        }
        nodes.add(registrationMessage.getNode());
        System.out.println("Node " + registrationMessage.getNode().getId() + " added correctly");
        sender().tell(new RegistrationConfirmationMessage(), ActorRef.noSender());
    }

    public void onMessage(NodeRedMessage message) {
        ResponseMessage responseMessage = new ResponseMessage(message.getDestinationId());
        for(Node item : nodes) {
            if(item.getId().equals(message.getDestinationId())) {
                System.out.println("The destination node was found!");
                return;
            }
        }
        System.out.println("No destination node found!");
    }

    static Props props() {
        return Props.create(ComputeActor.class);
    }

}
