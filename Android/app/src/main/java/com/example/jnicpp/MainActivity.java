package com.example.jnicpp;
import androidx.appcompat.app.AppCompatActivity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import com.example.jnicpp.databinding.ActivityMainBinding;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
public class MainActivity extends AppCompatActivity implements GLSurfaceView.Renderer {
    private static final String TAG = "MainActivity";
    // Used to load the 'jnicpp' library on application startup.
    static {
        try {
            System.loadLibrary("jnicpp");
            Log.d(TAG, "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
        }
    }
    private ActivityMainBinding binding;
    private GLSurfaceView glSurfaceView;
    private boolean isInitialized = false;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate called");
        try {
            binding = ActivityMainBinding.inflate(getLayoutInflater());
            setContentView(binding.getRoot());
// Setup GLSurfaceView
            glSurfaceView = binding.glSurfaceView;
            glSurfaceView.setEGLContextClientVersion(3); // OpenGL ES 3.0
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
        if (glSurfaceView != null) {
            glSurfaceView.onPause();
        }
    }
    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume called");
        if (glSurfaceView != null) {
            glSurfaceView.onResume();
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy called");
        if (isInitialized) {
            try {
                cleanupGL();
            } catch (Exception e) {
                Log.e(TAG, "Error during cleanup", e);
            }
        }
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
// Initialize OpenGL in native code (no longer needs surface parameter)
            boolean success = initGL();
            if (success) {
                isInitialized = true;
                Log.d(TAG, "OpenGL ES 3.0 initialized successfully");
            } else {
                Log.e(TAG, "Failed to initialize OpenGL ES 3.0");
                showToast("Failed to initialize OpenGL ES 3.0");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in onSurfaceCreated", e);
            showToast("Error creating surface: " + e.getMessage());
        }
    }
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.d(TAG, "Surface changed: " + width + "x" + height);
// Viewport is handled in native code
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
/**
 * A native method that is implemented by the 'jnicpp' native library,
 * which is packaged with this application.
 */
public native String stringFromJNI();
    /**
     * Initialize OpenGL ES 3.0 renderer
     */
    public native boolean initGL();
    /**
     * Render a frame
     */
    public native void renderFrame();
    /**
     * Cleanup OpenGL resources
     */
    public native void cleanupGL();
}