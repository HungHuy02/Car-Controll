package com.example.carcontrol;


import android.Manifest;
import android.animation.ObjectAnimator;
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
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.Toast;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;


public class BluetoothActivity extends AppCompatActivity {

    private static final int REQUEST_ENABLE_BT = 1;
    private BluetoothAdapter bluetoothAdapter;
    private ArrayAdapter<String> pairedArrayAdapter;
    private ArrayAdapter<String> discoveredArrayAdapter;
    private ArrayList<BluetoothDevice> pairedDevicesList;
    private ArrayList<BluetoothDevice> discoveredDevicesList;
    private BluetoothSocket bluetoothSocket = null;

    private MyData myData;

    private ListView pairedList;
    private ListView discoveredList;

    private ConnectedThread thread;

    private ImageButton refreshBtn;
    private Boolean isDiscovery = true;

    private ObjectAnimator objectAnimator;
    private final UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");


    @RequiresApi(api = Build.VERSION_CODES.S)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bluetooth);

        refreshBtn = findViewById(R.id.refresh_btn);



        pairedList = findViewById(R.id.pairedList);
        discoveredList = findViewById(R.id.discorveryList);

        pairedArrayAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_list_item_1);
        pairedList.setAdapter(pairedArrayAdapter);

        discoveredArrayAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_list_item_1);
        discoveredList.setAdapter(discoveredArrayAdapter);

        pairedDevicesList = new ArrayList<>();
        discoveredDevicesList = new ArrayList<>();

        pairedList.setOnItemClickListener(
                (parent, view, position, id) -> connectToDevice(pairedDevicesList.get(position)));

        discoveredList.setOnItemClickListener(
                (parent, view, position, id) -> connectToDevice(discoveredDevicesList.get(position)));

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            showToast("Bluetooth is not supported on this device.");
            finish();
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        BluetoothActivity.this,
                        new String[]{Manifest.permission.BLUETOOTH_SCAN},
                        1);
            }
        }

        refreshBtn.setOnClickListener(v -> {
            if (isDiscovery) {
                bluetoothAdapter.cancelDiscovery();
                objectAnimator.cancel();
            } else {
                discoveredArrayAdapter.clear();
                discoveredDevicesList.clear();
                bluetoothAdapter.startDiscovery();
                objectAnimator.start();
            }
            isDiscovery = !isDiscovery;
        });

        objectAnimator = ObjectAnimator.ofFloat(refreshBtn, "rotation", 0f, 360f);
        objectAnimator.setDuration(12000);
        objectAnimator.setRepeatCount(ObjectAnimator.INFINITE);

        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        registerReceiver(receiver, filter);

        if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }else {
            fetchDevice();
            objectAnimator.start();
        }
    }

    public void fetchDevice() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        BluetoothActivity.this,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                        1);
            }
        }
        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();

        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                pairedArrayAdapter.add(device.getName()
                        + "\n" + device.getAddress());
                pairedDevicesList.add(device);
            }
        }
        pairedArrayAdapter.notifyDataSetChanged();


        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        BluetoothActivity.this,
                        new String[]{Manifest.permission.BLUETOOTH_SCAN},
                        1);
            }
        }
        bluetoothAdapter.startDiscovery();
    }

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                BluetoothDevice device;
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
                    device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE, BluetoothDevice.class);
                }else {
                    device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                }

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                            Manifest.permission.BLUETOOTH_CONNECT)
                            != PackageManager.PERMISSION_GRANTED) {
                        ActivityCompat.requestPermissions(
                                BluetoothActivity.this,
                                new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                                1);
                    }
                }

                if(!discoveredDevicesList.contains(device)) {
                    discoveredDevicesList.add(device);
                    discoveredArrayAdapter.add(device.getName()
                            + "\n" + device.getAddress());
                    discoveredArrayAdapter.notifyDataSetChanged();
                }
            }
        }
    };

    private void connectToDevice(BluetoothDevice device) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(getApplicationContext(),
                    Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                        BluetoothActivity.this,
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
                e.printStackTrace();
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

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(receiver);
    }
}
