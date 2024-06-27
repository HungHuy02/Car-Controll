package com.example.carcontrol;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import io.github.controlwear.virtual.joystick.android.JoystickView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, JoystickView.OnMoveListener {

    private TextView tv;
    private TextView textViewStatus;
    private JoystickView joystickView;
    private Button ButtonWifi;
    private Button ButtonBluetooth;
    private Button ButtonAutoF;
    private Button ButtonAutoO;
    private Button ButtonAutoL;
    private MyData myData;
    private Intent intent;
    private ConnectedThread thread;
    private boolean wifiOn = false;
    private boolean bluetoothOn = false;
    private SocketIO socketIO;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        intent = new Intent(this, BluetoothActivity.class);
        myData = (MyData) getApplication();
        socketIO = new SocketIO();
        tv = findViewById(R.id.tv);
        textViewStatus = findViewById(R.id.tv_status);
        joystickView  = findViewById(R.id.jv);
        joystickView.setOnMoveListener(this);
        ButtonAutoO = findViewById(R.id.btn_ab);
        ButtonAutoO.setOnClickListener(this);
        ButtonAutoL = findViewById(R.id.btn_al);
        ButtonAutoL.setOnClickListener(this);
        ButtonAutoF = findViewById(R.id.btn_af);
        ButtonAutoF.setOnClickListener(this);
        ButtonBluetooth = findViewById(R.id.btn_bluetooth);
        ButtonBluetooth.setOnClickListener(this);
        ButtonWifi = findViewById(R.id.btn_wifi);
        ButtonWifi.setOnClickListener(this);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        MainActivity.this,
                        new String[]{Manifest.permission.BLUETOOTH_SCAN},
                        1);
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        MainActivity.this,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                        1);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (myData.getThread() != null) {
            thread = myData.getThread();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        socketIO.disconnect();
    }

    @Override
    public void onClick(View v) {
        CarStatus carStatus = new CarStatus();
        if (v.equals(ButtonWifi)) {
            if(!wifiOn) {
                wifiOn = true;
                socketIO.connect();
                bluetoothOn = false;
                textViewStatus.setText("Wifi");
            } else {
                textViewStatus.setText("Not Connected");
                carStatus.active = "Disconnect";
            }
        } else if (v.equals(ButtonBluetooth)) {
            if(!bluetoothOn) {
                bluetoothOn = true;
                wifiOn = false;
                textViewStatus.setText("Bluetooth");
                startActivity(intent);
            } else {
                textViewStatus.setText("Not connected");
                carStatus.active = "D";
            }
        } else if (v.equals(ButtonAutoF)) {
            carStatus.active = "T";
            setText("Auto Follow");
        } else if (v.equals(ButtonAutoL)) {
            carStatus.active = "Y";
            setText("Auto Line");
        } else if (v.equals(ButtonAutoO)) {
            carStatus.active = "Z";
            setText("Auto Obstacle");
        }
        if (carStatus.active != null) {
            putData(carStatus);
        }
    }
    @Override
    public void onMove(int angle, int strength) {
        CarStatus carStatus = new CarStatus();
        if(strength > 10) {
            if (isDirection(angle, "UP")) {
                carStatus.active = "F";
                setText("Forward");
            } else if (isDirection(angle, "DOWN")) {
                carStatus.active = "B";
                setText("Back");
            } else if (isDirection(angle, "LEFT")) {
                carStatus.active = "L";
                setText("Left");
            } else if (isDirection(angle, "RIGHT")) {
                carStatus.active = "R";
                setText("Rights");
            }
        } else {
            carStatus.active = "S";
            setText("Stop");
        }
        putData(carStatus);
    }

    private boolean isDirection(int angle, String direction) {
        switch (direction) {
            case "UP":
                return angle >= 45 && angle <= 135;
            case "DOWN":
                return angle >= 225 && angle <= 315;
            case "LEFT":
                return angle > 135 && angle < 225;
            case "RIGHT":
                return (angle >= 0 && angle <= 45) || (angle > 315 && angle <= 360);
            default:
                return false;
        }
    }

    public void putData(CarStatus carStatus) {
        if (wifiOn) {
            socketIO.emit("update-active", carStatus.active);
            if(carStatus.active.equals("Disconnect")) {
                wifiOn = false;
                socketIO.disconnect();
            }
        } else if (bluetoothOn) {
            thread.write(carStatus.active);
            if(carStatus.active.equals("D")) {
                thread.cancel();
                bluetoothOn = false;
            }
        }
    }

    public void setText(String text) {
        tv.setText("Active: " + text);
    }

    public class CarStatus {
        private String active;
    }
}