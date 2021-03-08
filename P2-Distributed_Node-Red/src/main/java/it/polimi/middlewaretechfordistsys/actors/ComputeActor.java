package it.polimi.middlewaretechfordistsys.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.Props;
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
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

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

    public void onMessage(NodeRedMessage message) {
        if(message.getDestinationId() == null) {
            Random random = new Random();
            Node node = nodes.get(random.nextInt(nodes.size()-1));
            if(pingHost(node.getIp(), Integer.parseInt(node.getPort()), 2000)) {
                System.out.println("The destination node was chosen randomly! It is " + node.getId());
                sender().tell(new ResponseMessage(node.getId(), node.getIp(), message.getContent(), node.getPort()), ActorRef.noSender());
                return;
            }
            nodes.remove(node);
        }
        else {
            for (Node item : nodes) {
                if (item.getId().equals(message.getDestinationId())) {
                    if(pingHost(item.getIp(), Integer.parseInt(item.getPort()), 2000)) {
                        System.out.println("The destination node was found!");
                        sender().tell(new ResponseMessage(item.getId(), item.getIp(), message.getContent(), item.getPort()), ActorRef.noSender());
                        return;
                    }
                    nodes.remove(item);
                }
            }
        }
        System.out.println("No destination node found!");
        sender().tell(new DestinationNotFoundException(), ActorRef.noSender());
    }

    public static Props props() {
        return Props.create(ComputeActor.class);
    }

    public static boolean pingHost(String host, int port, int timeout) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(host, port), timeout);
            socket.close();
            return true;
        } catch (IOException e) {
            return false; // Either timeout or unreachable or failed DNS lookup.
        }
    }
}
