package it.polimi.middlewaretechfordistsys.http;

import akka.Done;
import akka.NotUsed;
import akka.actor.ActorRef;
import akka.actor.typed.ActorSystem;
import akka.actor.typed.javadsl.Behaviors;
import akka.http.javadsl.Http;
import akka.http.javadsl.ServerBinding;
import akka.http.javadsl.marshallers.jackson.Jackson;
import akka.http.javadsl.model.StatusCodes;
import akka.http.javadsl.model.ws.Message;
import akka.http.javadsl.model.ws.TextMessage;
import akka.http.javadsl.model.ws.WebSocketRequest;
import akka.http.javadsl.model.ws.WebSocketUpgradeResponse;
import akka.http.javadsl.server.AllDirectives;
import akka.http.javadsl.server.Route;
import akka.japi.Pair;
import akka.pattern.Patterns;
import akka.stream.ActorMaterializer;
import akka.stream.Materializer;
import akka.stream.javadsl.Flow;
import akka.stream.javadsl.Keep;
import akka.stream.javadsl.Sink;
import akka.stream.javadsl.Source;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import it.polimi.middlewaretechfordistsys.actors.ClientActor;
import it.polimi.middlewaretechfordistsys.exceptions.AlreadyRegisteredException;
import it.polimi.middlewaretechfordistsys.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.messages.ResponseMessage;
import it.polimi.middlewaretechfordistsys.model.Input;
import it.polimi.middlewaretechfordistsys.model.Node;

import java.io.IOException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.*;

public class HttpServer extends AllDirectives {
    private final List<Node> registeredNodes = new ArrayList<>();
    private final akka.actor.ActorSystem actorSystem = akka.actor.ActorSystem.create("client");
    private final ActorRef client = actorSystem.actorOf(ClientActor.props(), "client");


    public static void main(String[] args) throws IOException {
        ActorSystem<Void> system = ActorSystem.create(Behaviors.<Void>empty(), "routes");
        final Http http = Http.get(system);

        HttpServer app = new HttpServer();

        final CompletionStage<ServerBinding> binding = http.newServerAt("0.0.0.0", 8080).bind(app.createRoute());

        System.out.println("Server started at http://localhost:8080 \nPress RETURN to stop.");
        System.in.read();

        binding.thenCompose(ServerBinding::unbind).thenAccept(unbound -> system.terminate());
    }

    private CompletionStage<Done> registerNode(Node node) {
        for (Node item : registeredNodes) {
            if (item.getId().equals(node.getId())) {
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
                                    CompletableFuture<Object> registrationFuture = Patterns.ask(client, new RegistrationMessage(node), Duration.ofMillis(5000)).toCompletableFuture();
                                    Route routeDirectives = complete(StatusCodes.INTERNAL_SERVER_ERROR);
                                    try {
                                        routeDirectives = registrationFuture.thenApply(response -> {
                                            if (response == "KO") {
                                                return complete(StatusCodes.BAD_REQUEST);
                                            }
                                            return complete(StatusCodes.OK);
                                        }).get();
                                    } catch (InterruptedException | ExecutionException e) {
                                        e.printStackTrace();
                                    }
                                    return routeDirectives;
                                }))),
                post(() ->
                        path("send", () -> entity(Jackson.unmarshaller(Input.class), input -> {
                            CompletableFuture<Object> sendFuture = Patterns.ask(client, new NodeRedMessage(input.getDestinationId(), input.getContent()), Duration.ofSeconds(5)).toCompletableFuture();
                            Route route = complete(StatusCodes.INTERNAL_SERVER_ERROR);
                            try {
                                route = sendFuture.thenApply(response -> {
                                    if (response == "KO") {
                                        return complete(StatusCodes.BAD_REQUEST);
                                    } else if (response instanceof ResponseMessage) {
                                        ExecutorService executorService = Executors.newFixedThreadPool(8);
                                        executorService.submit(() -> {
                                            Http http = Http.get(actorSystem);
                                            Materializer materializer = ActorMaterializer.create(actorSystem);
                                            final Sink<Message, CompletionStage<Done>> printSink =
                                                    Sink.foreach((message) ->
                                                            System.out.println("Got message: " + message.asTextMessage().getStrictText())
                                                    );
                                            Map<String, String> map = new HashMap<>() {{
                                                put("destinationId", ((ResponseMessage) response).getDestinationId());
                                                put("content", ((ResponseMessage) response).getContent());
                                            }};
                                            ObjectMapper objectMapper = new ObjectMapper();
                                            try {
                                                // send this as a message over the WebSocket
                                                final Source<Message, NotUsed> helloSource =
                                                        Source.single(TextMessage.create(objectMapper.writerWithDefaultPrettyPrinter().writeValueAsString(map)));
                                                final Flow<Message, Message, CompletionStage<Done>> flow =
                                                        Flow.fromSinkAndSourceMat(printSink, helloSource, Keep.left());
                                                String kek = "ws://" + ((ResponseMessage) response).getDestinationIp() + ":" + ((ResponseMessage)response).getDestinationPort() + "/ws/" + ((ResponseMessage)response).getDestinationId();
                                                final Pair<CompletionStage<WebSocketUpgradeResponse>, CompletionStage<Done>> pair =
                                                        http.singleWebSocketRequest(
                                                                WebSocketRequest.create("ws://" + ((ResponseMessage) response).getDestinationIp() + ":" + ((ResponseMessage)response).getDestinationPort() + "/ws/" + ((ResponseMessage)response).getDestinationId()),
                                                                flow,
                                                                materializer
                                                        );
                                            } catch (JsonProcessingException e) {
                                                e.printStackTrace();
                                            }
                                        });
                                        return complete(StatusCodes.OK);
                                    }
                                    return complete(StatusCodes.INTERNAL_SERVER_ERROR);
                                }).get();
                            } catch (InterruptedException | ExecutionException e) {
                                e.printStackTrace();
                            }
                            return route;
                        }))
                )
        );
    }
}
