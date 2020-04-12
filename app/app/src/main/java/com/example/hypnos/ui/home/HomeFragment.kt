package com.example.hypnos.ui.home

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.*
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import com.example.hypnos.BleService
import com.example.hypnos.R
import com.example.hypnos.isPermissionGranted
import com.example.hypnos.requestPermission
import kotlinx.android.synthetic.main.fragment_home.*

class HomeFragment : Fragment() {

    private lateinit var homeViewModel: HomeViewModel

    private val resultsAdapter =
        ScanResultsAdapter {
            sendMsg(
                BleService.Companion.BleIpcCmd.CONNECT_DEVICE,
                BleService.Companion.ConnectInfo(it.bleDevice.macAddress)
            )
            Log.v("handler", "user clicked on ${it.bleDevice.macAddress}")
        }

    override fun onCreateView(
            inflater: LayoutInflater,
            container: ViewGroup?,
            savedInstanceState: Bundle?
    ): View? {
        super.onCreateView(inflater, container, savedInstanceState)

        if (context?.isPermissionGranted()!!) {
            return inflater.inflate(R.layout.fragment_home, container, false)
        }

        return null;
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        Intent(context, BleService::class.java).also { intent ->
            context?.bindService(intent, mConnection, Context.BIND_AUTO_CREATE)
        }

        if (context?.isPermissionGranted()!!) {
            configureResultList()

            scan_toggle_btn.setOnClickListener {
                sendMsg(BleService.Companion.BleIpcCmd.START_SCAN, null)
            }
            disconnect_button.setOnClickListener {
                sendMsg(
                    BleService.Companion.BleIpcCmd.DISCONNECT_DEVICE,
                    BleService.Companion.DisconnectInfo(true)
                )
            }
        }
        Log.v("event", "view created")
    }

    private fun configureResultList() {
        with(scan_results) {
            setHasFixedSize(true)
            itemAnimator = null
            adapter = resultsAdapter
        }
    }

    private fun dispose() {
        resultsAdapter.clearScanResults()
    }

    override fun onDestroy() {
        super.onDestroy()
        sendMsg(BleService.Companion.BleIpcCmd.DEINIT, null)
    }

    private var mService: Messenger? = null
    private var bound: Boolean = false
    private val mConnection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            mService = Messenger(service)
            bound = true
            sendMsg(
                BleService.Companion.BleIpcCmd.INIT,
                BleService.Companion.InitInfo(resultsAdapter)
            )
            Log.d("Service","Service connected")
        }

        override fun onServiceDisconnected(className: ComponentName) {
            mService = null
            bound = false
        }
    }

    private fun sendMsg(what: BleService.Companion.BleIpcCmd, obj: Any?) {
        if (bound) {
            val msg: Message = Message.obtain(null, what.ordinal, obj)
            try {
                mService?.send(msg)
            } catch (e: RemoteException) {
                e.printStackTrace()
            }
        }
    }

    override fun onPause() {
        super.onPause()

//        scanDisposable?.dispose()
//        triggerDisconnect()

        if (bound) {
            context?.unbindService(mConnection)
            bound = false
        }
    }
}