package com.xys.ffmpegx.media;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceView;

public class FFVideoView extends SurfaceView {
    public FFVideoView(Context context) {
        this(context, null);
    }

    public FFVideoView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public FFVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        getHolder().setFormat(PixelFormat.RGBA_8888);
    }

    public void play(final String url) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                render(url, FFVideoView.this.getHolder().getSurface());
            }
        }).start();
    }

    public native void render(String url, Surface surface);
}
