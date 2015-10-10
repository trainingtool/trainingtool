package com.stac.powertrainer_44;

import android.os.Bundle;
import android.content.Intent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import com.stac.powertrainer_44.BlunoLibrary;

import java.util.Vector;


public class MainActivity  extends BlunoLibrary {
    private Button buttonScan;
    private Button buttonSerialSend;
    private EditText serialSendText;
    private TrainerState mTrainerState = new TrainerState();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        onCreateProcess();														//onCreate Process by BlunoLibrary

        serialBegin(115200);													//set the Uart Baudrate on BLE chip to 115200

        buttonScan = (Button) findViewById(R.id.scan);					//initial the button for scanning the BLE device
        buttonScan.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                buttonScanOnClickProcess();										//Alert Dialog for selecting the BLE device
            }
        }); // end setOnClickListener
    }

    protected void onResume(){
        super.onResume();
        System.out.println("BlUNOActivity onResume");
        onResumeProcess();														//onResume Process by BlunoLibrary
    }



    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        onActivityResultProcess(requestCode, resultCode, data);					//onActivityResult Process by BlunoLibrary
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    protected void onPause() {
        super.onPause();
        onPauseProcess();														//onPause Process by BlunoLibrary
    }

    protected void onStop() {
        super.onStop();
        onStopProcess();														//onStop Process by BlunoLibrary
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        onDestroyProcess();														//onDestroy Process by BlunoLibrary
    }

    @Override
    public void onConectionStateChange(connectionStateEnum theConnectionState) {//Once connection state changes, this function will be called
        D.p("Connection change " + theConnectionState.toString());
        switch (theConnectionState) {											//Four connection state
            case isConnected:
                buttonScan.setText("Connected");
                break;
            case isConnecting:
                buttonScan.setText("Connecting");
                break;
            case isToScan:
                buttonScan.setText("Scan");
                break;
            case isScanning:
                buttonScan.setText("Scanning");
                break;
            case isDisconnecting:
                buttonScan.setText("isDisconnecting");
                break;
            default:
                break;
        }
    }

    private class TrainerState
    {
        public static final int POWER = 0;
        public static final int SPEED = 1;
        public static final int POINT = 2;
        public static final int SLOPE = 3;
        public static final int WHEELTURNS = 4;

        private class XYPoint
        {
            public float x = 0;
            public float y = 0;
            public XYPoint(float _x, float _y)
            {
                this.x = _x;
                this.y = _y;
            }
        }
        private int cWheelTurns;
        private float flEstimatedSlope; // slope for y=mx, where x = speed, y = watts
        private Vector<XYPoint> lstPoints = new Vector<XYPoint>();
        private float flLastPower;
        private float flLastSpeed;

        public void addPoint(float speed, float watts, int index)
        {
            if(lstPoints.size() == index) {
                lstPoints.add(index, new XYPoint(speed, watts));
            } else if(index > lstPoints.size()) {
                // just ignore this one...
            } else {
                lstPoints.set(index, new XYPoint(speed, watts));
            }
        }
        public void setPower(float power) {
            this.flLastPower = power;
        }
        public void setSpeed(float speed) {
            this.flLastSpeed = speed;
        }
        public void setWheelTurns(int cTurns) {
            this.cWheelTurns = cTurns;
        }
        public void setSlope(float slope) {
            this.flEstimatedSlope = slope;
        }
    }

    public void parseSerial(String str) {
        String[] strTypeAndArgs = str.split(":");
        if(strTypeAndArgs.length > 1) {
            String strType = strTypeAndArgs[0];
            String[] rgArgs = strTypeAndArgs[1].split(",");
            int iType = Integer.parseInt(strType);
            switch(iType) {
                case TrainerState.WHEELTURNS:
                    // single arg:
                    if(rgArgs.length >= 1) {
                        int cWheelTurns = Integer.parseInt(rgArgs[0]);
                        mTrainerState.setWheelTurns(cWheelTurns);
                    }
                    break;
                case TrainerState.SLOPE:
                    // single floating arg
                    if(rgArgs.length >= 1) {
                        float flSlope = Float.parseFloat(rgArgs[0]);
                        mTrainerState.setSlope(flSlope);
                    }
                    break;
                case TrainerState.SPEED:
                    // single floating arg indicating speed in m/s
                    if(rgArgs.length >= 1) {
                        float speed = Float.parseFloat(rgArgs[0]);
                        mTrainerState.setSpeed(speed);
                    }
                    break;
                case TrainerState.POWER:
                    // single floating arg indicating power in W
                    if(rgArgs.length >= 1) {
                        float power = Float.parseFloat(rgArgs[0]);
                        mTrainerState.setPower(power);
                    }
                    break;
                case TrainerState.POINT:
                    // two floats, then an int
                    // <speed m/s> <power W> <index>
                    if(rgArgs.length >= 3) {
                        float speed = Float.parseFloat(rgArgs[0]);
                        float power = Float.parseFloat(rgArgs[1]);
                        int index = Integer.parseInt(rgArgs[2]);
                        mTrainerState.addPoint(speed, power, index);
                    }
                    break;
            }
        }
    }

    @Override
    public void onSerialReceived(String theString) {							//Once connection data received, this function will be called
        D.p("onSerialReceived " + theString);

        // we got a string!
        // format is:
        // <type of transmission>:<bunch of comma-separated parameters>
        this.parseSerial(TrainerState.POINT + ":5.5,4.3,2");
    }

}