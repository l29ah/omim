package com.mapswithme.maps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import com.mapswithme.maps.R;

/**
 * Draws progress wheel, consisting of circle with background and 'stop' button in the center of the circle.
 */
public class WheelProgressView extends View
{
  private static final int DEFAULT_THICKNESS = 4;

  private int mProgress;
  private int mRadius;
  private Paint mFgPaint;
  private Paint mBgPaint;
  private int mStrokeWidth;
  private Paint mRectPaint; // paint for rectangle(stop button)
  private RectF mProgressRect; // main rect for progress wheel
  private RectF mCenterRect; // rect for stop button
  private PointF mCenter; // center of rect

  public WheelProgressView(Context context)
  {
    super(context);
    init(null);
  }

  public WheelProgressView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    init(attrs);
  }

  public WheelProgressView(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
    init(attrs);
  }

  private void init(AttributeSet attrs)
  {
    final TypedArray typedArray = getContext().obtainStyledAttributes(attrs, R.styleable.WheelProgressView, 0, 0);
    mStrokeWidth = typedArray.getDimensionPixelSize(R.styleable.WheelProgressView_thickness, DEFAULT_THICKNESS);
    final int color = typedArray.getColor(R.styleable.WheelProgressView_progressColor, getResources().getColor(R.color.downloader_gray_bg));
    final int secondaryColor = typedArray.getColor(R.styleable.WheelProgressView_secondaryColor, getResources().getColor(R.color.base_green));
    typedArray.recycle();

    mBgPaint = new Paint();
    mBgPaint.setColor(secondaryColor);
    mBgPaint.setStrokeWidth(mStrokeWidth);
    mBgPaint.setStyle(Paint.Style.STROKE);
    mBgPaint.setAntiAlias(true);

    mFgPaint = new Paint();
    mFgPaint.setColor(color);
    mFgPaint.setStrokeWidth(mStrokeWidth);
    mFgPaint.setStyle(Paint.Style.STROKE);
    mFgPaint.setAntiAlias(true);

    mRectPaint = new Paint();
    mRectPaint.setColor(color);
    mRectPaint.setStrokeWidth(mStrokeWidth);
    mRectPaint.setStyle(Paint.Style.FILL);
    mRectPaint.setAntiAlias(true);
  }

  public void setProgress(int progress)
  {
    mProgress = progress;
    invalidate();
  }

  public int getProgress()
  {
    return mProgress;
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    int left = getPaddingLeft();
    int top = getPaddingTop();
    int right = w - getPaddingRight();
    int bottom = h - getPaddingBottom();
    int width = right - left;
    int height = bottom - top;

    mRadius = (Math.min(width, height) - mStrokeWidth) / 2;
    mCenter = new PointF(left + width / 2, top + height / 2);
    mProgressRect = new RectF(mCenter.x - mRadius, mCenter.y - mRadius, mCenter.x + mRadius, mCenter.y + mRadius);
    mCenterRect = new RectF(mCenter.x - width / 6, mCenter.y - height / 6, mCenter.x + width / 6, mCenter.y + height / 6);
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    canvas.drawCircle(mCenter.x, mCenter.y, mRadius, mBgPaint);
    canvas.drawArc(mProgressRect, -90, 360 * mProgress / 100, false, mFgPaint);
    canvas.drawRoundRect(mCenterRect, 3, 3, mRectPaint);
    super.onDraw(canvas);
  }
}
