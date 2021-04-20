package it.polimi.middlewaretechfordistsys.p2.actors;

import akka.actor.AbstractActor;
import akka.actor.ActorRef;
import akka.actor.Props;
import it.polimi.middlewaretechfordistsys.p2.exceptions.AlreadyRegisteredException;
import it.polimi.middlewaretechfordistsys.p2.exceptions.DestinationNotFoundException;
import it.polimi.middlewaretechfordistsys.p2.messages.NodeRedMessage;
import it.polimi.middlewaretechfordistsys.p2.messages.RegistrationConfirmationMessage;
import it.polimi.middlewaretechfordistsys.p2.messages.RegistrationMessage;
import it.polimi.middlewaretechfordistsys.p2.messages.ResponseMessage;
import it.polimi.middlewaretechfordistsys.p2.model.Node;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * This class represents the server computational actor, which can be seen as a sort of model of our application.
 * In fact, it contains the application logic necessary for a correct behavior of the system.
 * Its job is to:
 * - register new nodes in the list of the existent ones;
 * - retrieve information about them from the list of all nodes.
 */
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

    /**
     * This method is triggered when a new registration message is received from the client actor.
     * It checks if that node is already registered and, if not, proceeds to a registration.
     * @param registrationMessage the incoming registration message.
     */
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

    /**
     * This method is triggered when new node red message is received from the client actor.
     * It checks the destination node and ping it, in order to establish if it is still reachable. If the destination
     * is not reachable, it returns an exception to the client.
     * @param message the incoming message.
     * @see it.polimi.middlewaretechfordistsys.p2.messages.NodeRedMessage
     */
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

    /**
     * Static method to ping an host through a socket on the node-red running port.
     * @param host the host to ping.
     * @param port the port on which node-red is assumed to be running.
     * @param timeout a maximum timeout value.
     * @return true if the host is reachable, false if it times out.
     */
    public static boolean pingHost(String host, int port, int timeout) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(host, port), timeout);
            socket.close();
            return true;
        } catch (IOException e) {
            return false;
        }
    }
}
