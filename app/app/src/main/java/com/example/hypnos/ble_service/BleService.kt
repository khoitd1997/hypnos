package com.example.hypnos.ble_service

import android.app.Service
import android.bluetooth.*
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanSettings.*
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.Handler
import android.os.IBinder
import android.os.Message
import android.os.Messenger
import android.util.Log
import android.widget.Toast
import com.example.hypnos.ui.home.ScanResult
import com.example.hypnos.ui.home.ScanResultsAdapter
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import java.util.*
import java.util.concurrent.TimeUnit


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
            CONFIGURE_TIMETABLE,
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

        private val TIMETABLE_SERVICE_UUID = UUID.fromString("f3641400-b000-4042-ba50-05ca45bf8abc")
        private val MORNING_CURFEW_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID.toString(), "1401")
        private val NIGHT_CURFEW_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID.toString(), "1402")
        private val WORK_DURATION_MINUTE_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID.toString(), "1403")
        private val BREAK_DURATION_MINUTE_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID.toString(), "1404")
        private val ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID.toString(), "1405")
        private val TOKENS_LEFT_CHARACTERISTIC_UUID =
            getCharacteristicUuidFromServiceUuid(TIMETABLE_SERVICE_UUID.toString(), "1406")

        const val BLE_SERVICE_LOG_TAG = "ble"

        // use this https://developer.android.com/guide/topics/ui/controls/pickers for picker
        data class HourMinuteTime(val hourIn24HourFormat: Int, val minute: Int) {}

        data class TimeException(val start: Date, val end: Date) {}

        data class TimetableConfig(
            var morningCurfew: HourMinuteTime = HourMinuteTime(7, 30),
            var nightCurfew: HourMinuteTime = HourMinuteTime(9, 30),
            var workDurationMinute: Int = 120,
            var breakDurationMinute: Int = 10,
            var activeExceptions: List<TimeException> = emptyList(),
            var tokensLeft: Int = 100
        ) {}
    }

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

    private lateinit var sharedPreferences: SharedPreferences

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

//        val testConfig = TimetableConfig(
//            morningCurfew = HourMinuteTime(23, 59),
//            activeExceptions = listOf(
//                TimeException(
//                    GregorianCalendar(2000, 6, 16, 23, 59, 30).time,
//                    GregorianCalendar(2030, JANUARY, 29, 10, 15, 59).time
//                ),
//                TimeException(
//                    GregorianCalendar(1958, 1, 19, 2, 30, 4).time,
//                    GregorianCalendar(1967, 8, 12, 11, 50, 1).time
//                )
//            )
//        )
//        with(sharedPreferences.edit()) {
//            putString(TIME_TABLE_CONFIG_KEY, Gson().toJson(testConfig))
//
//            commit()
//        }

        bleGATTCallback = BleGATTCallback(this)

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

    private val TIME_TABLE_CONFIG_KEY = "timetableConfig"

    internal class BleGATTCallback(context: Context) : BluetoothGattCallback() {
        private var parentContext = context as BleService
        private lateinit var timetableConfig: TimetableConfig
        private val characteristicsToWrite = mutableListOf<BluetoothGattCharacteristic>()

        private fun convertToGMT(localTime: HourMinuteTime): HourMinuteTime {
            val localCalendar = Calendar.getInstance()
            localCalendar[Calendar.HOUR_OF_DAY] = localTime.hourIn24HourFormat
            localCalendar[Calendar.MINUTE] = localTime.minute
            localCalendar[Calendar.SECOND] = 0

            val gmtCalendar = GregorianCalendar(TimeZone.getTimeZone("GMT"))
            gmtCalendar.timeInMillis = localCalendar.timeInMillis

            return HourMinuteTime(
                gmtCalendar.get(Calendar.HOUR_OF_DAY),
                gmtCalendar.get(Calendar.MINUTE)
            )
        }

        private fun convertFromGMT(gmtTime: HourMinuteTime): HourMinuteTime {
            val gmtCalendar = GregorianCalendar(TimeZone.getTimeZone("GMT"))
            gmtCalendar[Calendar.HOUR_OF_DAY] = gmtTime.hourIn24HourFormat
            gmtCalendar[Calendar.MINUTE] = gmtTime.minute
            gmtCalendar[Calendar.SECOND] = 0

            val localCalendar = Calendar.getInstance()
            localCalendar.timeInMillis = gmtCalendar.timeInMillis

            return HourMinuteTime(
                localCalendar.get(Calendar.HOUR_OF_DAY),
                localCalendar.get(Calendar.MINUTE)
            )
        }

        private fun HourMinuteTime.encode(): ByteArray {
            with(convertToGMT(this)) {
                val buf = ByteBuffer.allocate(Short.SIZE_BYTES).order(ByteOrder.LITTLE_ENDIAN)
                buf.putShort(((hourIn24HourFormat shl 6) or minute).toShort())
                return buf.array()
            }
        }

        private fun decodeHourMinuteTime(bytes: ByteArray): HourMinuteTime {
            val temp = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asIntBuffer()
            return convertFromGMT(
                HourMinuteTime(
                    (temp.get() shr 6) and 0b11111,
                    temp.get() and 0b111111
                )
            )
        }


        private fun List<TimeException>.encode(): ByteArray {
            val buf =
                ByteBuffer.allocate(2 * Int.SIZE_BYTES * this.size).order(ByteOrder.LITTLE_ENDIAN)
            for (d in this) {
                buf.putInt(TimeUnit.MILLISECONDS.toSeconds(d.start.time).toInt())
                buf.putInt(TimeUnit.MILLISECONDS.toSeconds(d.end.time).toInt())
            }

            return buf.array()
        }

        private fun decodeTimeExceptions(bytes: ByteArray): List<TimeException> {
            val buf =
                ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asIntBuffer()
            val temp = IntArray(buf.remaining())
            buf.get(temp)
            assert(temp.size % 2 == 0)

            val ret = mutableListOf<TimeException>()
            val iterator = temp.iterator()
            while (iterator.hasNext()) {
                val start = iterator.next().toLong()
                val end = iterator.next().toLong()
                ret.add(
                    TimeException(
                        Date(start * 1000),
                        Date(end * 1000)
                    )
                )
            }

            return ret
        }

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

                    val configInJson = parentContext.sharedPreferences.getString(
                        parentContext.TIME_TABLE_CONFIG_KEY,
                        Gson().toJson(TimetableConfig())
                    )
                    val timetableConfigType =
                        object : TypeToken<TimetableConfig>() {}.type
                    this.timetableConfig =
                        Gson().fromJson<TimetableConfig>(configInJson, timetableConfigType)
                    this.characteristicsToWrite.clear()

                    for (service in gatt.services) {
                        if (service.uuid == TIMETABLE_SERVICE_UUID) {
                            Log.d(BLE_SERVICE_LOG_TAG, "Found timetable service")
                            for (characteristic in service.characteristics) {
                                var isKnown = true
                                when (characteristic.uuid) {
                                    MORNING_CURFEW_CHARACTERISTIC_UUID -> {
                                        characteristic.value =
                                            timetableConfig.morningCurfew.encode()
                                    }
                                    ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID -> {
                                        Log.d(BLE_SERVICE_LOG_TAG, "found active exceptions")
                                        characteristic.value =
                                            timetableConfig.activeExceptions.encode()
                                    }
                                    else -> {
                                        isKnown = false
                                        Log.d(BLE_SERVICE_LOG_TAG, "unknown characteristic")
                                    }
                                }
                                if (isKnown) {
                                    characteristicsToWrite.add(characteristic)
                                }
                            }
                        }
                    }
                }
                else -> Log.w(BLE_SERVICE_LOG_TAG, "onServicesDiscovered received: $status")
            }
            if (characteristicsToWrite.isNotEmpty()) {
                gatt.writeCharacteristic(characteristicsToWrite.removeAt(0))
            }
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?,
            status: Int
        ) {
            super.onCharacteristicWrite(gatt, characteristic, status)

            if (gatt != null) {
                assert(status == BluetoothGatt.GATT_SUCCESS)
                if (characteristicsToWrite.isNotEmpty()) {
                    assert(gatt.writeCharacteristic(characteristicsToWrite.removeAt(0)))
                }
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
                    with(parentContext) {
                        characteristic?.value?.let { values ->
                            when (characteristic.uuid) {
                                ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID -> {
                                    val timetableConfigType =
                                        object : TypeToken<TimetableConfig>() {}.type
                                    val s = Gson().toJson(timetableConfig)
                                    Gson().fromJson<TimetableConfig>(s, timetableConfigType)

//                                    timeExceptionList.replace(values)
//                                    Log.d(
//                                        BLE_SERVICE_LOG_TAG,
//                                        "gatt success: ${timeExceptionList.getDates()}"
//                                    )
                                }
                                else -> {
                                    Log.d(BLE_SERVICE_LOG_TAG, "unknown BLE characteristic")
                                }
                            }
                        }
                    }
                }
                else -> {
                    Log.d(BLE_SERVICE_LOG_TAG, "gatt read failed: $status")
                }
            }
        }
    }

    private lateinit var bleGATTCallback: BleGATTCallback


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

                    BleIpcCmd.CONFIGURE_TIMETABLE -> {
                        val timetableConfig = receivedObj as TimetableConfig
                        with(sharedPreferences.edit()) {
                            putString(TIME_TABLE_CONFIG_KEY, Gson().toJson(timetableConfig))
                            commit()
                        }
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