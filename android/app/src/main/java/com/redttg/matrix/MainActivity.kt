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
        val credits = TextView(this).apply {
            text = "Made with much love, by RedTTG!\n\n"
            textSize = 20f
            setTextColor(0xFFf277f2.toInt())
            setPadding(0, 0, 0, 48)
            gravity = android.view.Gravity.CENTER
        }
        layout.addView(credits)

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
            val params = android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                android.widget.LinearLayout.LayoutParams.WRAP_CONTENT
            )
            params.setMargins(0, 0, 0, 48)
            layoutParams = params
        }
        layout.addView(setWallpaperButton)

        // GitHub Button
        val githubButton = Button(this).apply {
            text = "View on GitHub"
            textSize = 18f
            setBackgroundColor(0xFF1a1a1a.toInt())
            setTextColor(0xFF00FF00.toInt())
            setPadding(32, 24, 32, 24)
            setOnClickListener {
                openGitHub()
            }
            val params = android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                android.widget.LinearLayout.LayoutParams.WRAP_CONTENT
            )
            params.setMargins(0, 0, 0, 0)
            layoutParams = params
        }
        layout.addView(githubButton)

        val openSourceNote = TextView(this).apply {
            text = "This app is open source and available for Linux too!"
            textSize = 14f
            setTextColor(0xFF00AA00.toInt())
            setPadding(0, 16, 0, 32)
            gravity = android.view.Gravity.CENTER
        }
        layout.addView(openSourceNote)

        val feedback = TextView(this).apply {
            text = "\nIf you encounter any issues, please contact me on Discord: RedTTG"
            textSize = 16f
            setTextColor(0xFFAAAAAA.toInt())
            setPadding(0, 0, 0, 32)
            val params = android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                android.widget.LinearLayout.LayoutParams.WRAP_CONTENT
            )
            params.setMargins(0, 24, 0, 0)
            layoutParams = params
        }
        layout.addView(feedback)

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

    private fun openGitHub() {
        try {
            val intent = Intent(Intent.ACTION_VIEW).apply {
                data = android.net.Uri.parse("https://github.com/RedTTG/matrix-c")
            }
            startActivity(intent)
        } catch (e: Exception) {
            Toast.makeText(
                this,
                "Unable to open browser",
                Toast.LENGTH_SHORT
            ).show()
        }
    }
}

