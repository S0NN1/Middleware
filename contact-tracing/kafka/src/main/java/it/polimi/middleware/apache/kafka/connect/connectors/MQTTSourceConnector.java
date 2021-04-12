package it.polimi.middleware.apache.kafka.connect.connectors;

import org.apache.kafka.common.config.ConfigDef;
import org.apache.kafka.connect.connector.Task;
import org.apache.kafka.connect.source.SourceConnector;

import java.util.*;

public class MQTTSourceConnector extends SourceConnector{
    public static  String MQTT_TOPIC_CONFIG = "mqtt-topic";
    public static String TOPIC_CONFIG = "topic";
    public static String BROKER_IP_CONFIG = "mqtt-broker-address";
    private String mqttTopic;
    private String topic;
    private String broker;
    private static final ConfigDef CONFIG_DEF = new ConfigDef()
            .define(MQTT_TOPIC_CONFIG, ConfigDef.Type.STRING, ConfigDef.Importance.HIGH, "The MQTT topic to subscribe on")
            .define(TOPIC_CONFIG, ConfigDef.Type.STRING, ConfigDef.Importance.HIGH, "The topic to publish data to")
            .define(BROKER_IP_CONFIG, ConfigDef.Type.STRING, ConfigDef.Importance.HIGH, "The MQTT broker IP address");

    @Override
    public void start(Map<String, String> props) {
        System.out.println("Starting up MQTT connector");
        mqttTopic = props.get(MQTT_TOPIC_CONFIG);
        topic = props.get(TOPIC_CONFIG);
        broker = props.get(BROKER_IP_CONFIG);
    }

    @Override
    public Class<? extends Task> taskClass() {
        return MQTTSourceTask.class;
    }

    @Override
    public List<Map<String, String>> taskConfigs(int maxTasks) {
        ArrayList<Map<String, String>> configs = new ArrayList<>();
        // Only one input partition makes sense.
        Map<String, String> config = new HashMap<>();
        config.put(MQTT_TOPIC_CONFIG, mqttTopic);
        config.put(TOPIC_CONFIG, topic);
        config.put(BROKER_IP_CONFIG, broker);
        configs.add(config);
        return configs;
    }

    @Override
    public void stop() {

    }

    @Override
    public ConfigDef config() {
        return CONFIG_DEF;
    }

    @Override
    public String version() {
        return "1.0";
    }
}