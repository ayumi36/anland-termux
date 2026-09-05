package com.anland.termux;

import android.annotation.TargetApi;
import android.app.Activity;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Build;
import android.view.Display;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

/** API 29 window operations. New framework calls stay in an API-gated class. */
final class PlatformCompat {
    private PlatformCompat() {}

    @SuppressWarnings("deprecation")
    static Display display(Activity activity) {
        // WindowManager belongs to this Activity's display, including on API 29.
        return activity.getWindowManager().getDefaultDisplay();
    }

    @SuppressWarnings("deprecation")
    static Rect maximumDisplayBounds(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            return Api30.maximumDisplayBounds(activity);
        Point size = new Point();
        display(activity).getRealSize(size);
        return new Rect(0, 0, size.x, size.y);
    }

    @SuppressWarnings("deprecation")
    static void configureDisplayWindow(Window window) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Api30.edgeToEdge(window);
        } else {
            // FLAG_FULLSCREEN prevents adjustResize on Android 10. Hide the
            // bars with immersive UI flags instead, retaining a full-size
            // layout while legacy insets report the docked keyboard.
            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN
                | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        }
    }

    @SuppressWarnings("deprecation")
    static void hideSystemBars(Window window) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Api30.hideSystemBars(window);
        } else {
            window.getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }

    static int immersiveCutoutMode() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R
            ? WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
            : WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
    }

    @SuppressWarnings("deprecation")
    static int imeBottom(WindowInsets insets, View root) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            return insets == null ? 0 : Api30.imeBottom(insets);

        int stableBottom = insets == null ? 0 : insets.getStableInsetBottom();
        int systemBottom = insets == null ? 0 : insets.getSystemWindowInsetBottom();
        if (systemBottom > stableBottom)
            return systemBottom; // Includes nav area, as Type.ime() does.

        // Some fullscreen OEMs update the visible frame without dispatching
        // new system insets. Root-local geometry avoids counting the status bar
        // or space outside a freeform window as part of the keyboard.
        if (root != null && root.isLaidOut()) {
            Rect visible = new Rect();
            root.getWindowVisibleDisplayFrame(visible);
            int[] location = new int[2];
            root.getLocationOnScreen(location);
            int obscured = Math.max(0, location[1] + root.getHeight() - visible.bottom);
            int threshold = Math.max(stableBottom,
                Math.round(80 * root.getResources().getDisplayMetrics().density));
            if (visible.height() > 0 && obscured > threshold)
                return obscured;
        }
        return 0;
    }

    static boolean imeVisible(WindowInsets insets, View root) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            return insets != null && Api30.imeVisible(insets);
        return imeBottom(insets, root) > 0;
    }

    static void requestIme(Window window, View input) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            Api30.requestIme(window, input);
        // The caller also uses InputMethodManager on every release.
    }

    static void configureSettings(Window window, View content, int base) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Api30.configureSettings(window, content, base);
        } else {
            // API 29 still fits the window to bars and resizes for the IME.
            // Let it do so, avoiding double padding of a resized ScrollView.
            window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
            content.setPadding(base, base, base, base);
        }
    }

    @TargetApi(30)
    private static final class Api30 {
        static Rect maximumDisplayBounds(Activity activity) {
            return activity.getWindowManager().getMaximumWindowMetrics().getBounds();
        }

        static void edgeToEdge(Window window) {
            window.setDecorFitsSystemWindows(false);
        }

        static void hideSystemBars(Window window) {
            WindowInsetsController controller = window.getInsetsController();
            if (controller == null) return;
            controller.hide(WindowInsets.Type.systemBars());
            controller.setSystemBarsBehavior(
                WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }

        static int imeBottom(WindowInsets insets) {
            return insets.getInsets(WindowInsets.Type.ime()).bottom;
        }

        static boolean imeVisible(WindowInsets insets) {
            return insets.isVisible(WindowInsets.Type.ime());
        }

        static void requestIme(Window window, View input) {
            WindowInsetsController controller = input.getWindowInsetsController();
            if (controller == null) controller = window.getInsetsController();
            if (controller != null) controller.show(WindowInsets.Type.ime());
        }

        static void configureSettings(Window window, View content, int base) {
            edgeToEdge(window);
            content.setOnApplyWindowInsetsListener((v, insets) -> {
                android.graphics.Insets in = insets.getInsets(
                    WindowInsets.Type.systemBars() | WindowInsets.Type.ime());
                v.setPadding(base + in.left, base + in.top,
                    base + in.right, base + in.bottom);
                return insets;
            });
            content.requestApplyInsets();
        }
    }
}
