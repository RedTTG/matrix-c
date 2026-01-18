package com.redttg.matrix

import android.opengl.GLSurfaceView
import android.service.wallpaper.WallpaperService
import android.view.MotionEvent
import android.view.SurfaceHolder
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MatrixWallpaperService : WallpaperService() {
    
    override fun onCreateEngine(): Engine {
        return MatrixEngine()
    }
    
    inner class MatrixEngine : Engine() {
        
        private var glSurfaceView: GLWallpaperSurfaceView? = null
        private var renderer: MatrixRenderer? = null
        
        override fun onCreate(surfaceHolder: SurfaceHolder) {
            super.onCreate(surfaceHolder)
            
            glSurfaceView = GLWallpaperSurfaceView(this@MatrixWallpaperService).apply {
                setEngine(this@MatrixEngine)
                setEGLContextClientVersion(3)
                renderer = MatrixRenderer(surfaceHolder)
                setRenderer(renderer)
                renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
            }
        }
        
        override fun onVisibilityChanged(visible: Boolean) {
            super.onVisibilityChanged(visible)
            if (visible) {
                glSurfaceView?.onResume()
            } else {
                glSurfaceView?.onPause()
            }
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
            glSurfaceView?.onDestroy()
            nativeDestroy()
        }
        
        inner class MatrixRenderer(private val holder: SurfaceHolder) : GLSurfaceView.Renderer {
            private var initialized = false
            
            override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
                // Surface created, but we'll init in onSurfaceChanged
            }
            
            override fun onSurfaceChanged(gl: GL10, width: Int, height: Int) {
                if (!initialized) {
                    nativeInit(holder.surface, width, height)
                    initialized = true
                }
            }
            
            override fun onDrawFrame(gl: GL10) {
                nativeRender()
            }
        }
    }
    
    private external fun nativeInit(surface: Any, width: Int, height: Int)
    private external fun nativeRender()
    private external fun nativeDestroy()
    private external fun nativeTouchEvent(x: Float, y: Float, pressed: Boolean)
    
    companion object {
        init {
            System.loadLibrary("matrix")
        }
    }
}
