package it.polimi.middlewaretechfordistsys.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.ActorSystem;
import akka.actor.Props;
import it.polimi.middlewaretechfordistsys.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.messages.ResponseMessage;

public class ClientActor extends AbstractActor {
    private final ActorSystem actorSystem;
    private final ActorRef server;

    public ClientActor() {
        actorSystem = ActorSystem.create("ClientActor");
        server = actorSystem.actorOf(ComputeActor.props(), "server");
    }

    @Override
    public Receive createReceive() {
        return receiveBuilder()
                .match(NodeRedMessage.class, this::onNodeMessage)
                .match(ResponseMessage.class, this::onResponse)
                .match(RegistrationMessage.class, this::onRegistrationMessage)
                .match(RegistrationConfirmationMessage.class, this::onRegistrationConfirmation)
                .build();
    }

    public void onNodeMessage(NodeRedMessage message) {
        server.tell(message, this.self());
    }

    public void onRegistrationConfirmation(RegistrationConfirmationMessage message) {
        System.out.println("Registration ack received");
    }

    public void onRegistrationMessage(RegistrationMessage message) {
        server.tell(message, self());
    }

    public void onResponse(ResponseMessage responseMessage) {

    }

    public static Props props() {
        return Props.create(ClientActor.class);
    }

}
