package it.polimi.middlewaretechfordistsys.p2.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.ActorSystem;
import akka.actor.Props;
import akka.pattern.Patterns;
import it.polimi.middlewaretechfordistsys.p2.exceptions.AlreadyRegisteredException;
import it.polimi.middlewaretechfordistsys.p2.exceptions.DestinationNotFoundException;
import it.polimi.middlewaretechfordistsys.p2.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.p2.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.p2.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.p2.messages.ResponseMessage;

import java.time.Duration;
import java.util.concurrent.CompletableFuture;

/**
 * This class represents the client actor, which receives information from the http server and ask to the compute actor
 * for a computation, which can be a new node registration or a request of a destination node information (ip, port, etc.)
 */
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
                .match(AlreadyRegisteredException.class, this::onRegistrationFailed)
                .match(DestinationNotFoundException.class, this::onDestinationNotFound)
                .build();
    }

    /**
     * This method is triggered when a NodeRedMessage is received. It simply sends it to the compute actor.
     * @param message the message to be passed.
     */
    public void onNodeMessage(NodeRedMessage message) {
        CompletableFuture<Object> completableFuture = Patterns.ask(server, message, Duration.ofSeconds(5)).toCompletableFuture();
        completableFuture.thenApply(resp -> {
            if(resp instanceof ResponseMessage) {
                sender().tell(resp, ActorRef.noSender());
            }
            else {
                sender().tell("KO", ActorRef.noSender());
            }
            return null;
        }).join();

    }

    public void onDestinationNotFound(DestinationNotFoundException message) {}

    /**
     * This method is triggered when a confirmation of registration is received from the compute actor.
     * @param message the confirmation message.
     */
    public void onRegistrationConfirmation(RegistrationConfirmationMessage message) {
        System.out.println("Registration ack received");
    }

    public void onRegistrationFailed(AlreadyRegisteredException e) {}

    public void onRegistrationMessage(RegistrationMessage message) {
        CompletableFuture<Object> kek = Patterns.ask(server, message, Duration.ofSeconds(5)).toCompletableFuture();
        kek.thenApply(resp -> {
            if(resp instanceof RegistrationConfirmationMessage) {
                sender().tell("OK", ActorRef.noSender());
            }
            else {
                sender().tell("KO", ActorRef.noSender());
            }
            return null;
        }).join();
    }

    public void onResponse(ResponseMessage responseMessage) {

    }

    public static Props props() {
        return Props.create(ClientActor.class);
    }
}
