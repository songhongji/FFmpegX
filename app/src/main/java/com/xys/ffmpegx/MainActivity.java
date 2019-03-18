package com.xys.ffmpegx;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.TextView;

import com.xys.ffmpegx.media.FFVideoView;

public class MainActivity extends AppCompatActivity {

    private TextView startPlayTv;
    private FFVideoView playView;

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        startPlayTv = findViewById(R.id.main_tv);
        startPlayTv.setText("开始播放");
        playView = findViewById(R.id.main_play_view);
        startPlayTv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                playView.play("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");
            }
        });
    }
}
