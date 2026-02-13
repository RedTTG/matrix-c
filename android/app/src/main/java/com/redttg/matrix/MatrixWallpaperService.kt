package com.redttg.matrix

import android.service.wallpaper.WallpaperService
import android.util.Log
import android.view.MotionEvent
import android.view.SurfaceHolder

class MatrixWallpaperService : WallpaperService() {
    
    override fun onCreateEngine(): Engine {
        return MatrixEngine()
    }
    
    inner class MatrixEngine : Engine() {
        
        private var renderThread: RenderThread? = null
        private var isVisible = false

        override fun onCreate(surfaceHolder: SurfaceHolder) {
            super.onCreate(surfaceHolder)
            Log.i(TAG, "Engine onCreate")

            // Disable touch events and offset notifications since we don't need them
            setTouchEventsEnabled(false)
            setOffsetNotificationsEnabled(false)

            // Set surface format for OpenGL ES
            surfaceHolder.setFormat(android.graphics.PixelFormat.RGBA_8888)
        }

        override fun onSurfaceCreated(holder: SurfaceHolder) {
            super.onSurfaceCreated(holder)
            Log.i(TAG, "Surface created")

            // Ensure format is set
            holder.setFormat(android.graphics.PixelFormat.RGBA_8888)
        }

        override fun onSurfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
            super.onSurfaceChanged(holder, format, width, height)
            Log.i(TAG, "Surface changed: ${width}x${height}")

            // Stop existing render thread if any and wait for it to finish
            renderThread?.let { thread ->
                Log.i(TAG, "Stopping existing render thread...")
                thread.stopRendering()
                // Wait for the thread to fully stop before proceeding
                thread.join(2000) // Wait up to 2 seconds
                if (thread.isAlive) {
                    Log.w(TAG, "Render thread didn't stop in time, forcing...")
                }
            }
            renderThread = null

            // CRITICAL: Give Android's hardware renderer (hwuiTask) time to fully release the surface
            // The hwuiTask runs asynchronously and may still be accessing the surface
            Log.i(TAG, "Waiting for hardware renderer to release surface...")
            Thread.sleep(500)

            // Start new render thread with the surface
            renderThread = RenderThread(holder, width, height)
            renderThread?.start()

            // Always start rendering when surface is created - wallpaper should animate on home screen
            renderThread?.resumeRendering()
        }

        override fun onSurfaceDestroyed(holder: SurfaceHolder) {
            super.onSurfaceDestroyed(holder)
            Log.i(TAG, "Surface destroyed")

            renderThread?.stopRendering()
            renderThread = null
        }

        override fun onVisibilityChanged(visible: Boolean) {
            super.onVisibilityChanged(visible)
            Log.i(TAG, "Visibility changed: $visible")
            isVisible = visible

            // Keep rendering active on home screen - only pause if explicitly invisible
            // The visibility flag is unreliable for wallpapers, they should always animate
            if (visible) {
                renderThread?.resumeRendering()
            }
            // Don't pause when visibility is false - wallpaper should keep animating
        }
        
        override fun onTouchEvent(event: MotionEvent) {
            val x = event.x
            val y = event.y
            val pressed = event.action == MotionEvent.ACTION_DOWN || 
                         event.action == MotionEvent.ACTION_MOVE
            nativeTouchEvent(x, y, pressed)
            super.onTouchEvent(event)
        }
        
        override fun onDestroy() {
            super.onDestroy()
            Log.i(TAG, "Engine onDestroy")
            renderThread?.stopRendering()
            renderThread = null
        }
        
        inner class RenderThread(
            private val holder: SurfaceHolder,
            private val width: Int,
            private val height: Int
        ) : Thread() {

            @Volatile
            private var running = false

            @Volatile
            private var rendering = false

            private var initialized = false
            
            override fun run() {
                Log.i(TAG, "Render thread started")
                running = true

                try {
                    // Initialize native renderer
                    nativeInit(holder.surface, width, height)
                    initialized = true
                    Log.i(TAG, "Native renderer initialized")

                    // Rendering loop
                    while (running) {
                        if (rendering && initialized) {
                            nativeRender()
                        } else {
                            // Sleep when not rendering to save battery
                            sleep(100)
                        }
                    }
                } catch (e: Exception) {
                    Log.e(TAG, "Error in render thread", e)
                } finally {
                    if (initialized) {
                        nativeDestroy()
                        initialized = false
                    }
                    Log.i(TAG, "Render thread stopped")
                }
            }

            fun resumeRendering() {
                Log.i(TAG, "Resuming rendering")
                rendering = true
            }

            fun pauseRendering() {
                Log.i(TAG, "Pausing rendering")
                rendering = false
            }

            fun stopRendering() {
                Log.i(TAG, "Stopping rendering")
                running = false
                rendering = false
                interrupt()
                try {
                    join(1000)
                } catch (e: InterruptedException) {
                    Log.w(TAG, "Interrupted while waiting for render thread to stop")
                }
            }
        }
    }
    
    private external fun nativeInit(surface: Any, width: Int, height: Int)
    private external fun nativeRender()
    private external fun nativeDestroy()
    private external fun nativeTouchEvent(x: Float, y: Float, pressed: Boolean)
    
    companion object {
        private const val TAG = "MatrixWallpaper"

        init {
            System.loadLibrary("matrix")
        }
    }
}
