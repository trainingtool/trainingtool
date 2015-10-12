package com.stac.powertrainer_44;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.view.View;
import java.util.Vector;
import java.util.List;
import java.util.Iterator;
/**
 * TODO: document your custom view class.
 */
public class SlopeChart extends View {
    private Paint mDots = new Paint();
    private Paint mLines = new Paint();
    private Paint mBackground = new Paint();
    private List<XYPoint> mPointData = new Vector<XYPoint>();

    public SlopeChart(Context context) {
        super(context);
        init(null, 0);
    }

    public SlopeChart(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    public SlopeChart(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(attrs, defStyle);
    }

    private void init(AttributeSet attrs, int defStyle) {
        mDots.setColor(Color.RED);
        mLines.setColor(Color.BLACK);

        mBackground.setColor(Color.WHITE);
        mBackground.setStyle(Paint.Style.FILL);

    }

    public void setChartData(List<XYPoint> lstPoints, float flSlopeLine) {
        mPointData = lstPoints.subList(0, lstPoints.size());
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        // fill background
        canvas.drawRect(0, 0, canvas.getWidth(), canvas.getHeight(), mBackground);

        if(mPointData != null) {
            // we have at least a little point data, let's draw it!
            Iterator<XYPoint> i = mPointData.iterator();
            XYPoint xyMax = new XYPoint(Integer.MIN_VALUE, Integer.MIN_VALUE);
            XYPoint xyMin = new XYPoint(Integer.MAX_VALUE, Integer.MAX_VALUE);

            while(i.hasNext()) {
                XYPoint xy = i.next();
                xyMax.x = Math.max(xyMax.x, xy.x);
                xyMax.y = Math.max(xyMax.y, xy.y);
                xyMin.x = Math.min(xyMin.x, xy.x);
                xyMin.y = Math.min(xyMin.y, xy.y);
            }

            // we've got maxes and mins!
            final float flXSPan = xyMax.x - xyMin.x;
            final float flYSpan = xyMax.y - xyMin.y;

            final float flCanvasX = canvas.getWidth();
            final float flCanvasY = canvas.getHeight();

            final float flPointWidth = flCanvasX / 20.0f; // a point is 1/20th the width of the canvas
            final float flPointHeight = (flCanvasY / 20.0f);


            i = mPointData.iterator();
            while(i.hasNext()) {
                XYPoint xy = i.next();
                final float flPctX = (xy.x - xyMin.x) / flXSPan;
                final float flPctY = (xy.y - xyMin.y) / flYSpan;

                final float flCenterX = flPctX * flCanvasX;
                final float flCenterY = flPctY * flCanvasY;
                canvas.drawLine(flCenterX-flPointWidth, flCenterY, flCenterX + flPointWidth, flCenterY, mDots);
                canvas.drawLine(flCenterX, flCenterY-flPointHeight, flCenterX, flCenterY+flPointHeight, mDots);
            }
        }
    }
}
