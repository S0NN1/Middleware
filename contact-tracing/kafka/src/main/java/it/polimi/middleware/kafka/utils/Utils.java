package it.polimi.middleware.kafka.utils;

import io.confluent.ksql.api.client.Client;
import io.confluent.ksql.api.client.ClientOptions;
import org.apache.kafka.common.serialization.StringSerializer;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Properties;

/**
 * Utils class used for setup function (initially thought to be used in multiple classes).
 */
public class Utils {
    /**
     * Sets producer props from kafka.properties.
     * @return the producer props.
     */
    public static Properties setupProducerProps() {
        Properties props = new Properties();
        try {
            FileInputStream file = new FileInputStream("./kafka.properties");
            props.load(file);
        } catch (FileNotFoundException e) {
            System.err.println("Config properties file not found, please be sure you have setup a kafka.properties " +
                    "file on the same root folder of the jar");
        } catch (IOException e) {
            e.printStackTrace();
        }
        return props;
    }

    /**
     * Create ksql client client with host and port defined in ksql.properties.
     *
     * @return the client.
     */
    public static Client createKSQLClient() {
        Properties props = new Properties();
        try {
            FileInputStream file = new FileInputStream("./ksql.properties");
            props.load(file);
            ClientOptions options = ClientOptions.create()
                    .setHost(props.getProperty("server"))
                    .setPort(Integer.parseInt(props.getProperty("port")));
            return Client.create(options);

        } catch (FileNotFoundException e) {
            System.err.println("Config properties file not found, please be sure you have setup a ksql.properties " +
                    "file on the same root folder of the jar");
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

}

