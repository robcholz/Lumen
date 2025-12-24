package com.robcholz.lumen;

import com.fazecast.jSerialComm.SerialPort;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class SerialPackClient implements AutoCloseable {
    private final SerialPort port;
    private final String portPath;

    public SerialPackClient(String portPath) throws IOException {
        this.portPath = portPath;
        port = SerialPort.getCommPort(portPath);
        if (port == null) {
            throw new IOException("Serial port not found: " + portPath);
        }
        port.setBaudRate(460800);
        port.setComPortTimeouts(SerialPort.TIMEOUT_WRITE_BLOCKING, 0, 0);
        if (!port.openPort()) {
            throw new IOException("Failed to open serial port: " + portPath);
        }
    }

    private static byte[] encodeU32(int value) {
        return new byte[]{
                (byte) (value & 0xFF),
                (byte) ((value >>> 8) & 0xFF),
                (byte) ((value >>> 16) & 0xFF),
                (byte) ((value >>> 24) & 0xFF)
        };
    }

    public synchronized void send(String path, byte[] data) throws IOException {
        if (!port.isOpen()) {
            throw new IOException("Serial port is closed");
        }
        byte[] pathBytes = path.getBytes(StandardCharsets.UTF_8);
        writeAll(pathBytes);
        writeAll(new byte[]{'\n'});
        writeAll(encodeU32(data.length));
        writeAll(data);
    }

    public boolean isOpen() {
        return port.isOpen();
    }

    public String getPortPath() {
        return portPath;
    }

    private void writeAll(byte[] bytes) throws IOException {
        int offset = 0;
        while (offset < bytes.length) {
            int written = port.writeBytes(bytes, bytes.length - offset, offset);
            if (written <= 0) {
                throw new IOException("Serial write failed");
            }
            offset += written;
        }
    }

    @Override
    public void close() {
        if (port.isOpen()) {
            port.closePort();
        }
    }
}
