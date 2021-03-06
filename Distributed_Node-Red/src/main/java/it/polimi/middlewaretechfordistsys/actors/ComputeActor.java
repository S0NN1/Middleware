package it.polimi.middlewaretechfordistsys.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.Props;
import akka.actor.Status;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import it.polimi.middlewaretechfordistsys.exceptions.AlreadyRegisteredException;
import it.polimi.middlewaretechfordistsys.exceptions.DestinationNotFoundException;
import it.polimi.middlewaretechfordistsys.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.messages.ResponseMessage;
import it.polimi.middlewaretechfordistsys.model.Node;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

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
                sender().tell(new AlreadyRegisteredException(), ActorRef.noSender());
                return;
            }
        }
        nodes.add(registrationMessage.getNode());
        System.out.println("Node " + registrationMessage.getNode().getId() + " added correctly");
        sender().tell(new RegistrationConfirmationMessage(), ActorRef.noSender());
    }

    public void onMessage(NodeRedMessage message) throws IOException, InterruptedException {
        for(Node item : nodes) {
            if(item.getId().equals(message.getDestinationId())) {
                ResponseMessage responseMessage = new ResponseMessage(item.getId(), item.getIp());
                System.out.println("The destination node was found!");
                sender().tell(responseMessage, ActorRef.noSender());
                return;
            }
        }
        System.out.println("No destination node found!");
        sender().tell(new DestinationNotFoundException(), ActorRef.noSender());
    }

    public static Props props() {
        return Props.create(ComputeActor.class);
    }

}
