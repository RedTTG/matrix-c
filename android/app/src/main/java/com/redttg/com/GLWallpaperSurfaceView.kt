package com.redttg.com

import android.content.Context
import android.opengl.GLSurfaceView
import android.service.wallpaper.WallpaperService
import android.view.SurfaceHolder

/**
 * Custom GLSurfaceView that works with WallpaperService
 */
class GLWallpaperSurfaceView(context: Context) : GLSurfaceView(context) {
    
    private var engine: WallpaperService.Engine? = null
    
    fun setEngine(engine: WallpaperService.Engine) {
        this.engine = engine
    }
    
    override fun getHolder(): SurfaceHolder {
        return engine?.surfaceHolder ?: super.getHolder()
    }
    
    fun onDestroy() {
        // Clean up resources
    }
}
