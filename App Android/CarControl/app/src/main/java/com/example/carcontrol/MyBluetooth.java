package com.example.carcontrol;


import android.Manifest;
import android.app.Application;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;


public class MyBluetooth extends AppCompatActivity {

    private static final int REQUEST_ENABLE_BT = 1;
    private BluetoothAdapter bluetoothAdapter;
    private ArrayAdapter<String> discoveredDevicesArrayAdapter;
    private ArrayList<BluetoothDevice> discoveredDevicesList;
    private BluetoothSocket bluetoothSocket = null;

    private MyData myData;

    private ListView discoveredDevicesListView;

    private ConnectedThread thread;

    private final UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // Standard SPP (Serial Port Profile) UUID


    @RequiresApi(api = Build.VERSION_CODES.S)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.bluetooth_layout);

        discoveredDevicesListView = (ListView)
                findViewById(R.id.discoveredDevicesListView);

        discoveredDevicesArrayAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_list_item_1);
        discoveredDevicesListView.setAdapter(discoveredDevicesArrayAdapter);

        discoveredDevicesList = new ArrayList<>();

        discoveredDevicesListView.setOnItemClickListener(
                new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent,
                                    View view, int position, long id) {
                connectToDevice(discoveredDevicesList.get(position));
            }
        });
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        MyBluetooth.this,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                        1);
            }
        }
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            showToast("Bluetooth is not supported on this device.");
            finish();
        }

        if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(
                    BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }else {
            fetchDevice();
        }
    }

    public void fetchDevice() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        MyBluetooth.this,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                        1);
            }
        }
        Set<BluetoothDevice> pairedDevices =
                bluetoothAdapter.getBondedDevices();

        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                discoveredDevicesArrayAdapter.add(device.getName()
                        + "\n" + device.getAddress());
                discoveredDevicesList.add(device);
            }
        }
        discoveredDevicesArrayAdapter.notifyDataSetChanged();
    }

    private void connectToDevice(BluetoothDevice device) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        MyBluetooth.this,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                        1);
            }
        }
        try {
            bluetoothSocket = device.createRfcommSocketToServiceRecord(uuid);
            bluetoothSocket.connect();
            showToast("Connected to " + device.getName());
            thread = new ConnectedThread(bluetoothSocket);
            myData = (MyData) getApplication();
            myData.setThread(thread);
            finish();
        } catch (IOException e) {
            showToast("Connection failed: " + e.getMessage());
            try {
                bluetoothSocket.close();
            } catch (IOException closeException) {
                // Ignore
            }
        }
    }

    private void showToast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == REQUEST_ENABLE_BT) {
            if(resultCode == RESULT_OK){
                showToast("Bluetooth is enabled.");
                fetchDevice();
        } else {
            showToast("Bluetooth enabling is canceled.");
            finish();
        }
        }
    }
}
