package com.stac.powertrainer_44;

import android.app.ActionBar;
import android.os.Bundle;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.support.v7.widget.Toolbar;

import com.stac.powertrainer_44.BlunoLibrary;

import java.util.Vector;


public class MainActivity  extends BlunoLibrary {
    private Button buttonScan;
    private Button buttonSerialSend;
    private EditText serialSendText;
    private TrainerState mTrainerState = new TrainerState();
    private SlopeChart mSlopeChart;
    private boolean mStrainEnabled=false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        onCreateProcess();														//onCreate Process by BlunoLibrary

        serialBegin(115200);													//set the Uart Baudrate on BLE chip to 115200

        mStrainEnabled = false;

        buttonScan = (Button) findViewById(R.id.scan);					//initial the button for scanning the BLE device
        buttonScan.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                buttonScanOnClickProcess();                                        //Alert Dialog for selecting the BLE device
            }
        }); // end setOnClickListener

        mSlopeChart = (SlopeChart)findViewById(R.id.viewChart);

        Toolbar mToolbar = (Toolbar) findViewById(R.id.toolbar);

        setSupportActionBar(mToolbar);
        getSupportActionBar().setDisplayShowTitleEnabled(false);
        //configureToolbar(mToolbar);
    }

    // note: the arduino code constants are the master for these
    private static final byte SENDVERB_SET_STRAIN = 1;

    private void toggleStrain() {
        byte rgSend[] = new byte[4];
        rgSend[0] = (byte)0xfe;
        rgSend[1] = (byte)0xef;
        rgSend[2] = SENDVERB_SET_STRAIN;
        rgSend[3] = (byte)(mStrainEnabled ? 0 : 1);
        mStrainEnabled = !mStrainEnabled;

        this.serialSendBytes(rgSend);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_main, menu);

        return true;
    }

    protected void onResume(){
        super.onResume();
        System.out.println("BlUNOActivity onResume");
        onResumeProcess();														//onResume Process by BlunoLibrary
    }

    public boolean onOptionsItemSelected (MenuItem item) {
        switch(item.getItemId()) {
            case R.id.toggle_strain:
                this.toggleStrain();
                break;
        }
        return true;
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
        // note: the arduino code constants are the master for these
        public static final int POWER = 0;
        public static final int SPEED = 1;
        public static final int POINT = 2;
        public static final int SLOPE = 3;
        public static final int WHEELTURNS = 4;
        public static final int STRAIN = 5;

        private int cWheelTurns;
        private float flEstimatedSlope; // slope for y=mx, where x = speed, y = watts
        private Vector<XYPoint> lstPoints = new Vector<XYPoint>();
        private float flLastPower;
        private float flLastSpeed;
        private float flLastStrain;

        public float getSpeedMs() {
            return flLastSpeed;
        }
        public float getPower() {
            return flLastPower;
        }
        public int getTurns() {
            return cWheelTurns;
        }
        public float getSlope() {
            return flEstimatedSlope;
        }
        public Vector<XYPoint> getPointData() {
            return lstPoints;
        }
        public float getStrain() {return flLastStrain;}

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
        public void setStrain(float strain) {this.flLastStrain = strain;}
    }

    public void parseSerial(String str) {
        String[] strTypeAndArgs = str.split(":");
        if(strTypeAndArgs.length > 1) {
            String strType = strTypeAndArgs[0];
            String[] rgArgs = strTypeAndArgs[1].split(",");
            int iType = D.tryParseInt(strType, -1);
            switch(iType) {
                case TrainerState.WHEELTURNS:
                    // single arg:
                    if(rgArgs.length >= 1) {
                        int cWheelTurns = D.tryParseInt(rgArgs[0]);
                        mTrainerState.setWheelTurns(cWheelTurns);
                    }
                    break;
                case TrainerState.SLOPE:
                    // single floating arg
                    if(rgArgs.length >= 1) {
                        float flSlope = D.tryParseFloat(rgArgs[0]);
                        mTrainerState.setSlope(flSlope);
                    }
                    break;
                case TrainerState.SPEED:
                    // single floating arg indicating speed in m/s
                    if(rgArgs.length >= 1) {
                        float speed = D.tryParseFloat(rgArgs[0]);
                        mTrainerState.setSpeed(speed);
                    }
                    break;
                case TrainerState.POWER:
                    // single floating arg indicating power in W
                    if(rgArgs.length >= 1) {
                        float power = D.tryParseFloat(rgArgs[0]);
                        mTrainerState.setPower(power);
                    }
                    break;
                case TrainerState.STRAIN:
                    // single float indicating last strain reading
                    if(rgArgs.length >= 1) {
                        float strain = D.tryParseFloat(rgArgs[0]);
                        mTrainerState.setStrain(strain);
                    }
                    break;
                case TrainerState.POINT:
                    // two floats, then an int
                    // <speed m/s> <power W> <index>
                    if(rgArgs.length >= 3) {
                        float speed = D.tryParseFloat(rgArgs[0]);
                        float power = D.tryParseFloat(rgArgs[1]);
                        int index = D.tryParseInt(rgArgs[2]);
                        mTrainerState.addPoint(speed, power, index);
                    }
                    break;
            }
        }
    }

    private void refreshUI() {
        TextView tvPower = (TextView)this.findViewById(R.id.txtPower);
        TextView tvSpeed = (TextView)this.findViewById(R.id.txtSpeed);
        TextView tvSlope = (TextView)this.findViewById(R.id.txtSlope);
        TextView tvTurns = (TextView)this.findViewById(R.id.txtTurns);
        TextView tvStrain = (TextView)this.findViewById(R.id.txtStrain);

        tvPower.setText("Power: " + (int)mTrainerState.getPower() + "W");
        tvSpeed.setText("Speed: " + String.format("%.1f",mTrainerState.getSpeedMs()*3.6) + "km/h");
        tvSlope.setText("Slope: " + String.format("%.2f",mTrainerState.getSlope()) + "W/m/s");
        tvTurns.setText("Turns: " + (int)mTrainerState.getTurns() + "");

        if(mStrainEnabled) {
            tvStrain.setVisibility(View.VISIBLE);
            tvStrain.setText("Strain: " + mTrainerState.getStrain() + "");
        } else {
            tvStrain.setVisibility(View.GONE);
        }


        mSlopeChart.setChartData(mTrainerState.getPointData(), mTrainerState.getSlope());
    }

    int cPoint = 99;
    @Override
    public void onSerialReceived(String theString) {
        // we got a string!
        // format is:
        // <type of transmission>:<bunch of comma-separated parameters>

        /*{ // simulatin'...
            cPoint = (cPoint + 1) % 100;
            final float flSimSpeed = (float)(Math.random()*10);
            final float flSimPower = (float)(5*flSimSpeed + Math.random()*5);
            this.parseSerial(TrainerState.POINT + ":" + flSimSpeed + "," + flSimPower + "," + cPoint);
            this.parseSerial(TrainerState.POWER + ":" + flSimPower);
            this.parseSerial(TrainerState.SLOPE + ":" + Math.random()*5);
            this.parseSerial(TrainerState.SPEED + ":" + flSimSpeed);
            this.parseSerial(TrainerState.WHEELTURNS + ":" + cPoint);
        }*/
        D.p("String: '" + theString + "'");
        theString = theString.replace("\r\n","\n"); // just do single-separaters...
        String[] rgLines = theString.split("\n");
        for(int x = 0;x < rgLines.length; x++)
        {
            try
            {
                parseSerial(rgLines[x]);
            }
            catch(Exception e)
            {
                // the world will live on...
            }
        }

        refreshUI();
        mSlopeChart.invalidate();
        // this is called on the UI thread, so we can do a refreshUI now
    }

}