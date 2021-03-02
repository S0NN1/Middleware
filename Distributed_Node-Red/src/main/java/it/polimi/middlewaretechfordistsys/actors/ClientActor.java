package it.polimi.middlewaretechfordistsys.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.ActorSystem;
import akka.actor.Props;
import akka.pattern.Patterns;
import it.polimi.middlewaretechfordistsys.exceptions.AlreadyRegisteredException;
import it.polimi.middlewaretechfordistsys.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.messages.ResponseMessage;

import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ClientActor extends AbstractActor {
    private final ActorSystem actorSystem;
    private final ActorRef server;
    private final int numThreads = 16;
    final ExecutorService executorService = Executors.newFixedThreadPool(numThreads);

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
                .build();
    }

    public void onNodeMessage(NodeRedMessage message) {
        server.tell(message, this.self());
    }

    public void onRegistrationConfirmation(RegistrationConfirmationMessage message) {
        System.out.println("Registration ack received");
    }

    public void onRegistrationFailed(AlreadyRegisteredException e) {
        System.out.println("KEK");
    }

    public void onRegistrationMessage(RegistrationMessage message) throws InterruptedException {
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
