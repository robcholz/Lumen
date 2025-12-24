package com.robcholz.lumen.client;

import com.fazecast.jSerialComm.SerialPort;
import com.robcholz.lumen.client.config.LumenConfig;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public final class SerialPortLocator {
    private SerialPortLocator() {
    }

    public static String resolvePortPath(LumenConfig config) {
        String configured = normalize(config.portPath);
        if (!configured.isEmpty()) {
            return configured;
        }
        String property = normalize(System.getProperty("lumen.serialPort"));
        if (!property.isEmpty()) {
            return property;
        }
        String env = normalize(System.getenv("LUMEN_SERIAL_PORT"));
        if (!env.isEmpty()) {
            return env;
        }
        return autoDetectPort();
    }

    public static String autoDetectPort() {
        SerialPort[] ports = SerialPort.getCommPorts();
        if (ports.length == 0) {
            return null;
        }
        String best = null;
        for (SerialPort port : ports) {
            String name = normalize(port.getSystemPortName());
            String lower = name.toLowerCase();
            if (lower.contains("usb") || lower.contains("modem") || lower.contains("tty")) {
                best = port.getSystemPortName();
                break;
            }
        }
        if (best != null) {
            return best;
        }
        return ports[0].getSystemPortName();
    }

    public static List<String> listPorts() {
        SerialPort[] ports = SerialPort.getCommPorts();
        List<String> names = new ArrayList<>(ports.length);
        for (SerialPort port : ports) {
            String name = normalize(port.getSystemPortName());
            if (!name.isEmpty()) {
                names.add(name);
            }
        }
        Collections.sort(names);
        return names;
    }

    private static String normalize(String value) {
        if (value == null) {
            return "";
        }
        return value.trim();
    }
}
