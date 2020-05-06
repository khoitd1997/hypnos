package com.example.hypnos.ble_service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.Build
import androidx.core.app.NotificationCompat
import com.example.hypnos.MainActivity
import com.example.hypnos.R

class NotificationHelper(
    private val sharedPreferences: SharedPreferences,
    private var parentContext: Context
) {
    companion object {
        const val PERSISTENT_NOTIFICATION_ID = 1 // ID for the always-present notification
        const val NOTIFICATION_CHANNEL_ID = "BleServiceChannel"
    }

    private lateinit var manager: NotificationManager

    init {
        // NOTE: DO NOT REMOVE THIS
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                NOTIFICATION_CHANNEL_ID,
                "BLE Service Channel",
                NotificationManager.IMPORTANCE_DEFAULT
            )

            manager = parentContext.getSystemService(NotificationManager::class.java)!!
            manager.createNotificationChannel(serviceChannel)
        }
    }

    private fun updateNotification(notification: Notification) {
        manager.notify(BleService.NOTIFICATION_ID, notification)
    }

    private fun buildSummaryNotification(): Notification {
        val notificationIntent = Intent(parentContext, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            parentContext,
            0, notificationIntent, 0
        )
        val title =
            "${sharedPreferences.getInt(
                "timeLeft",
                -1
            )} minutes till " + (if (sharedPreferences.getBoolean(
                    "isBreakPhase",
                    false
                )
            ) "work" else "break")
        val content = "Battery: ${sharedPreferences.getInt(
            "battery",
            -1
        )}%" + "\nLast Sync: ${sharedPreferences.getString("syncTime", "None")}"

        return NotificationCompat.Builder(
            parentContext,
            NOTIFICATION_CHANNEL_ID
        )
            .setContentTitle(title)
            .setSmallIcon(R.drawable.foreground_icon)
            .setContentText(content)
            .setStyle(NotificationCompat.BigTextStyle().bigText(content))
            .setContentIntent(pendingIntent)
            .build()
    }

    fun updateNotificationWithSummary() {
        updateNotification(buildSummaryNotification())
    }

    private fun buildTitleOnlyMessageNotification(title: String): Notification {
        val notificationIntent = Intent(parentContext, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            parentContext,
            0, notificationIntent, 0
        )

        return NotificationCompat.Builder(
            parentContext,
            NOTIFICATION_CHANNEL_ID
        )
            .setContentTitle(title)
            .setSmallIcon(R.drawable.foreground_icon)
            .setContentText("")
            .setContentIntent(pendingIntent)
            .build()
    }

    fun buildDisconnectedNotification(): Notification {
        return buildTitleOnlyMessageNotification("Disconnected")
    }

    fun updateDisconnected() {
        updateNotification(buildDisconnectedNotification())
    }

    fun updateConnected() {
        updateNotification(buildTitleOnlyMessageNotification("Connected"))
    }
}