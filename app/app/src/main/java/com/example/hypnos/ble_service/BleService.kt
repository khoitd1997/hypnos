package com.example.hypnos.ble_service

import android.app.*
import android.bluetooth.*
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanSettings.*
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.*
import android.util.Log
import android.widget.Toast
import com.example.hypnos.ui.home.ScanResult
import com.example.hypnos.ui.home.ScanResultsAdapter
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import java.nio.Buffer
import java.nio.ByteBuffer
import java.nio.ByteOrder

import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

import java.util.*


class BleService : Service() {
    companion object {
        const val NOTIFICATION_ID = 1
        const val SHARED_PREFERENCE_FILE_NAME = "ble_shared_prefs"

        const val ACTION_START_FOREGROUND_SERVICE = "ACTION_START_FOREGROUND_SERVICE"
        const val ACTION_STOP_FOREGROUND_SERVICE = "ACTION_STOP_FOREGROUND_SERVICE"

        enum class BleIpcCmd {
            INIT, DEINIT,
            START_SCAN, STOP_SCAN,
            CONNECT_DEVICE, DISCONNECT_DEVICE,
            TEST
        }

        data class InitInfo(val scanAdapter: ScanResultsAdapter)
        data class ConnectInfo(val device: BluetoothDevice)
        data class DisconnectInfo(val unbond: Boolean)

        private val BATTERY_LEVEL_CHARACTERISTIC_UUID =
            UUID.fromString("00002a19-0000-1000-8000-00805f9b34fb")

        private fun getCharacteristicUuidFromServiceUuid(
            service_uuid: String,
            characteristicShortUUID: String
        ): UUID = UUID.fromString(service_uuid.replaceRange(4, 8, characteristicShortUUID))

        private val TIMETABLE_SERVICE_UUID = "f3641400-b000-4042-ba50-05ca45bf8abc"
        private val MORNING_CURFEW_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID, "1401")
        private val ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID, "1405")

        const val BLE_SERVICE_LOG_TAG = "ble"
    }

    private lateinit var sharedPreferences: SharedPreferences

    // using Long because Kotlin only has experimental support for unsigned
    data class TimeException(val startTimeEpoch: Long, val endTimeEpoch: Long) {}
    class TimeExceptionList() {
        private val timeExceptions = mutableListOf<TimeException>()
        private val timeExceptionsType = object : TypeToken<List<TimeException>>() {}.type

        fun replace(rawValues: ByteArray) {
            val buf =
                ByteBuffer.wrap(rawValues).order(ByteOrder.LITTLE_ENDIAN).asIntBuffer()
            val temp = IntArray(buf.remaining())
            buf.get(temp)
            assert(temp.size % 2 == 0)

            timeExceptions.clear()

            val iterator = temp.iterator()
            while (iterator.hasNext()) {
                val start = iterator.next().toLong()
                val stop = iterator.next().toLong()
                timeExceptions.add(TimeException(start, stop))
            }
        }

        fun replace(jsonString: String) {
            timeExceptions.clear()
            timeExceptions.addAll(
                Gson().fromJson<List<TimeException>>(
                    jsonString,
                    timeExceptionsType
                )
            )
        }

        fun toJson(): String {
            return Gson().toJson(timeExceptions)
        }

        fun get(): List<TimeException> {
            return timeExceptions
        }

        fun getDates(): List<Pair<Date, Date>> {
            val ret = mutableListOf<Pair<Date, Date>>()
            for (e in timeExceptions) {
                ret.add(
                    Pair<Date, Date>(
                        Date(e.startTimeEpoch * 1000),
                        Date(e.endTimeEpoch * 1000)
                    )
                )
            }

            return ret
        }
    }

    private val timeExceptionList = TimeExceptionList()

    private lateinit var ipcMessenger: Messenger
    internal var scanResultsAdapter: ScanResultsAdapter? = null

    private val bluetoothAdapter: BluetoothAdapter by lazy(LazyThreadSafetyMode.NONE) {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter
    }

    private val bluetoothScanner: BluetoothLeScanner? by lazy(LazyThreadSafetyMode.NONE) {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter.bluetoothLeScanner
    }

    private var bleDevice: BluetoothDevice? = null
    private var bleGatt: BluetoothGatt? = null

    private val BluetoothAdapter.isDisabled: Boolean
        get() = !isEnabled

    private lateinit var notificationHelper: NotificationHelper


    override fun onCreate() {
        super.onCreate()

        this.sharedPreferences = applicationContext.getSharedPreferences(
            SHARED_PREFERENCE_FILE_NAME,
            Context.MODE_PRIVATE
        )

        this.notificationHelper = NotificationHelper(sharedPreferences, this)

//        bluetoothAdapter?.takeIf { it.isDisabled }?.apply {
//            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
//            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
//        }

        startForeground(
            NotificationHelper.PERSISTENT_NOTIFICATION_ID,
            notificationHelper.buildDisconnectedNotification()
        )
        Log.v("event", "onCreate of ble service")
    }

    override fun onStartCommand(intent: Intent, flags: Int, startId: Int): Int {
        Log.d("event", "ble service onStartCommand")

        return START_REDELIVER_INTENT
    }

    private fun getSyncTime(): String {
        val current = LocalDateTime.now()
        val formatter = DateTimeFormatter.ofPattern("HH:mm")
        return current.format(formatter)
    }

    private fun updateDeviceData(
        isBreakPhase: Boolean,
        minuteLeftInPhase: Int,
        batteryPercent: Int,
        syncTime: String
    ) {
        with(sharedPreferences.edit()) {
            putBoolean("isBreakPhase", isBreakPhase)
            putInt("timeLeft", minuteLeftInPhase)
            putInt("battery", batteryPercent)
            putString("syncTime", syncTime)

            commit()
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        Log.d(BLE_SERVICE_LOG_TAG, "service being destroyed")

        disconnectDevice(false)

        val broadcastIntent = Intent()
        broadcastIntent.action = "restartservice"
        broadcastIntent.setClass(this, BleServiceStarter::class.java)
        this.sendBroadcast(broadcastIntent)

        stopForeground(true)
        stopSelf()
    }

    internal class BleScannerCallback(context: Context) : ScanCallback() {
        private var parentContext = context as BleService
        override fun onScanResult(callbackType: Int, result: android.bluetooth.le.ScanResult?) {
            super.onScanResult(callbackType, result)

            with(parentContext) {
                result?.let {
                    val device = result.device
                    val rssi = result.rssi
                    Log.d(BLE_SERVICE_LOG_TAG, "found device: $device")
                    device?.let {
                        scanResultsAdapter?.addScanResult(ScanResult(it, rssi))
                    }
                }
            }
        }
    }

    private val bleScannerCallback = BleScannerCallback(this)

    internal class BleGATTCallback(context: Context) : BluetoothGattCallback() {
        private var parentContext = context as BleService

        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            super.onConnectionStateChange(gatt, status, newState)
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    Log.i(BLE_SERVICE_LOG_TAG, "Connected to GATT server.")
                    Log.i(
                        BLE_SERVICE_LOG_TAG, "Attempting to start service discovery: " +
                                gatt?.discoverServices()
                    )
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    Log.i(BLE_SERVICE_LOG_TAG, "Disconnected from GATT server.")
                }
                else -> {
                    Log.i(BLE_SERVICE_LOG_TAG, "gatt state: $newState")
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Log.d(BLE_SERVICE_LOG_TAG, "service discovery succeeded")
                    Log.d(BLE_SERVICE_LOG_TAG, "$MORNING_CURFEW_CHARACTERISTIC_UUID")

                    for (service in gatt.services) {
                        for (characteristic in service.characteristics) {
                            Log.d(BLE_SERVICE_LOG_TAG, "UUID: ${characteristic.uuid}")
                            if (characteristic.uuid == BATTERY_LEVEL_CHARACTERISTIC_UUID) {
                                Log.d(
                                    BLE_SERVICE_LOG_TAG,
                                    "found battery characteristic: ${characteristic.properties} ${characteristic.permissions}"
                                )
//                                gatt.readCharacteristic(characteristic)
//                                break;
                            } else if (characteristic.uuid == MORNING_CURFEW_CHARACTERISTIC_UUID) {
                                Log.d(BLE_SERVICE_LOG_TAG, "found morning curfew")
                            } else if (characteristic.uuid == ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID) {
                                Log.d(BLE_SERVICE_LOG_TAG, "found active exceptions")
                                gatt.readCharacteristic(characteristic)
                                break;
                            }
                        }
                    }
//                    gatt.readCharacteristic(
//                        BluetoothGattCharacteristic(
//                            BATTERY_LEVEL_UUID,
//                            BluetoothGattCharacteristic.PROPERTY_READ,
//                            BluetoothGattCharacteristic.PERMISSION_READ
//                        )
//                    )
                }
                else -> Log.w(BLE_SERVICE_LOG_TAG, "onServicesDiscovered received: $status")
            }
        }

        override fun onCharacteristicRead(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?,
            status: Int
        ) {
            super.onCharacteristicRead(gatt, characteristic, status)
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    characteristic?.value?.let { values ->
                        with(parentContext) {
                            timeExceptionList.replace(values)

                            Log.d(
                                BLE_SERVICE_LOG_TAG,
                                "gatt success: ${timeExceptionList.getDates()}"
                            )
                        }
                    }
                }
                else -> {
                    Log.d(BLE_SERVICE_LOG_TAG, "gatt read failed: $status")
                }
            }
        }
    }

    private val bleGATTCallback = BleGATTCallback(this)


    internal class IpcCommandHandler(
        context: Context,
        private val applicationContext: Context = context.applicationContext
    ) : Handler() {
        private var parentContext = context as BleService
        override fun handleMessage(msg: Message) {
            val receivedObj = msg.obj
            with(parentContext) {
                when (BleIpcCmd.values()[msg.what]) {
                    BleIpcCmd.INIT -> {
                        scanResultsAdapter = (receivedObj as InitInfo).scanAdapter
                    }

                    BleIpcCmd.DEINIT -> {
                        scanResultsAdapter = null
                    }

                    BleIpcCmd.START_SCAN -> {
                        Log.d(
                            BLE_SERVICE_LOG_TAG,
                            "extended advertising: ${bluetoothAdapter.isLeExtendedAdvertisingSupported}"
                        )
                        startScan()
                    }

                    BleIpcCmd.STOP_SCAN -> {
                        stopScan()
                    }

                    BleIpcCmd.CONNECT_DEVICE -> {
                        scanResultsAdapter?.clearScanResults()
                        stopScan()
                        val connectInfo = receivedObj as ConnectInfo
                        Log.d(BLE_SERVICE_LOG_TAG, "received connect device cmd")
                        connectDevice(connectInfo.device)
                    }

                    BleIpcCmd.DISCONNECT_DEVICE -> {
                        val disconnectInfo = receivedObj as DisconnectInfo
                        disconnectDevice(disconnectInfo.unbond)
                    }

                    BleIpcCmd.TEST -> {
                        scanResultsAdapter?.clearScanResults()
                    }
                }
            }
        }
    }

    private fun stopScan(): Unit? {
        return bluetoothScanner?.stopScan(
            bleScannerCallback
        )
    }

    private fun startScan(): Unit? {
        return bluetoothScanner?.startScan(
            scanFilter,
            scanSettings,
            bleScannerCallback
        )
    }

    override fun onBind(intent: Intent): IBinder? {
        Toast.makeText(applicationContext, "binding", Toast.LENGTH_SHORT).show()
        ipcMessenger = Messenger(
            IpcCommandHandler(
                this
            )
        )
        return ipcMessenger.binder
    }

    private fun connectDevice(device: BluetoothDevice? = this.bleDevice) {
//        Log.d(BLE_SERVICE_LOG_TAG, "trying to connect device $macAddr")
        device?.connectGatt(this, false, bleGATTCallback)

    }

    private fun unbondDevice() {
        // TODO(khoi): Talk to BMS here
        bleDevice?.let { dev ->
            try {
                dev::class.java.getMethod("removeBond")
                    .invoke(dev)
            } catch (e: Exception) {
                Log.e("event", "Removing bond has been failed. ${e.message}")
            }
            Log.v("event", "bonded devices cnt after unbond: ${getBondedMacAddr().size}")
        }
    }

    private fun disconnectDevice(unbond: Boolean) {
        Log.d(BLE_SERVICE_LOG_TAG, "disconnect func called")
        if (unbond) {
            unbondDevice()
        }

        bleGatt?.disconnect()
    }

    private fun getBondedMacAddr(): List<String> {
        return listOf()
        // TODO(khoi): Implement this
//        return rxBleClient.bondedDevices.filter {
//            it?.name?.contains("Hypnos") ?: false
//        }.map { it.macAddress!! }
    }

    private val scanSettings: android.bluetooth.le.ScanSettings by lazy {
        android.bluetooth.le.ScanSettings.Builder()
            .setCallbackType(CALLBACK_TYPE_ALL_MATCHES).setMatchMode(
                MATCH_MODE_AGGRESSIVE
            ).setPhy(PHY_LE_ALL_SUPPORTED).setScanMode(SCAN_MODE_LOW_LATENCY)
            .setLegacy(false)
            .build()
    }

    private val scanFilter: List<android.bluetooth.le.ScanFilter> by lazy {
        listOf<android.bluetooth.le.ScanFilter>()
    }
}