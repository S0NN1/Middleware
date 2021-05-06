package it.polimi.middleware.kafka.producers;

import io.confluent.ksql.api.client.*;
import io.vertx.core.json.JsonArray;
import it.polimi.middleware.kafka.utils.Utils;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.Producer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.sql.Timestamp;
import java.time.Duration;
import java.util.*;
import java.util.concurrent.ExecutionException;

/**
 * The KafkaSQLProducer class instantiates both a Kafka producer and a KSQL client (for Confluent Platform).
 * <p>
 * With the second one it creates a KSQL Stream for incoming connections from the MQTTSourceConnector writing to the
 * mqtt-to-kafka-alerts topic; then this stream is materialized into a KSQL table (CONNECTION_TABLE) which prevents
 * duplicates and lets the use of KSQL queries for retrieving results.
 * <p>
 * Each result is a client ID of a Contiki mote which established a connection with the mote firing the alert and
 * it is used for every alert received in order to encapsulated it in a ProducerRecord and send it to the
 * kafka-to-mqtt-alerts topic.
 *
 * @author Sonny
 * @version 1.0
 */
public class KafkaKSQLProducer {
    private static final Logger logger = LoggerFactory.getLogger("Logger");
    private static final String connectionStream = "CREATE STREAM CONNECTIONS (" +
            "EventType VARCHAR , " +
            "ClientId VARCHAR, " +
            "SenderClientId VARCHAR) " +
            "WITH (" +
            "KAFKA_TOPIC='mqtt-to-kafka-connections', " +
            "VALUE_FORMAT='json');";
    private static final String alertStream = "CREATE STREAM ALERTS (" +
            "EventType VARCHAR , " +
            "ClientId VARCHAR) " +
            "WITH (" +
            "KAFKA_TOPIC='mqtt-to-kafka-alerts', " +
            "VALUE_FORMAT='json');";
    private static final String connectionTableQuery = """
            CREATE TABLE CONNECTION_TABLE AS
                SELECT CLIENTID,
                COLLECT_SET(SENDERCLIENTID) AS SENDERCLIENTID
                FROM  CONNECTIONS\s
                GROUP BY CLIENTID
                EMIT CHANGES;""";
    private static final String ksqlQuery = "SELECT * FROM ALERTS EMIT CHANGES;";

    /**
     * The entry point of application.
     *
     * @param args the input arguments.
     * @throws ExecutionException   the execution exception.
     * @throws InterruptedException the interrupted exception.
     */
    public static void main(String[] args) throws ExecutionException, InterruptedException {
        //Kafka producer settings.
        Properties props = Utils.setupProducerProps();
        Producer<String, String> producer = new KafkaProducer<>(props);
        //KSQL client creation.
        Client ksqlClient = Utils.createKSQLClient();
        //List current KSQL streams on the broker/cluster.
        List<StreamInfo> streams = ksqlClient.listStreams().get();
        boolean alertStreamFound = false;
        boolean connectionStreamFound = false;

        for (StreamInfo stream : streams) {
            if (stream.getName().equals("ALERTS")) {
                alertStreamFound = true;
            } else if (stream.getName().equals("CONNECTIONS")) {
                connectionStreamFound = true;
            }
        }
        if (!alertStreamFound) {
            //Create Kafka Alert Stream
            ksqlClient.executeStatement(alertStream).get();
        }
        if (!connectionStreamFound) {
            //Create Kafka Connection Stream
            ksqlClient.executeStatement(connectionStream).get();
        }
        boolean connectionTableFound = false;
        //List current KSQL tables on the broker/cluster.
        List<TableInfo> tables = ksqlClient.listTables().get();
        for (TableInfo table : tables) {
            if (table.getName().equals("CONNECTION_TABLE")) {
                connectionTableFound = true;
            }
        }
        if (!connectionTableFound) {
            //Create Kafka Connection Table
            Map<String, Object> properties = Collections.singletonMap("auto.offset.reset", "earliest");
            ksqlClient.executeStatement(connectionTableQuery, properties).get();
        }
        /*Stream query, particular KSQL query which runs continuously (the main benefit of using KSQLdb instead of a
        regular DB).*/
        StreamedQueryResult streamedQueryResult = ksqlClient.streamQuery(ksqlQuery).get();
        while (true) {
            //As soon as it receives a new row
            Row row = streamedQueryResult.poll(Duration.ofSeconds(40));
            if (row != null) {
                //Get client ID
                String clientId = row.values().getString(1);
                //Create KSQL query for CONNECTION_TABLE
                String pullQuery = "SELECT * FROM CONNECTION_TABLE WHERE CLIENTID='" + clientId + "';";
                Map<String, Object> properties = new HashMap<>();
                properties.put("auto.offset.reset", "latest");
                //Property needed for KSQL v0.17 in order to unlock usage of non-key queries.
                properties.put("ksql.query.pull.table.scan.enabled", true);
                List<Row> queryResult1 = ksqlClient.executeQuery(pullQuery, properties).get();
                logger.info("Received an alert from client " + clientId + ". Creating a mqtt alert message...");
                //Each client ID ha a column in the table with a JsonArray storing all motes encountered.
                for (Row result : queryResult1) {
                    JsonArray array = (JsonArray) result.getValue(2);
                    for (Object json : array) {
                        Timestamp timestamp = new Timestamp(System.currentTimeMillis());
                        String value = "{" +
                                "\"AlertDest\":\"" + json + "\", " +
                                "\"AlertSource\":\"" + clientId + "\"" +
                                "}";
                        //Write on Kafka topic.
                        producer.send(new ProducerRecord<>("kafka-to-mqtt-alerts", timestamp.toString(), value));
                    }
                }
                logger.info("Alerts sent to clients who has been in contact with " + clientId);
            }
        }
    }
}
