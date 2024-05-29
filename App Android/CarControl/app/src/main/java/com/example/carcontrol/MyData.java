package com.example.carcontrol;

import android.app.Application;

public class MyData extends Application {

    private ConnectedThread thread;

    public ConnectedThread getThread() {
        return thread;
    }

    public void setThread(ConnectedThread thread) {
        this.thread = thread;
    }
}
