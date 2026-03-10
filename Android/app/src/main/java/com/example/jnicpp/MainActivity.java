package com.example.jnicpp;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.res.AssetManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.widget.Toast;
import com.example.jnicpp.databinding.ActivityMainBinding;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
public class MainActivity extends AppCompatActivity implements GLSurfaceView.Renderer {
    private static final String TAG = "MainActivity";
    // Used to load the 'jnicpp' library on application startup.
    static {
        try {
            //Try to load both jnicpp and jnicppd libraries.
            if (BuildConfig.IS_DEBUG_BUILD) {
                System.loadLibrary("fmodL");  // Debug / Logging version
                try {
                    System.loadLibrary("jnicppd");
                }
                catch(UnsatisfiedLinkError e) {
                    System.loadLibrary("jnicpp");
                }
            } else {
                System.loadLibrary("fmod");   // Release / Optimized version
                try {
                    System.loadLibrary("jnicpp");
                }
                catch(UnsatisfiedLinkError e) {
                    System.loadLibrary("jnicppd");
                }
            }
            Log.d(TAG, "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
        }
    }
    private ActivityMainBinding binding;
    private GLSurfaceView glSurfaceView;
    private boolean isInitialized = false;

    private SensorManager mSensorManager;
    private Sensor mGyroscope;
    private SensorEventListener mSensorListener;

    public native void handleNativeGyroscope(float x, float y, float z, long timestamp);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate called");
        org.fmod.FMOD.init(this);
        try {
            binding = ActivityMainBinding.inflate(getLayoutInflater());
            setContentView(binding.getRoot());

//GYROSCOPE Initialization
            mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
            mGyroscope = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);

//Implement Listener
            mSensorListener = new SensorEventListener() {
                @Override
                public void onAccuracyChanged(Sensor sensor, int accuracy) {
                    //Do nothing for now
                }

                @Override
                public void onSensorChanged(SensorEvent event) {
                    // Only forward data if initialized and the sensor is the gyroscope
                    if (isInitialized && event.sensor.getType() == Sensor.TYPE_GYROSCOPE) {
                        handleNativeGyroscope(
                                event.values[0],
                                event.values[1],
                                event.values[2],
                                event.timestamp
                        );
                    }
                }
            };

// Setup GLSurfaceView
            glSurfaceView = binding.glSurfaceView;
            glSurfaceView.setEGLContextClientVersion(3); // OpenGL ES 3.X, uses highest version.
            glSurfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
            glSurfaceView.setRenderer(this);
            glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
// Example of a call to a native method
            String message = stringFromJNI();
            Log.d(TAG, "Native message: " + message);
        } catch (Exception e) {
            Log.e(TAG, "Error in onCreate", e);
            showToast("Error initializing app: " + e.getMessage());
        }
    }
    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause called");

        // --- GYROSCOPE UNREGISTER ---
        if (mSensorManager != null) {
            mSensorManager.unregisterListener(mSensorListener);
        }

        if (glSurfaceView != null) {
            glSurfaceView.onPause();
        }

        if (glSurfaceView != null) {
            glSurfaceView.onPause();
        }
    }
    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume called");

        // --- GYROSCOPE REGISTER ---
        if (mGyroscope != null) {
            // SENSOR_DELAY_GAME is a standard rate for games (about 50Hz)
            mSensorManager.registerListener(mSensorListener, mGyroscope, SensorManager.SENSOR_DELAY_GAME);
        }

        if (glSurfaceView != null) {
            glSurfaceView.onResume();
        }

        if (glSurfaceView != null) {
            glSurfaceView.onResume();
        }
    }
    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy called");
        if (isInitialized) {
            try {
                cleanupGL();
            } catch (Exception e) {
                Log.e(TAG, "Error during cleanup", e);
            }
        }
        org.fmod.FMOD.close();
        super.onDestroy();
    }
    // Helper method to show toast from any thread
    private void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(MainActivity.this, message, Toast.LENGTH_SHORT).show();
            }
        });
    }
    // GLSurfaceView.Renderer implementation
    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.d(TAG, "Surface created");
        try {
            // Initialize OpenGL in native code with AssetManager
            boolean success = initGL(getAssets());
            if (success) {
                isInitialized = true;
                Log.d(TAG, "OpenGL ES 3.2 initialized successfully");
            } else {
                Log.e(TAG, "Failed to initialize OpenGL ES 3.2");
                showToast("Failed to initialize OpenGL ES 3.2");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in onSurfaceCreated", e);
            showToast("Error creating surface: " + e.getMessage());
        }
    }
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.d(TAG, "Surface changed: " + width + "x" + height);

        if(isInitialized){
            setDisplaySize(width, height);
        }
    }
    @Override
    public void onDrawFrame(GL10 gl) {
        if (isInitialized) {
            try {
                renderFrame();
            } catch (Exception e) {
                Log.e(TAG, "Error in onDrawFrame", e);
            }
        }
    }

    // ------------------------------------------------------------------------
    // New Input Handling Overrides (Capture events from Android OS)
    // ------------------------------------------------------------------------
    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        //[Touch/Motion]
        if(isInitialized)
        {
            //Pass action type (Up, Down, etc), and primary pointer pos
            handleNativeTouchEvent(event.getActionMasked(), event.getX(), event.getY());
            return true;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        //[Key down/Pressed]
        if(isInitialized)
        {
            //pass raw android keycode
            handleNativeKeyEvent(keyCode, true);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        //[Key up/Released]
        if(isInitialized)
        {
            handleNativeKeyEvent(keyCode, false);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    // --------------------------------------------------------------------------
    // New Native Declarations for Input (Tell Java compiler about C++ functions)
    // --------------------------------------------------------------------------

    /**
     * Handles touch motion events (DOWN, UP, MOVE, etc.)
     */
    public native void handleNativeTouchEvent(int action, float x, float y);

    /**
     * Handles key press events (keyboard or hardware buttons)
     */
    public native void handleNativeKeyEvent(int keyCode, boolean isDown);

/**
 * A native method that is implemented by the 'jnicpp' native library,
 * which is packaged with this application.
 */
public native String stringFromJNI();
    /**
     * Initialize OpenGL ES 3.0 renderer
     */
    public native boolean initGL(AssetManager assets);
    /**
     * Render a frame
     */
    public native void renderFrame();
    /**
     * Cleanup OpenGL resources
     */
    public native void cleanupGL();

    /**
     * Notify native code of the current display size.
     */
    public native void setDisplaySize(int width, int height);
}
