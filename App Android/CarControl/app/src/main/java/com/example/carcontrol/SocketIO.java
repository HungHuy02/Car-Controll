package com.example.carcontrol;

import org.json.JSONException;
import org.json.JSONObject;

import java.net.URISyntaxException;

import io.socket.client.IO;
import io.socket.client.Socket;
import io.socket.emitter.Emitter;

public class SocketIO {

    private Socket mSocket;

    public void connect() {
        try {
            mSocket = IO.socket("https://bugnef-be-xedieukhien.onrender.com/car-active");
        } catch (URISyntaxException e) {
            e.printStackTrace();
        }
        mSocket.connect();
    }

    public void emit(String event, String active) {
        try {
            mSocket.emit(event, new JSONObject().put("active", active));
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    public void on(String event, Emitter.Listener func) {
        mSocket.on(event, func);
    }

    public void off (String event, Emitter.Listener func) {
        mSocket.off(event, func);
    }

    public void disconnect() {
        mSocket.disconnect();
    }
}
