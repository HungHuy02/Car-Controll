package com.example.carcontrol;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.google.gson.Gson;

import org.jetbrains.annotations.NotNull;
import org.json.JSONObject;

import java.io.IOException;
import java.net.URL;
import java.util.concurrent.TimeUnit;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.Headers;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okio.ByteString;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, View.OnTouchListener {

    private TextView tv;

    private TextView textViewStatus;
    private Button ButtonUp;
    private Button ButtonStop;
    private Button ButtonBack;
    private Button ButtonLeft;
    private Button ButtonRight;

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

    private boolean putData = false;

    private final String url = "https://bugnef-be-xedieukhien.onrender.com/cars/update";

    private static final int REQUEST_ENABLE_BT = 1;

    private OkHttpClient client;

    private Handler handler;
    private boolean isButtonHeld = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        intent = new Intent(this, MyBluetooth.class);
        myData = (MyData) getApplication();
        client = new OkHttpClient();
        tv = (TextView) findViewById(R.id.tv);
        textViewStatus = (TextView) findViewById(R.id.tv_status);
        ButtonUp = (Button) findViewById(R.id.btn_up);
        ButtonUp.setOnTouchListener(this);
        ButtonStop = (Button) findViewById(R.id.btn_stop);
        ButtonStop.setOnClickListener(this);
        ButtonBack = (Button) findViewById(R.id.btn_down);
        ButtonBack.setOnTouchListener(this);
        ButtonLeft = (Button) findViewById(R.id.btn_left);
        ButtonLeft.setOnTouchListener(this);
        ButtonRight = (Button) findViewById(R.id.btn_right);
        ButtonRight.setOnTouchListener(this);
        ButtonAutoO = (Button) findViewById(R.id.btn_ab);
        ButtonAutoO.setOnClickListener(this);
        ButtonAutoL = (Button) findViewById(R.id.btn_al);
        ButtonAutoL.setOnClickListener(this);
        ButtonAutoF = (Button) findViewById(R.id.btn_af);
        ButtonAutoF.setOnClickListener(this);
        ButtonBluetooth = (Button) findViewById(R.id.btn_bluetooth);
        ButtonBluetooth.setOnClickListener(this);
        ButtonWifi = (Button) findViewById(R.id.btn_wifi);
        ButtonWifi.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        CarStatus carStatus = new CarStatus();
        if (v.equals(ButtonStop)) {
            carStatus.active = "S";
            setText("Stop");
        } else if (v.equals(ButtonWifi)) {
            wifiOn = true;
            bluetoothOn = false;
            textViewStatus.setText("Wifi");
        } else if (v.equals(ButtonBluetooth)) {
            bluetoothOn = true;
            wifiOn = false;
            textViewStatus.setText("Bluetooth");
            startActivity(intent);
        } else if (v.equals(ButtonAutoF)) {
            carStatus.active = "T";
            setText("Auto Follow");
        } else if (v.equals(ButtonAutoL)) {
            carStatus.active = "Y";
            setText("Auto Line");
            ;
        } else if (v.equals(ButtonAutoO)) {
            carStatus.active = "Z";
            setText("Auto Obstacle");
        }
        if (carStatus.active != null) {
            putData(carStatus);
        }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                isButtonHeld = true;
                if (wifiOn) {
                    controlHandle(v);
                } else if (bluetoothOn) {
                    handler = new Handler();
                    handler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if (isButtonHeld) {
                                controlHandle(v);
                                handler.postDelayed(this, 10);
                            }
                        }
                    }, 10);
                }
                break;
            case MotionEvent.ACTION_UP:
                isButtonHeld = false;
                if (bluetoothOn) {
                    CarStatus carStatus = new CarStatus();
                    carStatus.active = "S";
                    putData(carStatus);
                }
                break;
        }
        return false;
    }

    public void putData(CarStatus carStatus) {
        if (wifiOn) {
            String json = new Gson().toJson(carStatus);
            RequestBody requestBody = RequestBody.create(json,
                    MediaType.get("application/json; charset=utf-8"));
            Request request = new Request.Builder()
                    .url(url)
                    .put(requestBody)
                    .build();
            client.newCall(request).enqueue(new Callback() {
                @Override
                public void onFailure(@NotNull Call call,
                                      @NotNull IOException e) {
                    e.printStackTrace();
                }
                @Override
                public void onResponse(@NotNull Call call,
                                       @NotNull Response response)
                        throws IOException {
                    try (ResponseBody responseBody = response.body()) {
                        if (!response.isSuccessful())
                            throw new IOException("Unexpected code "
                                    + response);

                        Headers responseHeaders = response.headers();
                        for (int i = 0, size = responseHeaders.size();
                             i < size; i++) {
                            System.out.println(responseHeaders.name(i) +
                                    ": " + responseHeaders.value(i));
                        }

                        System.out.println(responseBody.string());
                    }
                }
            });
        } else if (bluetoothOn) {
            thread.write(carStatus.active);
        }
    }

    public void setText(String text) {
        tv.setText("Active: " + text);
    }


    public class CarStatus {
        private String active;
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (myData.getThread() != null) {
            thread = myData.getThread();
        }
    }

    public void controlHandle(View v) {
        CarStatus carStatus = new CarStatus();
        if (v.equals(ButtonUp)) {
            carStatus.active = "F";
            setText("Forward");
        } else if (v.equals(ButtonBack)) {
            carStatus.active = "B";
            setText("Back");
        } else if (v.equals(ButtonLeft)) {
            carStatus.active = "L";
            setText("Left");
        } else if (v.equals(ButtonRight)) {
            carStatus.active = "R";
            setText("Rights");
        }
        putData(carStatus);
    }
}