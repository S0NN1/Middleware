package it.polimi.middlewaretechfordistsys.http;

import akka.Done;
import akka.actor.ActorRef;
import akka.actor.typed.ActorSystem;
import akka.actor.typed.javadsl.Behaviors;
import akka.http.javadsl.Http;
import akka.http.javadsl.ServerBinding;
import akka.http.javadsl.marshallers.jackson.Jackson;
import akka.http.javadsl.model.StatusCodes;
import akka.http.javadsl.server.AllDirectives;
import akka.http.javadsl.server.Route;
import akka.pattern.Patterns;
import akka.util.Timeout;
import it.polimi.middlewaretechfordistsys.actors.ClientActor;
import it.polimi.middlewaretechfordistsys.exceptions.AlreadyRegisteredException;
import it.polimi.middlewaretechfordistsys.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.model.Input;
import it.polimi.middlewaretechfordistsys.model.Node;
import scala.concurrent.Await;
import scala.concurrent.Future;
import scala.concurrent.duration.FiniteDuration;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class HttpServer extends AllDirectives {
    private final List<Node> registeredNodes = new ArrayList<>();
    private final akka.actor.ActorSystem actorSystem = akka.actor.ActorSystem.create("client");
    private final ActorRef client = actorSystem.actorOf(ClientActor.props(), "client");

    public static void main(String[] args) throws IOException {
        ActorSystem<Void> system = ActorSystem.create(Behaviors.<Void>empty(), "routes");
        final Http http = Http.get(system);

        HttpServer app = new HttpServer();

        final CompletionStage<ServerBinding> binding = http.newServerAt("localhost", 8080).bind(app.createRoute());

        System.out.println("Server started at http://localhost:8080 \nPress RETURN to stop.");
        System.in.read();

        binding.thenCompose(ServerBinding::unbind).thenAccept(unbound -> system.terminate());
    }

    private CompletionStage<Done> registerNode(Node node) {
        for(Node item : registeredNodes) {
            if(item.getId().equals(node.getId())) {
                return CompletableFuture.failedFuture(new AlreadyRegisteredException());
            }
        }
        registeredNodes.add(node);
        System.out.println(registeredNodes.size());
        return CompletableFuture.completedFuture(Done.getInstance());
    }

    private Route createRoute() {
        return concat(
                post(() ->
                        path("register", () ->
                                entity(Jackson.unmarshaller(Node.class), node -> {
                                    client.tell(new RegistrationMessage(node), ActorRef.noSender());
                                    return complete(StatusCodes.OK);
                                    /*CompletionStage<Done> futureSaved = registerNode(node);
                                    if(futureSaved.toCompletableFuture().isCompletedExceptionally()) {
                                        return complete(StatusCodes.UNPROCESSABLE_ENTITY, "Node already registered");
                                    }
                                    return onSuccess(futureSaved, done ->
                                            complete("Node successfully registered!")
                                    );*/
                                }))),
                get(() ->
                        path("send", () -> entity(Jackson.unmarshaller(Input.class), input -> {
                            client.tell(new NodeRedMessage(input.getDestinationId()), ActorRef.noSender());
                            return complete(StatusCodes.OK);
                        }))
                        )
        );
    }
}
