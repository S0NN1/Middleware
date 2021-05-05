package it.polimi.middleware.apache.kafka.connect.producers;

import io.confluent.ksql.api.client.*;
import it.polimi.middleware.apache.kafka.connect.utils.Utils;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.Producer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.concurrent.ExecutionException;

import static it.polimi.middleware.apache.kafka.connect.utils.Utils.createKSQLClient;

public class MainProducer {
    private static final Logger logger = LoggerFactory.getLogger("Logger");
    private static final String connectionStream = "CREATE STREAM CONNECTIONS (" +
            "EventType VARCHAR , " +
            "ClientId VARCHAR, " +
            "SenderClientId VARCHAR, " +
            "Identifier VARCHAR) " +
            "WITH (" +
            "KAFKA_TOPIC='mqtt-to-kafka-connections', " +
            "VALUE_FORMAT='json');";
    private static final String alertStream = "CREATE STREAM ALERTS (" +
            "EventType VARCHAR , " +
            "ClientId VARCHAR) " +
            "WITH (" +
            "KAFKA_TOPIC='mqtt-to-kafka-alerts', " +
            "VALUE_FORMAT='json');";
    private static final String connectionTableQuery= "CREATE TABLE CONNECTION_TABLE AS\n" +
            "    SELECT IDENTIFIER,\n" +
            "    LATEST_BY_OFFSET(CLIENTID) AS CLIENTID,\n" +
            "    LATEST_BY_OFFSET(SENDERCLIENTID) AS SENDERCLIENTID\n" +
            "    FROM  CONNECTIONS \n" +
            "    GROUP BY IDENTIFIER\n" +
            "    EMIT CHANGES;";
    private static final String ksqlQuery = "SELECT * FROM ALERTS EMIT CHANGES;";

    public static void main(String[] args) throws ExecutionException, InterruptedException {
        Properties props = Utils.setupProducerProps();
        Producer<String, String> producer = new KafkaProducer<String, String>(props);
        Client ksqlClient = createKSQLClient();
        List<StreamInfo> streams = ksqlClient.listStreams().get();
        boolean alertStreamFound = false;
        boolean connectionStreamFound = false;
        for(StreamInfo stream : streams) {
            if(stream.getName().equals("ALERTS")){
                alertStreamFound = true;
            }
            else if(stream.getName().equals("CONNECTIONS")) {
                connectionStreamFound = true;
            }
        }
        if(!alertStreamFound){
            //Create Kafka Alert Stream
            ksqlClient.executeStatement(alertStream).get();
        }
        if (!connectionStreamFound){
            //Create Kafka Connection Stream
            ksqlClient.executeStatement(connectionStream).get();
        }
        boolean connectionTableFound = false;
        List<TableInfo> tables = ksqlClient.listTables().get();
        for (TableInfo table : tables){
            if (table.getName().equals("CONNECTION_TABLE")){
                connectionTableFound = true;
            }
        }
        if (!connectionTableFound){
            Map<String, Object> properties = Collections.singletonMap("auto.offset.reset", "earliest");
            ksqlClient.executeStatement(connectionTableQuery, properties).get();
        }
        StreamedQueryResult streamedQueryResult = ksqlClient.streamQuery(ksqlQuery).get();
        while(true) {
            Row row = streamedQueryResult.poll();
            String clientId = row.values().getString(1);
            String pullQuery = "SELECT * FROM CONNECTION_TABLE WHERE CLIENTID='"+ clientId + "';";
            Map<String, Object> properties = new HashMap<>();
            properties.put("auto.offset.reset", "latest");
            properties.put("ksql.query.pull.table.scan.enabled", true);
            List<Row> queryResult1 = ksqlClient.executeQuery(pullQuery, properties).get();
            logger.info("Received an alert from client " + clientId + ". Creating a mqtt alert message...");
            for (Row result : queryResult1){
               Random random = new Random();
               String key = Integer.toString(random.nextInt());
               String value = "{" +
                       "\"AlertDest\":\""+ result.getValue(3) + "\", " +
                       "\"AlertSource\":\"" + clientId + "\"" +
                       "}";
               logger.debug(value);
               producer.send(new ProducerRecord<String, String>("kafka-to-mqtt-alerts", key, value));
            }
            logger.info("Alerts sent to clients who has been in contact with " + clientId);
        }
    }
}
