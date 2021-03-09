package it.polimi.middlewaretechfordistsys.p2;

import it.polimi.middlewaretechfordistsys.p2.http.HttpServer;

import java.io.IOException;

/**
 * Akka server launcher
 */
public class App {
    public static void main(String[] args) throws IOException {
        System.out.println("Starting application...");
        HttpServer.main(args);
    }
}
