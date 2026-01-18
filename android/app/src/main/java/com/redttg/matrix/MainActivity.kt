package com.redttg.matrix

import android.app.Activity
import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.Toast

class MainActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Create a simple layout programmatically
        val layout = android.widget.LinearLayout(this).apply {
            orientation = android.widget.LinearLayout.VERTICAL
            setPadding(48, 48, 48, 48)
            setBackgroundColor(0xFF000000.toInt())
        }

        // Title
        val title = TextView(this).apply {
            text = "Matrix Wallpaper"
            textSize = 24f
            setTextColor(0xFF00FF00.toInt())
            setPadding(0, 0, 0, 32)
            gravity = android.view.Gravity.CENTER
        }
        layout.addView(title)

        // Description
        val description = TextView(this).apply {
            text = "Digital rain effect from The Matrix.\n\nTap the button below to set this as your live wallpaper."
            textSize = 16f
            setTextColor(0xFF00FF00.toInt())
            setPadding(0, 0, 0, 48)
        }
        layout.addView(description)

        // Set Wallpaper Button
        val setWallpaperButton = Button(this).apply {
            text = "Set as Wallpaper"
            textSize = 18f
            setBackgroundColor(0xFF003300.toInt())
            setTextColor(0xFF00FF00.toInt())
            setPadding(32, 24, 32, 24)
            setOnClickListener {
                setWallpaper()
            }
        }
        layout.addView(setWallpaperButton)

        setContentView(layout)
    }

    private fun setWallpaper() {
        try {
            val intent = Intent(WallpaperManager.ACTION_CHANGE_LIVE_WALLPAPER).apply {
                putExtra(
                    WallpaperManager.EXTRA_LIVE_WALLPAPER_COMPONENT,
                    ComponentName(this@MainActivity, MatrixWallpaperService::class.java)
                )
            }
            startActivity(intent)
        } catch (e: Exception) {
            // Fallback to wallpaper picker if the direct intent doesn't work
            try {
                val intent = Intent(WallpaperManager.ACTION_LIVE_WALLPAPER_CHOOSER)
                startActivity(intent)
            } catch (e2: Exception) {
                Toast.makeText(
                    this,
                    "Please set wallpaper manually: Long press home screen → Wallpapers → Matrix Rain",
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }
}

