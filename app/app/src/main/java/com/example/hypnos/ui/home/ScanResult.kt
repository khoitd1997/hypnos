package com.example.hypnos.ui.home

import android.bluetooth.BluetoothDevice

data class ScanResult(val bleDevice: BluetoothDevice, val rssi: Int)