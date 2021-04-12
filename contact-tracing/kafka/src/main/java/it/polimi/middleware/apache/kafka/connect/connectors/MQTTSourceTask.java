package it.polimi.middleware.apache.kafka.connect.connectors;

import org.apache.kafka.connect.data.Schema;
import org.apache.kafka.connect.source.SourceRecord;
import org.apache.kafka.connect.source.SourceTask;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class MQTTSourceTask extends SourceTask {
    private String mqttTopic;
    private String topic;
    private String broker;
    private int offset = 0;
    String clientId = "MQTTToKafkaNode";
    MqttClient client;
    MemoryPersistence persistence = new MemoryPersistence();
    ArrayList<String> incomingMessages = new ArrayList<>();

    @Override
    public String version() {
        return null;
    }

    public void start(Map<String, String> props) {
        mqttTopic = props.get(MQTTSourceConnector.MQTT_TOPIC_CONFIG);
        topic = props.get(MQTTSourceConnector.TOPIC_CONFIG);
        broker = "tcp://" + props.get(MQTTSourceConnector.BROKER_IP_CONFIG);
        try {
            client = new MqttClient(broker, clientId, persistence);
            MqttConnectOptions connectOptions = new MqttConnectOptions();
            connectOptions.setAutomaticReconnect(true);
            connectOptions.setCleanSession(true);
            connectOptions.setConnectionTimeout(10);
            System.out.println("connected");
            client.setCallback(new MqttCallback() {
                @Override
                public void connectionLost(Throwable throwable) {
                    System.err.println("Connection lost");
                }

                @Override
                public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
                    incomingMessages.add(mqttMessage.toString());
                }

                @Override
                public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
                    System.out.println("Delivered Message");
                }
            });
            System.out.println("dopo il setting");
            client.connect(connectOptions);
        } catch (MqttException me) {
            me.printStackTrace();
        }

    }


    @Override
    public List<SourceRecord> poll() throws InterruptedException {
        ArrayList<SourceRecord> records = new ArrayList<>();
        try {
            client.subscribe(mqttTopic, 1);
            for (String message : incomingMessages) {
                if (offset != 0) {
                    offset++;
                }
                records.add(new SourceRecord(Collections.singletonMap("source", mqttTopic), Collections.singletonMap("offset", Integer.toString(offset)), topic, Schema.STRING_SCHEMA, message));
            }
        } catch (MqttException e) {
            e.printStackTrace();
        }
        return records;
    }

    @Override
    public synchronized void stop() {
        try {
            client.disconnect();
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }
}
